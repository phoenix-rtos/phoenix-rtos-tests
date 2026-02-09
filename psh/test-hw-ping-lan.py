# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# ping command test for hardware
#
# Copyright 2023, 2024 Phoenix Systems
# Author: Damian Modzelewski
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh
import psh.tools.netset as netset
import trunner.ctx as ctx

GOOGLE_DNS = "8.8.8.8"
IP_LOOPBACK = "127.0.0.1"
PPPOU_HOST_IP = "10.0.0.1"
PPPOU_TARGET_IP = "10.0.0.2"


def ping_output_regex(target, iteration):
    iter_str = r"{" + str(iteration) + r"}"

    psh_output_regex = (
        r"PING [()0-9a-z-.: ]+ data bytes"
        f"{psh.EOL}"
        r"(([0-9]+[a-z\s]+ "
        f"{str(target)}"
        r": ttl=[0-9]+ icmp_seq=[0-9]+ time=([0-9.]+) ms)"
        f"{psh.EOL})"
    )
    psh_output_regex += iter_str

    return psh_output_regex


def ping_error_regex(error_msg):
    return r"PING [()0-9a-z.: ]+ data bytes" + psh.EOL + error_msg + psh.EOL


def psh_ping_help(p):
    help = (
        "Usage: ping [options] address",
        "Options",
        "  -h:  prints help",
        "  -c:  count, number of requests to be sent, default 5",
        "  -i:  interval in milliseconds, default 1000",
        "  -t:  IP Time To Live, default 64",
        "  -s:  payload size, default 56, maximum 2040",
        "  -W:  socket timetout, default 2000",
        "",
    )
    psh.assert_cmd(p, "ping -h", expected=help, result="success", is_regex=False)


def psh_ping_errors(p):
    # invalid ping target
    ping_target = ""
    psh.assert_cmd(
        p,
        f"ping -c 4 -W 200 {ping_target}",
        expected="ping: Expected address!",
        result="fail",
        is_regex=False,
    )

    # invalid ping count
    psh.assert_cmd(
        p,
        f"ping -c -W 200 {GOOGLE_DNS}",
        expected="ping: Wrong count value!",
        result="fail",
        is_regex=False,
    )

    # invalid ttl value
    psh.assert_cmd(
        p,
        f"ping -t -W 200 {GOOGLE_DNS}",
        expected="ping: Wrong ttl value!",
        result="fail",
        is_regex=False,
    )

    # invalid interval value
    psh.assert_cmd(
        p,
        f"ping -i -W 200 {GOOGLE_DNS}",
        expected="ping: Wrong interval value!",
        result="fail",
        is_regex=False,
    )

    # invalid timeout
    psh.assert_cmd(
        p,
        f"ping -W {GOOGLE_DNS}",
        expected="ping: Wrong timeout value!",
        result="fail",
        is_regex=False,
    )

    # invlid_payload_len
    psh.assert_cmd(
        p,
        f"ping -s 2041 -W 200 {GOOGLE_DNS}",
        expected="ping: Wrong payload len",
        result="fail",
        is_regex=False,
    )


def psh_ping_en_disabled(p):
    # for all possible case of use ping command i and W flag are used to speed up test process
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {GOOGLE_DNS}",
        expected=ping_error_regex("ping: Fail to send a packet!"),
        result="fail",
        is_regex=True,
    )

    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {IP_LOOPBACK}",
        expected=ping_output_regex(IP_LOOPBACK, 5),
        result="success",
        is_regex=True,
    )


def psh_direct_target_ping(p):
    counter = 0
    # right now it uses only targets defined in ip_table which are same targets as before + an interface of tested evk
    avaliable_targets = netset.ip_table()
    for x in avaliable_targets:
        psh.assert_cmd(
            p,
            f"ping -i 10 -W 200 {x}",
            expected=ping_output_regex(x, 5) + "|Host timeout" + psh.EOL,
            result="dont-check",
            is_regex=True,
        )
        if psh.get_exit_code(p) == 0:
            counter += 1
    assert counter >= 2


def psh_ping(p):
    # self ping
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {IP_LOOPBACK}",
        expected=ping_output_regex(IP_LOOPBACK, 5),
        result="success",
        is_regex=True,
    )

    # find TTL route
    for i in range(10, 50, 10):
        counter = 0
        for x in range(0, 5):
            psh.assert_cmd(
                p,
                f"ping -t {i} -i 10 -W 200 192.168.72.{i+x}",
                expected=ping_output_regex(f"192.168.72.{i+x}", 5)
                + "|"
                + ping_error_regex("Host timeout"),
                result="dont-check",
                is_regex=True,
            )

            if psh.get_exit_code(p) == 0:
                counter += 1
        assert counter >= 2

    # using "localhost" as desired target to check address resolver
    ping_target = "localhost"
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {ping_target}",
        expected=ping_output_regex(IP_LOOPBACK, 5),
        result="success",
        is_regex=True,
    )


def pppou_ping(p):
    # self ping
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {IP_LOOPBACK}",
        expected=ping_output_regex(IP_LOOPBACK, 5),
        result="success",
        is_regex=True,
    )

    # using "localhost" as desired target to check address resolver
    ping_target = "localhost"
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {ping_target}",
        expected=ping_output_regex(IP_LOOPBACK, 5),
        result="success",
        is_regex=True,
    )

    # self ping on pp1 interface
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {PPPOU_TARGET_IP}",
        expected=ping_output_regex(PPPOU_TARGET_IP, 5),
        result="success",
        is_regex=True,
    )

    # ping on ppp provider
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {PPPOU_HOST_IP}",
        expected=ping_output_regex(PPPOU_HOST_IP, 5),
        result="success",
        is_regex=True,
    )

    # long ping on ppp provider
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 -c 200 {PPPOU_HOST_IP}",
        expected=ping_output_regex(PPPOU_HOST_IP, 200),
        result="success",
        is_regex=True,
    )


@psh.run
def harness(p):
    if ctx.target.rootfs:
        psh_ping_help(p)
        psh_ping_errors(p)
        psh_ping_en_disabled(p)
        netset.en_setup(p, eth="en1")
        psh_direct_target_ping(p)
        psh_ping(p)
    else:
        psh_ping_help(p)
        netset.pppd_setup(p)
        psh_ping_errors(p)
        pppou_ping(p)
