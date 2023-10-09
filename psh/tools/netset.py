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

from typing import Optional
import psh.tools.psh as psh
import subprocess
import re


def host_interface():
    out = subprocess.run("ifconfig", encoding="ascii", stdout=subprocess.PIPE, check=True)
    eth_device = re.findall("eth[0-9]:.+\n.+[0-9]+", out.stdout)
    inet = re.findall("inet [0-9.]+", str(eth_device))
    netmask = re.findall("netmask [0-9.]+", str(eth_device))
    host_setup = inet + netmask
    return host_setup


def en_setup(p, eth="en1", up: Optional[bool] = True, wan: Optional[bool] = False):
    host_setup = host_interface()
    host_ip = host_setup[0]
    host_netmask = host_setup[1]

    ip_split = re.findall("([0-9]{1,3}.)", host_ip)
    ip_split[3] = "1" + ip_split[3]
    desired_target_ip = "".join(ip_split)
    default_gw = desired_target_ip[: len(desired_target_ip) - 2]

    ifconfig_setup_cmd = f"./sbin/ifconfig {eth} {desired_target_ip} {host_netmask} "

    if wan:
        route_setup_cmd = f"./sbin/route add default gw {default_gw} {eth}"
    else:
        route_setup_cmd = f"./sbin/route add -net {desired_target_ip} {eth}"

    if up:
        ifconfig_setup_cmd += "up "

    psh.assert_prompt_after_cmd(p, ifconfig_setup_cmd, "success")
    psh.assert_prompt_after_cmd(p, route_setup_cmd, "success")


@psh.run
def harness(p):
    en_setup(p)
