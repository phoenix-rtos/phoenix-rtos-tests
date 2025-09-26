import random
import select
import re
import socket
import subprocess
import os
import threading
import struct
import time
from dataclasses import dataclass
from multiprocessing.pool import ThreadPool

import psh.tools.psh as psh
from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status
from trunner.harness.unity import unity_harness


def get_config():
    """Return config depending if trunner is being run inside github-runner container or not"""
    out = subprocess.run(
        ["ifconfig", "eth0"],
        encoding="ascii",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )

    host_ip = os.environ.get("HOST_IP")
    netmask = os.environ.get("HOST_MASK")
    doc_ip = None

    if host_ip is None or netmask is None:
        # Host interface
        host_ip = re.search("inet ([0-9.]+)", out.stdout).group(1)
        netmask = re.search("netmask ([0-9.]+)", out.stdout).group(1)
    else:
        # Docker interface
        doc_ip = re.search("inet ([0-9.]+)", out.stdout).group(1)

    octets = host_ip.split(".")
    fourth_octet = int(octets[3]) + 100
    target_ip = ".".join(octets[:3] + [str(fourth_octet)])

    return doc_ip, host_ip, target_ip, netmask


def target_setup(p, iface, ip, netmask):
    ifconfig_setup_cmd = f"ifconfig {iface} {ip} netmask {netmask} up"
    psh.assert_prompt_after_cmd(p, ifconfig_setup_cmd, "success")


def get_conn(doc_ip, host_ip, port, num=1):
    """This function return 1 or list of `num` connections to target"""
    sockets = []

    try:
        host_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        host_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        if doc_ip is None:
            host_socket.bind((host_ip, port))
        else:
            host_socket.bind((doc_ip, port))

        host_socket.listen(num)

        while len(sockets) < num:
            ready_to_read, _, _ = select.select([host_socket], [], [], 20)

            if ready_to_read:
                con, _ = host_socket.accept()
                sockets.append(con)
            else:
                for i in range(len(sockets)):
                    sockets[i].close()

                print("Host error - get_conn: listen timeout")
                return None

        host_socket.close()

    except socket.error as e:
        print(f"Host error - get_conn: {e}")
        return None

    if len(sockets) == 1:
        return sockets[0]

    return sockets


def create_IP_header(src_ip, dest_ip):
    # Convert IP addresses to integer
    src_ip_int = struct.unpack("!I", socket.inet_aton(src_ip))[0]
    dest_ip_int = struct.unpack("!I", socket.inet_aton(dest_ip))[0]

    ip_header = struct.pack("!BBHHHBBHII",
                            69,           # Version and IHL
                            0,            # Type of Service
                            0,            # Total length (filled by kernel)
                            0,            # Packet id
                            0x4000,       # Flags and Fragment Offset
                            64,           # Time to Live
                            6,            # TCP protocol
                            0,            # Checksum (filled by kernel)
                            src_ip_int,   # Source Address
                            dest_ip_int)  # Destination Address

    return ip_header


def create_pseudo_header(src_ip, dest_ip, payload):
    # Convert IP addresses to integer
    src_ip_int = struct.unpack("!I", socket.inet_aton(src_ip))[0]
    dest_ip_int = struct.unpack("!I", socket.inet_aton(dest_ip))[0]

    if payload is True:
        payload_len = 4
    else:
        payload_len = 0

    tcp_len = struct.calcsize("!HHIIBBHHH") + payload_len

    pseudo_header = struct.pack("!IIBBH",
                                src_ip_int,   # Source IP address
                                dest_ip_int,  # Destination IP address
                                0,            # Reserved
                                6,            # TCP protocol
                                tcp_len)      # TCP segment length

    return pseudo_header


@dataclass
class TCP_Data:
    src_port: int
    dest_port: int
    seq: int
    ack: int
    flags: int
    window: int
    checksum: int
    wrong_checksum: int
    urg: int
    payload: bool


