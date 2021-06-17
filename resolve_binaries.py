#! /usr/bin/env python3

import argparse

from trunner.test_runner import TestsRunner
from trunner.config import Config, ConfigParser, TestCaseConfig, TestConfig, PHRTOS_TEST_DIR, \
    ALL_TARGETS


def parse_args():
    parser = argparse.ArgumentParser(
        description="Program prints binaries defined in the yamls configs accepted by the runner. "
                    "If the target flag is used, then configs are constricted to targets specified "
                    "by a user."
    )

    parser.add_argument("-T", "--target",
                        action='append',
                        choices=ALL_TARGETS,
                        help="Filters binaries by target. Flag can be used multiple times.")

    args = parser.parse_args()
    if not args.target:
        args.target = list(ALL_TARGETS)

    return args


def resolve_test_bins(targets):
    runner = TestsRunner(
        targets=targets,
        test_paths=[PHRTOS_TEST_DIR],
        build=False,
        flash=False
    )

    runner.search_for_tests()
    execs = set()
    parser = ConfigParser()

    for path in runner.test_paths:
        config = TestCaseConfig.load_yaml(path)
        main, tests = TestCaseConfig.extract_components(config)
        main = Config.from_dict(main)
        for test in tests:
            test = TestConfig(test)
            parser.parse(test)
            test.join_targets(main)
            test.setdefault_targets()
            parser.resolve_targets(test, targets)

            if not test['targets']['value']:
                # test is not for our target
                continue

            exec_cmd = test.get('exec') or main.get('exec')
            if exec_cmd:
                execs.add(exec_cmd[0])

    return list(execs)


def main():
    args = parse_args()
    execs = resolve_test_bins(args.target)

    for binary in execs:
        print(binary)


if __name__ == "__main__":
    main()
