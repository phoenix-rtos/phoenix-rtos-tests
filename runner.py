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
                        action='append', choices=config.ALL_TARGETS,
                        help="Filter targets on which test will be built and run. "
                             "By default runs tests on all available targets. "
                             "Flag can be used multiple times.")

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
                        default=config.DEVICE_SERIAL,
                        help="Specify serial to communicate with device board. "
                             "By default uses %(default)s.")

    parser.add_argument("--no-flash",
                        default=False, action='store_true',
                        help="Board will not be flashed by runner.")

    args = parser.parse_args()

    args.log_level = logging_level[args.log_level]

    if not args.test:
        args.test = [config.PHRTOS_TEST_DIR]

    if not args.target:
        # Run on all available targets
        args.target = config.ALL_TARGETS

    config.DEVICE_SERIAL = args.serial

    return args


def print_summary_successed(runner):
    print("Succeeded!")
    if 'armv7m7-imxrt106x' in runner.targets:
        runner.runners['armv7m7-imxrt106x'].led('green', 'on')


def print_summary_failed(runner):
    print("Failed!")
    if 'armv7m7-imxrt106x' in runner.targets:
        runner.runners['armv7m7-imxrt106x'].led('red', 'on')


def main():
    args = parse_args()
    set_logger(args.log_level)

    runner = TestsRunner(targets=args.target,
                         test_paths=args.test,
                         build=args.build,
                         flash=not args.no_flash)

    passed, failed, skipped = runner.run()

    total = passed + failed + skipped
    summary = f'TESTS: {total}'
    summary += f' {Color.colorify("PASSED", Color.OK)}: {passed}'
    summary += f' {Color.colorify("FAILED", Color.FAIL)}: {failed}'
    summary += f' {Color.colorify("SKIPPED", Color.SKIP)}: {skipped}\n'
    logging.info(summary)

    if failed == 0:
        print_summary_successed(runner)
        sys.exit(0)
    else:
        print_summary_failed(runner)
        sys.exit(1)


if __name__ == "__main__":
    main()