def create_tcp_data(seq, ack, window, src_port, target_port, **kwargs) -> TCP_Data:
    src_port = src_port
    dest_port = target_port
    checksum = 0
    wrong_checksum = None
    urg = 0
    payload = True
    flags = 24  # ACK + PUSH

    key, value = next(iter(kwargs.items()))
    if key == "bad_src_port":
        src_port = value
    elif key == "bad_dest_port":
        dest_port = value
    elif key == "bad_seq":
        seq = value
    elif key == "bad_ack":
        ack = value
    elif key == "bad_window":
        window = value
    elif key == "bad_checksum":
        wrong_checksum = value
    elif urg == "bad_urg":
        urg = value
    elif key == "flags":
        flags = value
        if flags == 20:  # RST + ACK
            ack = 0
            window = 0
            payload = False

    tcp_data = TCP_Data(src_port, dest_port, seq, ack, flags,
                        window, checksum, wrong_checksum, urg, payload)

    return tcp_data


def create_tcp_header(tcp_data: TCP_Data):
    tcp_header = struct.pack("!HHIIBBHHH",
                             tcp_data.src_port,   # Source port
                             tcp_data.dest_port,  # Destination port
                             tcp_data.seq,        # Sequence number
                             tcp_data.ack,        # Acknowledgment number
                             5 << 4,              # Data offset
                             tcp_data.flags,      # Flags
                             tcp_data.window,     # Window size
                             tcp_data.checksum,   # Checksum
                             tcp_data.urg)        # Urgent pointer

    return tcp_header


def calc_tcp_checksum(data):
    sum = 0
    data_len = len(data)

    for i in range(0, data_len - data_len % 2, 2):
        sum += (data[i] << 8) + data[i + 1]

    if data_len % 2:
        sum += data[-1] << 8

    while sum >> 16:
        sum = (sum & 0xFFFF) + (sum >> 16)

    return ~sum & 0xFFFF


def create_packet(src_ip, dest_ip, tcp_data: TCP_Data):
    ip_header = create_IP_header(src_ip, dest_ip)
    tcp_header = create_tcp_header(tcp_data)
    pseudo_header = create_pseudo_header(src_ip, dest_ip, tcp_data.payload)
    payload = bytes([1, 2, 3, 4])

    if tcp_data.payload is True:
        data = pseudo_header + tcp_header + payload
    else:
        data = pseudo_header + tcp_header

    checksum = calc_tcp_checksum(data)
    tcp_data.checksum = checksum

    if tcp_data.wrong_checksum is not None:
        tcp_data.checksum = tcp_data.wrong_checksum

    tcp_header = create_tcp_header(tcp_data)

    if tcp_data.payload is True:
        packet = ip_header + tcp_header + payload
    else:
        packet = ip_header + tcp_header

    return packet


def sniff_packet(target_ip, result):
    # Sniff last packet of 3 way handshake to get connection details
    raw_socket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_TCP)
    timeout = struct.pack('ll', 15, 0)
    raw_socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeout)
    prev_seq = None

    while True:
        try:
            packet, addr = raw_socket.recvfrom(128)
        except socket.error as e:
            raw_socket.close()
            print(f"Host error - sniff thread: {e}")
            return

        ack = (packet[28] << 24) + (packet[29] << 16) + (packet[30] << 8) + packet[31]
        seq = (packet[24] << 24) + (packet[25] << 16) + (packet[26] << 8) + packet[27]

        if ack == 0:
            prev_seq = (packet[24] << 24) + (packet[25] << 16) + (packet[26] << 8) + packet[27]

        if addr[0] == target_ip and ack != 0 and prev_seq is not None and prev_seq + 1 == seq:
            seq = (packet[24] << 24) + (packet[25] << 16) + (packet[26] << 8) + packet[27]
            window = (packet[34] << 8) + packet[35]
            target_port = (packet[20] << 8) + packet[21]

            result.append(ack)
            result.append(seq)
            result.append(window)
            result.append(target_port)
            raw_socket.close()
            break


def end_tc(sync_socket, err_msg, target_socket):
    success_msg = "success"

    if len(err_msg) != 0:
        # Send error message indicating fail
        sync_socket.sendall(err_msg.encode("utf-8") + b"\0")
    else:
        # Send success message
        sync_socket.sendall(success_msg.encode("utf-8") + b"\0")

    if target_socket is not None:
        target_socket.close()


