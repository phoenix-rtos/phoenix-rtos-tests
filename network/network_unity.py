import socket
import select
import struct
import pexpect
import re
import os
import time

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult


class Connection:
    SOCKET_TIMEOUT = 3

    def __init__(self):
        self.sock = None
        self.data = bytearray()
        self.events = 0

    def connect(self, ip, port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, struct.pack("ll", self.SOCKET_TIMEOUT, 0))
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDTIMEO, struct.pack("ll", self.SOCKET_TIMEOUT, 0))
        self.sock.connect((ip, port))

    def accept(self, port):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listen_sock:
            listen_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listen_sock.settimeout(float(self.SOCKET_TIMEOUT))
            listen_sock.bind(("0.0.0.0", port))
            listen_sock.listen()

            self.sock, _ = listen_sock.accept()
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, struct.pack("ll", self.SOCKET_TIMEOUT, 0))
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDTIMEO, struct.pack("ll", self.SOCKET_TIMEOUT, 0))

    def recv(self, size):
        data = self.sock.recv(size, socket.MSG_WAITALL)
        self.data += data

        return data

    def send(self, size):
        self.sock.sendall(os.urandom(size))

    def send_received(self, size):
        self.sock.sendall(self.data[:size])
        self.data = self.data[size:]

    def close(self, forcibly=False):
        if forcibly:
            linger_struct = struct.pack('ii', 1, 0)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, linger_struct)

        self.sock.close()
        self.sock = None
        self.data = None


class NetworkTestResponder:
    CMD_SOCKET_TIMEOUT = 3
    PORT = 50000
    EVENT_RECV_EOF = 1 << 0
    EVENT_SEND_BLOCKED = 1 << 1

    def __init__(self):
        self.peer_ip = self._get_peer_ip()
        self.connections = []
        self.cmd_sock = self._setup_cmd_sock()
        self.cmd_fd = self.cmd_sock.fileno()

    def _get_peer_ip(self) -> str:
        iface_config = os.environ.get("IFACE_CONFIG")
        match = re.search(r"\d{1,3}(\.\d{1,3}){3}", iface_config)
        return match.group()

    def _setup_cmd_sock(self):
        for _ in range(10):
            try:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.connect((self.peer_ip, self.PORT))
                break
            except ConnectionRefusedError:
                sock.close()
                time.sleep(0.5)

        sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, struct.pack("ll", self.CMD_SOCKET_TIMEOUT, 0))
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDTIMEO, struct.pack("ll", self.CMD_SOCKET_TIMEOUT, 0))
        return sock

    def do_cmd(self, cmd: str):
        if cmd == "Connect":
            c = Connection()
            c.connect(self.peer_ip, self.PORT)
            self.connections.append(c)

        elif cmd == "Accept":
            c = Connection()
            c.accept(self.PORT)
            self.connections.append(c)

        elif "Receive" in cmd:
            match = re.search(r"(?:\((\d+)\)\s*)?Receive\s+(\d+)", cmd)
            idx = int(match.group(1)) if match.group(1) else 0
            size = int(match.group(2))

            n = self.connections[idx].recv(size)

            if n == b'':
                self.connections[idx].events |= self.EVENT_RECV_EOF

        elif "Send received" in cmd:
            match = re.search(r"(?:\((\d+)\)\s*)?Send received\s+(\d+)", cmd)
            idx = int(match.group(1)) if match.group(1) else 0
            size = int(match.group(2))

            try:
                self.connections[idx].send_received(size)
            except (socket.timeout, BlockingIOError):
                self.connections[idx].events |= self.EVENT_SEND_BLOCKED

        elif "Send" in cmd:
            match = re.search(r"(?:\((\d+)\)\s*)?Send\s+(\d+)", cmd)
            idx = int(match.group(1)) if match.group(1) else 0
            size = int(match.group(2))

            try:
                self.connections[idx].send(size)
            except (socket.timeout, BlockingIOError):
                self.connections[idx].events |= self.EVENT_SEND_BLOCKED

        elif "Close" in cmd:
            match = re.search(r"(?:\((\d+)\)\s*)?Close", cmd)
            idx = int(match.group(1)) if match.group(1) else 0

            self.connections[idx].close(forcibly="forcibly" in cmd)
            self.connections[idx] = None

            if all(conn is None for conn in self.connections):
                self.connections = []

        elif cmd == "Get events":
            match = re.search(r"(?:\((\d+)\)\s*)?Get events", cmd)
            idx = int(match.group(1)) if match.group(1) else 0

            msg = f"NET: {self.connections[idx].events}\n"
            os.write(self.cmd_fd, msg.encode("ascii"))


