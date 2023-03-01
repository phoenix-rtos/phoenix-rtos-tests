#!/usr/bin/env python3
import argparse
import sys
import os
from pathlib import Path
from typing import Dict, Tuple

import trunner
from trunner.extensions import ExtensionError, load_extensions
from trunner.host import EmulatorHost, Host, RpiHost
from trunner import TestRunner
from trunner.dut import PortError
from trunner.target import (
    IMXRT106xEvkTarget,
    IMXRT117xEvkTarget,
    HostPCGenericTarget,
    IA32GenericQemuTarget,
    RISCV64GenericQemuTarget,
    ARMv7A9Zynq7000QemuTarget,
    STM32L4x6Target,
    Zynq7000ZedboardTarget,
)
from trunner.config import TestContext
from trunner.target.base import TargetBase


def args_file(arg):
    path = Path(arg)
    if not path.exists():
        print(f"Path {path} does not exist")
        sys.exit(3)

    path = path.resolve()
    return path


def is_raspberry_pi() -> bool:
    if os.name != "posix":
        return False

    try:
        with open("/sys/firmware/devicetree/base/model", "r") as f:
            model = f.read().lower()
            return "raspberry pi" in model
    except Exception:
        pass

    return False


def parse_args(targets: Dict[str, TargetBase], hosts: Dict[str, Host]):
    """Parses and returns program arguments.

    Arguments:
        targets: Dictionary of available targets.
        hosts: Dictionary of available hosts.

    """
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "-T",
        "--target",
        default="ia32-generic-qemu",
        choices=targets.keys(),
        help=(
            "Set target on which tests will be run. "
            "By default runs tests on %(default)s target. "
            "Targets can be extended using extension feature."
        ),
    )

    parser.add_argument(
        "-H",
        "--host",
        default="rpi" if is_raspberry_pi() else "pc",
        choices=hosts.keys(),
        help=(
            'Set host that run tests. "pc" host is recommended for running tests on emulators. '
            'To control physical boards you can use "rpi" host or pc '
            "(but it requires restarting board manually by yourself). By default runs tests on %(default)s. "
            "Hosts can be extended using extension feature."
        ),
    )

    parser.add_argument(
        "-t",
        "--test",
        default=[],
        action="append",
        type=args_file,
        help=(
            "Specify directory in which test will be searched. "
            "If flag is not used then runner searches for tests in "
            "phoenix-rtos-project directory. Flag can be used multiple times."
        ),
    )

    parser.add_argument(
        "-p",
        "--port",
        help="Specify serial to communicate with device board. Default value depends on the target",
    )

    parser.add_argument(
        "-b",
        "--baudrate",
        default=115200,
        help="Specify the connection speed of serial. By default uses %(default)d",
    )

    parser.add_argument(
        "--no-flash",
        default=False,
        action="store_true",
        help="Board will not be flashed by runner.",
    )

    parser.add_argument(
        "--no-test",
        default=False,
        action="store_true",
        help=(
            "Tests will be not run by runner. This option may be helpful to only flash the device "
            "or check if yamls are parsed correctly."
        ),
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Increases verbosity level",
    )

    parser.add_argument(
        "--nightly",
        default=False,
        action="store_true",
        help="Nightly tests will be run",
    )

    args = parser.parse_args()

    if not args.test:
        args.test = [resolve_project_path()]

    return args


def resolve_project_path():
    file_path = Path(__file__).resolve()
    # file_path is phoenix-rtos-project/phoenix-rtos-tests/runner.py
    project_dir = file_path.parent.parent
    return project_dir


def resolve_targets_and_hosts() -> Tuple[Dict[str, TargetBase], Dict[str, Host]]:
    """
    Returns a tuple of dictionaries that map host and target names to an equivalent class.

    Load external targets and hosts defined by user in extensions and combine them
    with internal runnner targets.
    """

    targets = [
        HostPCGenericTarget,
        IA32GenericQemuTarget,
        RISCV64GenericQemuTarget,
        ARMv7A9Zynq7000QemuTarget,
        STM32L4x6Target,
        IMXRT106xEvkTarget,
        IMXRT117xEvkTarget,
        Zynq7000ZedboardTarget,
    ]

    hosts = [EmulatorHost, RpiHost]

    try:
        ext_targets, ext_hosts = load_extensions()
    except ExtensionError as e:
        print(e)
        sys.exit(4)

    targets.extend(ext_targets)
    hosts.extend(ext_hosts)

    # NOTE default targets/hosts can be overwritten if extensions have the same name!
    targets = {t.name: t for t in targets}
    hosts = {h.name: h for h in hosts}

    return targets, hosts


def main():
    targets, hosts = resolve_targets_and_hosts()

    args = parse_args(targets, hosts)
    host_cls = hosts[args.host]
    host = host_cls()

    # Create partial context for target initialization
    ctx = TestContext(
        target=None,
        host=host,
        port=args.port,
        baudrate=args.baudrate,
        project_path=resolve_project_path(),
        nightly=args.nightly,
        should_flash=not args.no_flash,
        should_test=not args.no_test,
        verbosity=args.verbose,
    )

    try:
        target_cls = targets[args.target]
        target = target_cls.from_context(ctx)
    except PortError as e:
        # TODO Make port finding in the global scope
        print(e)
        return 2

    ctx = TestContext(
        target=target,
        host=ctx.host,
        port=ctx.port,
        baudrate=ctx.baudrate,
        project_path=ctx.project_path,
        nightly=ctx.nightly,
        should_flash=ctx.should_flash,
        should_test=ctx.should_test,
        verbosity=ctx.verbosity,
    )

    # Set global context
    trunner.ctx = ctx

    runner = TestRunner(
        ctx=ctx,
        test_paths=args.test,
    )

    ok = runner.run()
    if not ok:
        return 1

    return 0


if __name__ == "__main__":
    res = main()
    if res != 0:
        sys.exit(res)