def tc_basic(target_socket, sync_socket):
    err_msg = ""

    if target_socket is None:
        err_msg = "Host error: connection failed"
        end_tc(sync_socket, err_msg, target_socket)
        return

    try:
        send_data = bytes([random.randint(0, 128) for _ in range(128)])
        target_socket.sendall(send_data)

        recv_data = target_socket.recv(128, socket.MSG_WAITALL)
        recv_len = len(recv_data)
        if recv_len != 128:
            err_msg = f"Host error: received just {recv_len} bytes out of 128"
            end_tc(sync_socket, err_msg, target_socket)
            return

        send_data = bytes([recv_data[i] - send_data[i] for i in range(128)])
        target_socket.sendall(send_data)
    except socket.error as e:
        err_msg = f"Host error: {e}"
        end_tc(sync_socket, err_msg, target_socket)
        return

    end_tc(sync_socket, err_msg, target_socket)


def tc_big_data(target_socket, sync_socket):
    err_msg = ""
    data_size = 24 * 1024

    if target_socket is None:
        err_msg = "Host error: connection failed"
        end_tc(sync_socket, err_msg, target_socket)
        return

    try:
        send_data = bytes([random.randint(0, 128) for _ in range(data_size)])
        target_socket.sendall(send_data)

        timeout = struct.pack('ll', 5, 0)
        target_socket.setsockopt(socket.SOL_SOCKET, socket.SO_RCVTIMEO, timeout)

        recv_data = target_socket.recv(data_size, socket.MSG_WAITALL)
        recv_len = len(recv_data)
        if recv_len != data_size:
            err_msg = f"Host error: received just {recv_len} bytes out of {data_size}"
            end_tc(sync_socket, err_msg, target_socket)
            return

        send_data = bytes([recv_data[i] - send_data[i] for i in range(data_size)])
        target_socket.sendall(send_data)
    except socket.error as e:
        err_msg = f"Host error: {e}"
        end_tc(sync_socket, err_msg, target_socket)
        return

    end_tc(sync_socket, err_msg, target_socket)


def tc_accept_connections(target_socket, sync_socket):
    err_msg = ""
    target_ip = target_socket.getpeername()[0]
    target_port = 1900
    max_retries = 10
    retry_delay = 0.5
    num = 200  # Number of connections to make

    if target_socket is None:
        err_msg = "Host error: connection failed"
        end_tc(sync_socket, err_msg, target_socket)
        return

    try:
        # Wait until target start listening
        sockets = []
        for _ in range(num):
            con_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sockets.append(con_sock)

        for i in range(num):
            attempts = 0
            # Sleep abit to not overflow accept mbox
            time.sleep(0.1)
            while attempts < max_retries:
                try:
                    sockets[i].connect((target_ip, target_port))
                    break
                except socket.error as e:
                    attempts += 1
                    if attempts < max_retries:
                        time.sleep(retry_delay)
                    else:
                        err_msg = f"Host error: failed to connect after {max_retries} attempts: {e}"
                        end_tc(sync_socket, err_msg, target_socket)
                        return

        for i in range(num):
            sockets[i].close()

    except socket.error as e:
        err_msg = f"Host error: {e}"
        end_tc(sync_socket, err_msg, target_socket)
        return

    end_tc(sync_socket, err_msg, target_socket)


def tc_send_after_close(target_socket, sync_socket):
    # Just close host side of connection
    err_msg = ""
    end_tc(sync_socket, err_msg, target_socket)


def tc_recv_remaining_data(target_socket, sync_socket):
    err_msg = ""
    data_size = 24 * 1024

    if target_socket is None:
        err_msg = "Host error: connection failed"
        end_tc(sync_socket, err_msg, target_socket)
        return

    try:
        send_data = bytes([random.randint(0, 128) for _ in range(data_size)])
        target_socket.sendall(send_data)

        target_socket.close()

    except socket.error as e:
        err_msg = f"Host error: {e}"
        end_tc(sync_socket, err_msg, target_socket)
        return

    end_tc(sync_socket, err_msg, target_socket=None)


