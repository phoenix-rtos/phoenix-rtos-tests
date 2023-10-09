# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# ping command test
#
# Copyright 2023 Phoenix Systems
# Author: Damian Modzelewski
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh
import psh.tools.netset as netset
import time

GOOGLE_DNS = "8.8.8.8"
IP_LOOPBACK = "127.0.0.1"


def ping_output_regex(target, iteration):
    iter_str = r"{" + str(iteration) + r"}"

    psh_output_regex = r"(([0-9]+[a-z\s]+ "
    psh_output_regex += str(target)
    psh_output_regex += r": ttl=[0-9]+ icmp_seq=[0-9]+ time=([0-9.]+) ms)"
    psh_output_regex += f"{psh.EOL}){iter_str}"

    return psh_output_regex


def psh_ping_en_disabled(p):
    # wait for last lwip log
    time.sleep(5)

    psh.assert_cmd(
        p, f"ping -i 10 {GOOGLE_DNS}", expected="ping: Fail to send a packet!", result="fail", is_regex=False
    )

    # self ping i flag used to speed up test process
    psh.assert_cmd(
        p, f"ping -i 10 {IP_LOOPBACK}", expected=ping_output_regex(IP_LOOPBACK, 5), result="success", is_regex=True
    )


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


def psh_ping(p):
    # self ping
    psh.assert_cmd(
        p, f"ping -i 10 {IP_LOOPBACK}", expected=ping_output_regex(IP_LOOPBACK, 5), result="success", is_regex=True
    )

    # find TTL route
    for i in range(10, 50, 10):
        counter = 0
        for x in range(0, 5):
            psh.assert_cmd(
                p,
                f"ping -t {i} -i 10 192.168.72.{i+x}",
                expected=ping_output_regex(f"192.168.72.{i+x}", 5) + "|Host timeout" + psh.EOL,
                result="dont-check",
                is_regex=True,
            )

            if psh.get_exit_code(p) == 0:
                counter += 1
                print(counter)
        assert counter >= 2


def psh_ping_errors(p):
    # invalid ping target
    ping_target = ""
    psh.assert_cmd(p, f"ping -c 4 {ping_target}", expected="ping: Expected address!", result="fail", is_regex=False)

    # invalid ip
    ping_target = "localhost"
    psh.assert_cmd(p, f"ping {ping_target}", expected="ping: Invalid IP address!", result="fail", is_regex=False)

    # invalid ping count
    psh.assert_cmd(p, f"ping -c -1 {GOOGLE_DNS}", expected="ping: Wrong count value!", result="fail", is_regex=False)

    # invalid ttl value
    psh.assert_cmd(p, f"ping -t -1 {GOOGLE_DNS}", expected="ping: Wrong ttl value!", result="fail", is_regex=False)

    # invalid interval value
    psh.assert_cmd(p, f"ping -i -1 {GOOGLE_DNS}", expected="ping: Wrong interval value!", result="fail", is_regex=False)

    # invalid timeout
    psh.assert_cmd(p, f"ping -W -1 {GOOGLE_DNS}", expected="ping: Wrong timeout value!", result="fail", is_regex=False)

    # invlid_payload_len
    psh.assert_cmd(p, f"ping -s 2041 {GOOGLE_DNS}", expected="ping: Wrong payload len", result="fail", is_regex=False)


@psh.run
def harness(p):
    psh_ping_en_disabled(p)

    netset.en_setup(p, eth="en1")

    psh_ping_help(p)
    psh_ping(p)
    psh_ping_errors(p)
