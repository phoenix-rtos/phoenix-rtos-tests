#!/usr/bin/env python3

import argparse
import logging
import pathlib
import sys

import trunner.config as config

from trunner.test_runner import TestsRunner
from trunner.tools.color import Color


def set_logger(level=logging.INFO):
    root = logging.getLogger()
    root.setLevel(logging.DEBUG)

    stream_handler = logging.StreamHandler(sys.stdout)
    stream_handler.setLevel(level)
    stream_handler.terminator = ''
    formatter = logging.Formatter('%(message)s')
    stream_handler.setFormatter(formatter)
    root.addHandler(stream_handler)


def args_file(arg):
    path = pathlib.Path(arg)
    if not path.exists():
        print(f"Path {path} does not exist")
        sys.exit(1)

    path = path.resolve()
    return path


def parse_args():
    logging_level = {
            'debug': logging.DEBUG,
            'info': logging.INFO,
            'warning': logging.WARNING,
            'error': logging.ERROR
    }

    parser = argparse.ArgumentParser()

    parser.add_argument("-T", "--target",
                        default='ia32-generic-qemu',
                        choices=config.ALL_TARGETS,
                        help="Set target on which tests will be run. "
                             "By default runs tests on %(default)s target. ")

    parser.add_argument("-t", "--test",
                        default=[], action='append', type=args_file,
                        help="Specify directory in which test will be searched. "
                             "If flag is not used then runner searches for tests in "
                             "phoenix-rtos-tests directory. Flag can be used multiple times.")

    parser.add_argument("--build",
                        default=False, action='store_true',
                        help="Runner will build all tests.")

    parser.add_argument("-l", "--log-level",
                        default='info',
                        choices=logging_level,
                        help="Specify verbosity level. By default uses level info.")

    parser.add_argument("-s", "--serial",
                        help="Specify serial to communicate with device board. "
                             "Default value depends on the target")

    parser.add_argument("-b", "--baudrate",
                        default=config.DEVICE_SERIAL_BAUDRATE,
                        help="Specify the connection speed of serial. "
                             "By default uses 115200")

    parser.add_argument("--no-flash",
                        default=False, action='store_true',
                        help="Board will not be flashed by runner.")

    # Add alternative option "--long-test" "" for long-test argument, which is False
    # Needed for proper disabling long tests in gh actions
    # because of problem with passing empty argument through gh workflows
    parser.add_argument("--long-test",
                        type=bool,
                        nargs='?', default=False, const=True,
                        help="Long tests will be run")

    args = parser.parse_args()

    args.log_level = logging_level[args.log_level]

    if not args.test:
        args.test = [config.PHRTOS_TEST_DIR]

    if not args.serial:
        if args.target == 'armv7a9-zynq7000-zedboard':
            args.serial = config.DEVICE_SERIAL_PORT_XYLINX
        else:
            args.serial = config.DEVICE_SERIAL_PORT_NXP

    if not args.baudrate:
        args.baudrate = config.DEVICE_SERIAL_BAUDRATE

    return args


def main():
    args = parse_args()
    set_logger(args.log_level)

    runner = TestsRunner(target=args.target,
                         test_paths=args.test,
                         build=args.build,
                         flash=not args.no_flash,
                         serial=(args.serial, args.baudrate),
                         log=(args.log_level == logging.DEBUG),
                         long_test=args.long_test
                         )

    passed, failed, skipped = runner.run()

    total = passed + failed + skipped
    summary = f'TESTS: {total}'
    summary += f' {Color.colorify("PASSED", Color.OK)}: {passed}'
    summary += f' {Color.colorify("FAILED", Color.FAIL)}: {failed}'
    summary += f' {Color.colorify("SKIPPED", Color.SKIP)}: {skipped}\n'
    logging.info(summary)

    if failed == 0:
        print("Succeeded!")
        sys.exit(0)
    else:
        print("Failed!")
        sys.exit(1)


if __name__ == "__main__":
    main()
