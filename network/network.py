import random
import select
import time
import re
import socket
import subprocess

from typing import Optional
import psh.tools.psh as psh
from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status


def host_interface():
    out = subprocess.run(
        "ifconfig",
        encoding="ascii",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )

    eth_device = re.findall("eth[0-9]:.+\n.+[0-9]+", out.stdout)
    inet = re.findall("inet [0-9.]+", str(eth_device))
    netmask = re.findall("netmask [0-9.]+", str(eth_device))

    host_setup = inet + netmask
    return host_setup


def en_setup(
    p,
    eth="en1",
    up: Optional[bool] = True,
    wan: Optional[bool] = False,
    ip: Optional[str] = None,
    dynamic: Optional[bool] = False,
):
    """
    This setup is used to create links with the ethernet interface using ifconfig and route.
    It is strictly connected with the host IP because the setup will take it as a new IP for the
    target with light modification.
    up:      It indicates that you want this interface to be active.
    wan:     It sets the default gateway using host parameters
    ip:      It is determinate custom set ip for interface
    dynamic: If True it will set ifconfig with dynamic flag, without netmask and ip
    """

    host_ip, host_netmask = host_interface()

    if ip is None:
        ip_split = re.findall(r"([0-9]{1,3})", host_ip)
        if int(ip_split[3]) >= 100:
            ip_split[3] = "150"
        else:
            ip_split[3] = "1" + ip_split[3]

        desired_target_ip = ".".join(ip_split)
        ip_split[3] = "1"
        default_gw = ".".join(ip_split)

    else:
        desired_target_ip = ip
        ip_split = re.findall(r"([0-9]{1,3})", ip)
        ip_split[3] = "1"
        default_gw = ".".join(ip_split)

    ifconfig_setup_cmd = f"ifconfig en1 192.168.72.151 netmask 255.255.255.0 up"

    if wan:
        route_setup_cmd = f"route add default gw {default_gw} {eth}"
    else:
        route_setup_cmd = f"route add -net {desired_target_ip} {eth}"

    # if up:
    #     ifconfig_setup_cmd += " up"

    if not dynamic:
        psh.assert_prompt_after_cmd(p, ifconfig_setup_cmd, "success")
    else:
        psh.assert_prompt_after_cmd(p, "ifconfig en1 dynamic", "success")

    time.sleep(1)


def con_setup(dut: Dut, host_ip, host_port):
    print("con setup")
    host_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    host_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # docker
    if host_socket.bind(('172.17.0.2', host_port)) is not None:
        print('bind failed')

    # if host_socket.bind(('192.168.72.21', host_port)) is not None:
    #     print('bind failed')

    host_socket.listen()

    dut.sendline('/bin/test-network ' + host_ip + '\n')

    dut.expect('/bin/test-network ' + host_ip)

    ready_to_read, _, _ = select.select([host_socket], [], [], 20)

    if host_socket in ready_to_read:
        peer_socket, peer_address = host_socket.accept()

    host_socket.close()

    print('closed')

    return peer_socket, peer_address


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    assert_re = r"ASSERTION (?P<path>[\S]+):(?P<line>\d+):(?P<status>FAIL|INFO|IGNORE): (?P<msg>.*?)\r"
    result_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>PASS|IGNORE)"
    # Fail need to have its own regex due to greedy matching
    result_fail_re = r"TEST\((?P<group>\w+), (?P<name>\w+)\) (?P<status>FAIL) at (?P<path>.*?):(?P<line>\d+)\r"
    final_re = r"(?P<total>\d+) Tests (?P<fail>\d+) Failures (?P<ignore>\d+) Ignored \r+\n(?P<result>OK|FAIL)"

    last_assertion = {}
    stats = {"FAIL": 0, "IGNORE": 0, "PASS": 0}
    results = []
    timeout_val = 30
    if ctx.nightly:
        timeout_val = 60

    # ipv4_pattern = r'\b(?:\d{1,3}\.){3}\d{1,3}\b'

    # host_ip = re.findall(ipv4_pattern, host_interface()[0])[0]
    en_setup(dut)
    peer_socket, peer_address = con_setup(dut, '192.168.72.21', 1025)
    random_bytes = bytes([random.randint(0, 255) for _ in range(128)])
    peer_socket.send(random_bytes)

    while True:
        idx = dut.expect([assert_re, result_re, result_fail_re, final_re], timeout=timeout_val)
        parsed = dut.match.groupdict()

        if idx == 0:
            if parsed["status"] in ["FAIL", "IGNORE"]:
                last_assertion = parsed
        elif idx in (1, 2):
            if last_assertion:
                parsed["msg"] = last_assertion["msg"]
                last_assertion = {}

            status = Status.from_str(parsed["status"])
            subname = f"{parsed['group']}.{parsed['name']}"
            if "path" in parsed and "line" in parsed:
                parsed["msg"] = f"[{parsed['path']}:{parsed['line']}] " + parsed["msg"]
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

    status = Status.FAIL if stats["FAIL"] != 0 else Status.OK
    return TestResult(status=status)
