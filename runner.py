#!/usr/bin/env python3
import argparse
import dataclasses
import sys
import os
from pathlib import Path
from typing import Dict, List, Tuple, Type

import trunner
from trunner import resolve_project_path
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
    IMX6ULLEvkTarget,
)
from trunner.ctx import TestContext
from trunner.target.base import TargetBase
from trunner.types import is_github_actions


def args_file(arg):
    path = Path(arg).resolve()
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


def parse_args(targets: Dict[str, Type[TargetBase]], hosts: Dict[str, Type[Host]]):
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
        "-S",
        "--stream",
        default=True if is_github_actions() else False,
        action="store_true",
        help="Stream the DUT output during the test execution. Defaults to %(default)s",
    )

    parser.add_argument(
        "-O",
        "--output",
        action="store",
        const="report.xml",
        nargs='?',
        help=("Write machine-readable test results as csv and xml file. "
              "When no value is provided uses %(const)s"),
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
        help="Increate result verbosity level (0={FAIL}, 1={FAIL, SKIP}, 2={FAIL, SKIP, OK})",
    )

    parser.add_argument(
        "--nightly",
        default=False,
        action="store_true",
        help="Nightly tests will be run",
    )

    def is_dir(dirpath):
        """Check whether the provided path is a directory (if exists)"""
        if os.path.exists(dirpath) and not os.path.isdir(dirpath):
            raise argparse.ArgumentTypeError(f"{dirpath} is not a directory!")
        return dirpath

    parser.add_argument(
        "--logdir",
        default=None,
        const="./phoenix_test_campaign_logs",
        help="Directory path where log files from the test campaign will be stored. "
        "By default log files will not be created. "
        "When only --logdir without a value is set, uses %(const)s",
        nargs="?",
        type=is_dir,
    )

    class keyValue(argparse.Action):
        def __call__(self, parser, namespace, values, option_string=None):
            kwargs = getattr(namespace, self.dest)
            if kwargs is None:
                kwargs = dict()

            if values is None:
                parser.error()

            for item in values:
                try:
                    key, val = item.split("=", 1)
                except ValueError:
                    parser.error(f"expected key=value format, got: {item}")

                kwargs[key] = val

            setattr(namespace, self.dest, kwargs)

    parser.add_argument(
        "--kwargs",
        nargs="+",
        default=dict(),
        action=keyValue,
        help=(
            "Pass extra arguments in key=value format to use them in extensions or in harnesses. Caution: do not use"
            " '=' character in the `key` string! Runner just splits the word on the first occurrence of `=` character."
        ),
    )

    args = parser.parse_args()

    if not args.test:
        args.test = [resolve_project_path()]

    if args.output and "." in args.output:
        # remove extension for output stem if possibly exists
        args.output = args.output.rsplit(".", 1)[0]

    return args


def resolve_targets_and_hosts() -> Tuple[Dict[str, Type[TargetBase]], Dict[str, Type[Host]]]:
    """
    Returns a tuple of dictionaries that map host and target names to an equivalent class.

    Load external targets and hosts defined by user in extensions and combine them
    with internal runnner targets.
    """

    targets: List[Type[TargetBase]] = [
        HostPCGenericTarget,
        IA32GenericQemuTarget,
        RISCV64GenericQemuTarget,
        ARMv7A9Zynq7000QemuTarget,
        STM32L4x6Target,
        IMXRT106xEvkTarget,
        IMXRT117xEvkTarget,
        Zynq7000ZedboardTarget,
        IMX6ULLEvkTarget,
    ]

    hosts: List[Type[Host]] = [EmulatorHost, RpiHost]

    try:
        ext_targets, ext_hosts = load_extensions()
    except ExtensionError as e:
        print(e)
        sys.exit(4)

    targets.extend(ext_targets)
    hosts.extend(ext_hosts)

    # NOTE default targets/hosts can be overwritten if extensions have the same name!
    targets_dict = {t.name: t for t in targets}
    hosts_dict = {h.name: h for h in hosts}

    return targets_dict, hosts_dict


def main():
    targets, hosts = resolve_targets_and_hosts()

    args = parse_args(targets, hosts)

    ctx = TestContext(
        port=args.port,
        baudrate=args.baudrate,
        project_path=resolve_project_path(),
        nightly=args.nightly,
        logdir=args.logdir,
        should_flash=not args.no_flash,
        should_test=not args.no_test,
        verbosity=args.verbose,
        stream_output=args.stream,
        output=args.output,
        kwargs=args.kwargs,
    )

    host_cls = hosts[args.host]
    host = host_cls.from_context(ctx)
    ctx = dataclasses.replace(ctx, host=host)

    target_cls = targets[args.target]
    try:
        target = target_cls.from_context(ctx)
    except PortError as e:
        # TODO Make port finding in the global scope
        print(e)
        return 2

    ctx = dataclasses.replace(ctx, target=target)
    ctx = host.add_to_context(ctx)

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
    sys.exit(main())
