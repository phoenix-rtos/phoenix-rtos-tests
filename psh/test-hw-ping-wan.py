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
from typing import Optional

CLOUD_FLARE_DNS = "1.1.1.1"
GOOGLE_DNS = "8.8.8.8"
IP_LOOPBACK = "127.0.0.1"


def ping_output_regex(target, iteration, possible_fail: Optional[bool] = False):
    iter_str = r"{" + str(iteration) + r"}"

    psh_output_regex = (
        r"PING [()0-9a-z-.: ]+ data bytes"
        f"{psh.EOL}"
        r"(([0-9]+[a-z\s]+ "
        f"{str(target)}"
        r": ttl=[0-9]+ icmp_seq=[0-9]+ time=([0-9.]+) ms)"
        f"{psh.EOL})"
    )
    if possible_fail:
        psh_output_regex += r"|Host timeout"
    psh_output_regex += iter_str

    return psh_output_regex


def ping_error_regex(error_msg):
    return r"PING [()0-9a-z.: ]+ data bytes" + psh.EOL + error_msg + psh.EOL


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
    counter = 0

    # self ping
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {IP_LOOPBACK}",
        expected=ping_output_regex(IP_LOOPBACK, 5),
        result="success",
        is_regex=True,
    )

    # google dns ping after linuxrc run (success)
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200 {GOOGLE_DNS}",
        expected=ping_output_regex(GOOGLE_DNS, 5),
        result="success",
        is_regex=True,
    )

    # find TTL route
    for i in range(1, 10):
        psh.assert_cmd(
            p,
            f"ping -t {i} -i 10 -W 200 {GOOGLE_DNS}",
            expected=ping_output_regex(GOOGLE_DNS, 5) + "|Host timeout" + psh.EOL,
            result="dont-check",
            is_regex=True,
        )

        if psh.get_exit_code(p) == 0:
            counter += 1

    assert counter > 0

    # multiple iteration ping
    psh.assert_cmd(
        p,
        f"ping -c 200 -i 100 -W 200 {CLOUD_FLARE_DNS}",
        expected=ping_output_regex(CLOUD_FLARE_DNS, 200),
        result="success",
        is_regex=True,
    )

    # ping with long time to respond (IBM cloud name server).
    ping_target = "67.228.254.4"
    psh.assert_cmd(
        p,
        f"ping -i 10 -W 200  {ping_target}",
        expected=ping_output_regex(ping_target, 5, possible_fail=True),
        result="success",
        is_regex=True,
    )

    # ping max payload (truncated to 1480)
    psh.assert_cmd(
        p,
        f"ping -s 2000 -i 10 -W 200 {ping_target}",
        expected=ping_output_regex(ping_target, 5),
        result="success",
        is_regex=True,
    )

    # ping min payload
    psh.assert_cmd(
        p,
        f"ping -i 200 {ping_target}",
        expected=ping_output_regex(ping_target, 5),
        result="success",
        is_regex=True,
    )

    # ping max payload (Cloudflare DNS capable to hold payload)
    ping_target = "1.0.0.1"
    psh.assert_cmd(
        p,
        f"ping -s 2000 -i 10 -W 200 {ping_target}",
        expected=ping_output_regex(ping_target, 5),
        result="success",
        is_regex=True,
    )


def psh_ping_name_resolve(p):
    counter = 0

    ping_target = [
        "google.com",
        "controld.com",
        "quad9.net",
        "opendns.com",
        "alternate-dns.com",
        "adguard-dns.io",
        "publicdns.xyz",
        "iana.org",
        "dns.watch",
    ]

    for x in ping_target:
        psh.assert_cmd(
            p,
            f"ping -W 8000 -i 1000 {str(x)}",
            expected=ping_output_regex(r"[0-9.]+", 5),
            result="dont-check",
            is_regex=True,
        )

        if psh.get_exit_code(p) == 0:
            counter += 1

    assert counter > 5


@psh.run
def harness(p):
    psh_ping_en_disabled(p)

    netset.en_setup(p, eth="en1", wan=True)

    psh_ping_help(p)
    psh_ping(p)

    netset.en_setup(p, eth="en1", dynamic=True)

    psh_ping_name_resolve(p)