def basic_transmit_reveive(target_socket):
    for _ in range(30):
        try:
            send_data = bytes([random.randint(0, 128) for _ in range(128)])
            target_socket.sendall(send_data)

            recv_data = target_socket.recv(128, socket.MSG_WAITALL)
            recv_len = len(recv_data)
            if recv_len != 128:
                err_msg = f"Host error: received just {recv_len} bytes out of 128"
                return err_msg

            send_data = bytes([recv_data[i] - send_data[i] for i in range(128)])
            target_socket.sendall(send_data)
        except socket.error as e:
            err_msg = f"Host error: {e}"
            return err_msg

    return ""


def tc_simultaneous_clients(target_sockets, sync_socket):
    err_msg = ""
    thread_num = 20

    if target_sockets is None:
        err_msg = "Host error: setting connections failed"
        end_tc(sync_socket, err_msg, None)
        return

    pool = ThreadPool(processes=thread_num)
    threads = []
    for i in range(thread_num):
        threads.append(pool.apply_async(basic_transmit_reveive, (target_sockets[i],)))

    results = []
    for thread in threads:
        result = thread.get()
        results.append(result)

    pool.close()
    pool.join()

    for i in range(thread_num):
        target_sockets[i].close()

    for i in range(thread_num):
        if len(results[i]) != 0:
            err_msg = results[i]
            end_tc(sync_socket, err_msg, None)
            return

    end_tc(sync_socket, err_msg, None)


def tc_receive_rst(target_socket, sync_socket, packet):
    err_msg = ""

    if packet is None:
        err_msg = "Host error: testcase setup failed"
        end_tc(sync_socket, err_msg, target_socket)
        return

    if target_socket is None:
        err_msg = "Host error: connection failed"
        end_tc(sync_socket, err_msg, target_socket)
        return

    raw_socket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_TCP)
    raw_socket.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1)

    target_ip = target_socket.getpeername()[0]
    ret = raw_socket.sendto(packet, (target_ip, 0))
    if ret <= 0:
        err_msg = "Host error: Failed to send raw packet"
        end_tc(sync_socket, err_msg, target_socket)
        return

    try:
        target_socket.sendall(bytes([1, 2, 3, 4]))
    except socket.error as e:
        err_msg = f"Host error: {e}"
        end_tc(sync_socket, err_msg, target_socket)
        return

    end_tc(sync_socket, err_msg, target_socket)


def raw_tc_setup(doc_ip, host_ip, host_port, target_ip, **kwargs):
    result = []
    sniff_thread = threading.Thread(target=sniff_packet, args=(target_ip, result))
    sniff_thread.start()
    target_socket = get_conn(doc_ip, host_ip, host_port)
    sniff_thread.join()

    if not result:
        return None, None

    # Get values set by sniff_thread
    seq = result[0]
    ack = result[1]
    window = result[2]
    target_port = result[3]

    tcp_data = create_tcp_data(seq, ack, window, host_port, target_port, **kwargs)
    if doc_ip is None:
        packet = create_packet(host_ip, target_ip, tcp_data)
    else:
        packet = create_packet(doc_ip, target_ip, tcp_data)

    return target_socket, packet