def harness(dut: Dut, ctx: TestContext, result: TestResult) -> TestResult:
    assert_re = r"ASSERTION [\S]+:\d+:(?P<status>FAIL|INFO|IGNORE)(: (?P<msg>.*?))?\r"
    result_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>PASS|IGNORE)"
    # Fail need to have its own regex due to greedy matching
    result_fail_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>FAIL) at (?P<path>.*?):(?P<line>\d+)\r"
    final_re = r"(?P<total>\d+) Tests (?P<fail>\d+) Failures (?P<ignore>\d+) Ignored \r+\n(?P<result>OK|FAIL)"
    network_re = r"NET: (?P<cmd>[^\r\n]+)\r?\n"

    last_assertion = {}
    stats = {"FAIL": 0, "IGNORE": 0, "PASS": 0}
    results = []
    network_test_responder = NetworkTestResponder()

    tty_fd = dut.pexpect_proc.fileno()
    net_fd = network_test_responder.cmd_fd

    cmd_conn = pexpect.fdpexpect.fdspawn(net_fd)

    TIMEOUT = 30.0
    start_time = time.monotonic()

    done = False

    while not done:
        r, _, _ = select.select([tty_fd, net_fd], [], [], TIMEOUT)
        if not r:
            raise TimeoutError("No input from DUT")

        if tty_fd in r:
            while True:
                try:
                    idx = dut.expect([assert_re, result_re, result_fail_re, final_re, network_re], timeout=0)
                    parsed = dut.match.groupdict()
                    start_time = time.monotonic()
                except pexpect.TIMEOUT:
                    elapsed = time.monotonic() - start_time
                    if elapsed >= TIMEOUT:
                        raise

                    break

                if idx == 0:
                    if parsed["status"] in ["FAIL", "IGNORE"]:
                        last_assertion = parsed

                elif idx in (1, 2):
                    if last_assertion.get("msg"):
                        parsed["msg"] = last_assertion["msg"]
                        last_assertion = {}

                    status = Status.from_str(parsed["status"])
                    subname = f"{parsed['group']}.{parsed['name']}"
                    if "path" in parsed and "line" in parsed:
                        parsed["msg"] = f"[{parsed['path']}:{parsed['line']}] " + parsed.get("msg", "")
                    result.add_subresult(subname, status, parsed.get("msg", ""))

                    stats[parsed["status"]] += 1
                    results.append(parsed)

                elif idx == 3:
                    for k, v in parsed.items():
                        if k != "result":
                            parsed[k] = int(v)

                    assert (
                        parsed["total"] == sum(stats.values())
                        and parsed["fail"] == stats["FAIL"]
                        and parsed["ignore"] == stats["IGNORE"]
                    ), "".join(("There is a mismatch between the number of parsed tests and overall results!\n",
                                "Parsed results from the final Unity message (total, failed, ignored): ",
                                f"{parsed['total']}, {parsed['fail']}, {parsed['ignore']}\n",
                                "Found test summary lines (total, failed, ignored): ",
                                f"{sum(stats.values())}, {stats['FAIL']}, {stats['IGNORE']}"))

                    done = True
                    break

                elif idx == 4:
                    network_test_responder.cmd_fd = tty_fd
                    network_test_responder.do_cmd(parsed["cmd"])

        if net_fd in r:
            # End of test might close cmd sock early
            if network_test_responder.cmd_sock.recv(1, socket.MSG_PEEK) == b'':
                pass
            else:
                while True:
                    try:
                        idx = cmd_conn.expect([network_re], timeout=0)
                        parsed = cmd_conn.match.groupdict()
                        start_time = time.monotonic()
                    except pexpect.TIMEOUT:
                        elapsed = time.monotonic() - start_time
                        if elapsed >= TIMEOUT:
                            raise

                        break

                    if idx == 0:
                        network_test_responder.do_cmd(parsed["cmd"].decode("ascii"))

    cmd_conn.close()

    status = Status.FAIL if stats["FAIL"] != 0 else Status.OK
    return TestResult(status=status)
