# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# Useful functions connected with ethernet setup
#
# Copyright 2023, 2024 Phoenix Systems
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
import time
from typing import Tuple


def ip_table() -> Tuple[str, ...]:
    return (
        "192.168.72.20",
        "192.168.72.21",
        "192.168.72.120",
        "192.168.72.121",
    )


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

    ifconfig_setup_cmd = f"ifconfig {eth} {desired_target_ip} {host_netmask} "

    if wan:
        route_setup_cmd = f"route add default gw {default_gw} {eth}"
    else:
        route_setup_cmd = f"route add -net {desired_target_ip} {eth}"

    if up:
        ifconfig_setup_cmd += "up "

    if not dynamic:
        psh.assert_prompt_after_cmd(p, ifconfig_setup_cmd, "success")
        psh.assert_prompt_after_cmd(p, route_setup_cmd, "success")
    else:
        psh.assert_prompt_after_cmd(p, "ifconfig en1 dynamic", "success")

    time.sleep(1)


def pppd_setup(
    p,
    baudrate: Optional[int] = 460800,
    port: Optional[str] = "/dev/ttyUSB0",
    host_ip: Optional[str] = "10.0.0.1",
    target_ip: Optional[str] = "10.0.0.2",
    interface_idle_time: Optional[int] = "10",
    timeout: Optional[int] = 10,
):
    """
    This setup is used to create the connection from host to target with enabled pppou (Point-to-Point over UART).
    To create a link is needed to connect the device with a host via UART converter.
    NOTE: to use pppd without sudo privilege is needed to grant permissions to it via
    "sudo chmod u+s,+x /usr/sbin/pppd"
    and change in "/etc/ppp/options" (auth) to (noauth)
    Sometimes is good to disable on host target ModemManager:
    "sudo systemctl disable ModemManager.service && sudo systemctl stop ModemManager.service"

    Port:                Port located in /dev pointing on to UART converter
    host_ip:             Desired host IP
    target_ip:           Desired target IP
    interface_idle_time: Time that after created ppp interface will be destroyed
    timeout:             Time that indicates how long it will be searching for ppp interface
    """

    host_target_ip = f"{host_ip}:{target_ip}"
    idle_value = f"idle {interface_idle_time}"
    subprocess.Popen(
        f"/sbin/pppd {port} {baudrate} {host_target_ip} lock local debug nocrtscts defaultroute maxfail 0 {idle_value}",
        shell=True,
        cwd=".",
        start_new_session=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        text=True,
    )

    asleep = 0
    ppd_devices = []

    while not ppd_devices:
        time.sleep(0.01)
        asleep += 0.01
        out = subprocess.run(
            "ifconfig",
            encoding="ascii",
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=True,
        )
        ppd_devices = re.findall("ppp[0-9]:.+\n.+[0-9]+", out.stdout)

        if timeout and asleep >= timeout:
            raise TimeoutError("Couldn't find any ppp interface")


@psh.run
def harness(p):
    en_setup(p)
    pppd_setup(p)