def raw_tc(target_socket, sync_socket, packet):
    err_msg = ""

    if packet is None:
        err_msg = "Host error: testcase setup failed"
        end_tc(sync_socket, err_msg, target_socket)
        return

    if target_socket is None:
        err_msg = "Host error: connection failed"
        end_tc(sync_socket, err_msg, target_socket)
        return

    raw_socket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_TCP)
    raw_socket.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1)

    target_ip = target_socket.getpeername()[0]
    ret = raw_socket.sendto(packet, (target_ip, 0))
    if ret <= 0:
        err_msg = "Host error: Failed to send raw packet"
        end_tc(sync_socket, err_msg, target_socket)
        return

    ready_to_read, _, _ = select.select([target_socket], [], [], 5)

    if not ready_to_read:
        err_msg = "Host error: Packet with error was accepted"
        end_tc(sync_socket, err_msg, target_socket)
        return

    end_tc(sync_socket, err_msg, target_socket)


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    doc_ip, host_ip, target_ip, netmask = get_config()
    target_port = 1900
    if doc_ip is not None:
        # Tests run using docker (packet redirection)
        host_port = 1700
    else:
        # Tests run locally
        host_port = 1800

    target_setup(dut, "en1", target_ip, netmask)

    if ctx.target.rootfs:
        dut.send(f"/bin/test-tcp {host_ip}:{host_port} {target_ip}:{target_port}\n")
        dut.expect(f"/bin/test-tcp {host_ip}:{host_port} {target_ip}:{target_port}" + psh.EOL)
    else:
        dut.send(f"sysexec test-tcp {host_ip}:{host_port} {target_ip}:{target_port}\n")
        dut.expect(f"sysexec test-tcp {host_ip}:{host_port} {target_ip}:{target_port}" + psh.EOL)

    sync_socket = get_conn(doc_ip, host_ip, host_port)
    if sync_socket is None:
        return TestResult(None, "Host error: sync socket set up failed", Status.FAIL)

    try:
        raw_socket = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_TCP)
    except PermissionError:
        return TestResult(None, "Host error: python has no cap_net_raw capabilities set", Status.FAIL)
    else:
        raw_socket.close()

    pool = ThreadPool(processes=1)
    target_harness = pool.apply_async(unity_harness, (dut, ctx, result))

    # Testcase basic
    target_socket = get_conn(doc_ip, host_ip, host_port)
    tc_basic(target_socket, sync_socket)

    # Testcase big_data
    target_socket = get_conn(doc_ip, host_ip, host_port)
    tc_big_data(target_socket, sync_socket)

    # # Testcase accept_connections
    target_socket = get_conn(doc_ip, host_ip, host_port)
    tc_accept_connections(target_socket, sync_socket)

    # # Testcase send_after_close
    target_socket = get_conn(doc_ip, host_ip, host_port)
    tc_send_after_close(target_socket, sync_socket)

    # # Testcase recv_remaining_data
    target_socket = get_conn(doc_ip, host_ip, host_port)
    tc_recv_remaining_data(target_socket, sync_socket)

    # # Testcase simultaneous_clients
    # # It is here to match testcase setup on target TODO: get rid of it
    # target_socket = get_conn(doc_ip, host_ip, host_port)
    # target_socket.close()

    # target_sockets = get_conn(doc_ip, host_ip, host_port, 20)
    # tc_simultaneous_clients(target_sockets, sync_socket)

    # Testcase receive_rst
    target_socket, packet = raw_tc_setup(doc_ip, host_ip, host_port, target_ip, flags=20)
    tc_receive_rst(target_socket, sync_socket, packet)

    # Testcase wrong_src_port
    target_socket, packet = raw_tc_setup(doc_ip, host_ip, host_port, target_ip, bad_src_port=1)
    raw_tc(target_socket, sync_socket, packet)

    # Testcase wrong_dest_port
    target_socket, packet = raw_tc_setup(doc_ip, host_ip, host_port, target_ip, bad_dest_port=12345)
    raw_tc(target_socket, sync_socket, packet)

    # Testcase wrong_seq
    target_socket, packet = raw_tc_setup(doc_ip, host_ip, host_port, target_ip, bad_seq=12345)
    raw_tc(target_socket, sync_socket, packet)

    # Testcase wrong_chk_sum
    target_socket, packet = raw_tc_setup(doc_ip, host_ip, host_port, target_ip, bad_checksum=12345)
    raw_tc(target_socket, sync_socket, packet)

    # Testcase wrong_ack
    # Packets with wrong ack are accepted ISSUE??
    # target_socket, packet = raw_tc_setup(doc_ip, host_ip, host_port, target_ip, bad_ack=1)
    # raw_tc(target_socket, sync_socket, packet)

    sync_socket.close()

    return target_harness.get()
