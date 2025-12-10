import socket
import os
import re

from typing import Optional

from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import Status, TestResult


class NetworkTestResponder:
    SOCKET_TIMEOUT = 10
    PORT = 50000
    peer_ip = None
    sock = None
    data = None

    @staticmethod
    def _get_peer_ip() -> str:
        iface_config = os.environ.get("IFACE_CONFIG")
        match = re.search(r"\d{1,3}(\.\d{1,3}){3}", iface_config)
        return match.group()

    @classmethod
    def do_cmd(cls, cmd: str):
        if cmd == "Accept":
            cls.peer_ip = cls._get_peer_ip()
            cls.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            cls.sock.settimeout(cls.SOCKET_TIMEOUT)
            cls.sock.connect((cls.peer_ip, cls.PORT))

        elif cmd == "Connect":
            listen_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            listen_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            listen_sock.settimeout(cls.SOCKET_TIMEOUT)
            listen_sock.bind(("0.0.0.0", cls.PORT))
            listen_sock.listen()
            cls.sock, _ = listen_sock.accept()
            listen_sock.close()

        elif cmd == "Close":
            cls.sock.close()

        elif cmd.startswith("Transmit"):
            size = int(re.search(r"\d+", cmd).group())

            if cls.data is None:
                cls.data = bytearray()

            cls.data += cls.sock.recv(size, socket.MSG_WAITALL)

        elif cmd.startswith("Receive"):
            size = int(re.search(r"\d+", cmd).group())
            cls.sock.sendall(cls.data[:size])
            cls.data = cls.data[size:]


def unity_harness(dut: Dut, ctx: TestContext, result: TestResult) -> Optional[TestResult]:
    assert_re = r"ASSERTION [\S]+:\d+:(?P<status>FAIL|INFO|IGNORE)(: (?P<msg>.*?))?\r"
    result_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>PASS|IGNORE)"
    # Fail need to have its own regex due to greedy matching
    result_fail_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>FAIL) at (?P<path>.*?):(?P<line>\d+)\r"
    final_re = r"(?P<total>\d+) Tests (?P<fail>\d+) Failures (?P<ignore>\d+) Ignored \r+\n(?P<result>OK|FAIL)"
    network_re = r"NET: (?P<cmd>[^\r\n]+)\r?\n"

    last_assertion = {}
    stats = {"FAIL": 0, "IGNORE": 0, "PASS": 0}
    results = []
    # some unity tests (e.g. mprotect) take 20 or even 30+ seconds on zynqmp-qemu
    timeout_val = 30 if ctx.target.name != "aarch64a53-zynqmp-qemu" else 60
    if ctx.nightly:
        timeout_val = 60

    while True:
        idx = dut.expect([assert_re, result_re, result_fail_re, final_re, network_re], timeout=timeout_val)
        parsed = dut.match.groupdict()

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

            break

        elif idx == 4:
            NetworkTestResponder.do_cmd(parsed["cmd"])

    status = Status.FAIL if stats["FAIL"] != 0 else Status.OK
    return TestResult(status=status)
