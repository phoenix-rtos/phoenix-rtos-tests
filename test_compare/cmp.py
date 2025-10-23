#!/usr/bin/env python3

import argparse
import sys
from dataclasses import dataclass, field
from enum import Enum, IntEnum

import yaml
from junitparser import Error, Failure, JUnitXml, Skipped

ESCAPE_RED = "\033[31m"
ESCAPE_YELLOW = "\033[33m"
ESCAPE_GREEN = "\033[32m"
ESCAPE_GRAY = "\033[90m"
ESCAPE_BOLD = "\033[1m"
ESCAPE_ITALIC = "\033[3m"
ESCAPE_UNDERLINE = "\033[4m"
ESCAPE_RESET = "\033[0m"
INF = float("inf")
OK = f"{ESCAPE_GREEN}{'OK':^9}{ESCAPE_RESET}"
SKIP = f"{ESCAPE_GRAY}{'SKIP':^9}{ESCAPE_RESET}"
FAIL = f"{ESCAPE_RED}{'FAIL':^9}{ESCAPE_RESET}"
PRESENT = f"{ESCAPE_GREEN}{'PRESENT':^9}{ESCAPE_RESET}"
MISSING = f"{ESCAPE_GRAY}{'MISSING':^9}{ESCAPE_RESET}"


class ChangeDirection(Enum):
    INCREASE = 1
    DECREASE = -1
    UNCHANGED = 0
    MISSING = 2


class Level(IntEnum):
    TARGET = 0
    LOCATION = 1
    SUITE = 2
    CASE = 3

    def __add__(self, other):
        return Level(self.value + other)


class Status(Enum):
    OK = (ESCAPE_GREEN, "OK")
    FAIL = (ESCAPE_RED, "FAIL")
    SKIP = (ESCAPE_GRAY, "SKIP")
    PRESENT = (ESCAPE_GREEN, "PRESENT")
    MISSING = (ESCAPE_GRAY, "MISSING")
    NONE = ("", "--")

    def __init__(self, color, label):
        self._color = color
        self._label = label

    def format(self):
        return f"{self._color}{self._label:^9}{ESCAPE_RESET}"


@dataclass
class Args:
    file_old: {}
    file_new: {}
    verbose: int
    benchmarks: {}
    targets: []
    locations: []
    suites: []
    cases: []
    threshold_absolute: float
    threshold_relative: float
    threshold_filter: bool
    status_diff: bool
    show_fails: bool


@dataclass
class Testsuite:
    target: str
    location: str
    name: str
    cases: {} = field(default_factory=dict)


@dataclass
class Testcase:
    name: str
    time: float
    status: Status


@dataclass
class ComparedNode:
    name: str
    time_old: float
    time_new: float


@dataclass
class ComparedCaseNode(ComparedNode):
    status_old: Status
    status_new: Status
    difference: float = None
    percentage: float = None

    @classmethod
    def from_cases(cls, case1, case2):
        case_cmp = cls(
            name=case1.name if case1 else case2.name,
            time_old=case1.time if case1 else None,
            status_old=case1.status if case1 else Status.NONE,
            time_new=case2.time if case2 else None,
            status_new=case2.status if case2 else Status.NONE,
        )
        case_cmp.difference, case_cmp.percentage = (
            difference_and_percentage(case_cmp.time_old, case_cmp.time_new) if case1 and case2 else (None, None)
        )
        return case_cmp


@dataclass
class ComparedAgregateNode(ComparedNode):
    children: [] = field(default_factory=list)


@dataclass
class ComparedContainerNode(ComparedAgregateNode):
    only_old: [] = field(default_factory=list)
    only_new: [] = field(default_factory=list)


@dataclass
class ComparedStatusNode:
    name: str
    status_old: Status
    status_new: Status
    children: [] = field(default_factory=list)


@dataclass
class ComparedSuiteNode(ComparedAgregateNode):
    single_case: bool = False


@dataclass
class TimeRow:
    style: str = ""
    separator: str = ""
    name: str = ""
    time_old: str = ""
    time_new: str = ""
    color: str = ""
    difference: str = ""
    percentage: str = ""

    def __init__(self, data, style, separator, prefix, args):
        difference, percentage = difference_and_percentage(data.time_old, data.time_new)
        color = CHANGE_COLORS[change_dir(difference, percentage, args)]
        self.style = style
        self.separator = separator
        self.name = f"{prefix}{data.name}"
        self.time_old = f"{data.time_old:.3f}"
        self.time_new = f"{data.time_new:.3f}"
        self.color = color
        self.difference = f"{difference:+.3f}"
        self.percentage = f"{percentage:+.2f}" if percentage < 10000 else "-.--"

    def name_width(self):
        return len(self.name)

    def print(self, max_name_len):
        print(
            f"{self.style}{self.name:<{max_name_len}}{ESCAPE_RESET}{self.separator}"
            f"{self.style}{self.time_old:>8}s{ESCAPE_RESET}{self.separator}"
            f"{self.style}{self.time_new:>8}s{ESCAPE_RESET}{self.separator}"
            f"{self.style}{self.color}{self.difference:>9}s{ESCAPE_RESET}{self.separator}"
            f"{self.style}{self.color}{self.percentage:>8}%{ESCAPE_RESET}"
        )


@dataclass
class StatusRow:
    style: str = ""
    separator: str = ""
    name: str = ""
    status_old: str = ""
    status_new: str = ""

    def name_width(self):
        return len(self.name)

    def print(self, max_name_len):
        print(
            f"{self.style}{self.name:<{max_name_len}}{ESCAPE_RESET}{self.separator}"
            f"{self.style}{self.status_old.format()}{self.separator}"
            f"{self.style}{self.status_new.format()}{self.separator}"
        )


class StatusRowHeader:
    @staticmethod
    def name_width():
        return len("NAME")

    @staticmethod
    def print(max_name_len):
        print(f"{ESCAPE_BOLD}{'':>{max_name_len}}║{'OLD':^9}║{'NEW':^9}║{ESCAPE_RESET}")
        print(f"{ESCAPE_BOLD}{ESCAPE_UNDERLINE}{'NAME':^{max_name_len}}║{'STATUS':^9}║{'STATUS':^9}║{ESCAPE_RESET}")


class TimeRowHeader:
    @staticmethod
    def name_width():
        return len("NAME")

    @staticmethod
    def print(max_name_len):
        print(f"{ESCAPE_BOLD}{'':>{max_name_len}}║{'OLD':^9}║{'NEW':^9}║{'DIFFERENCE':^20}{ESCAPE_RESET}")
        print(
            f"{ESCAPE_BOLD}{ESCAPE_UNDERLINE}{'NAME':^{max_name_len}}║"
            f"{'TIME':^9}║{'TIME':^9}║{'TIME':^10}║{'PERCENTAGE':^9}{ESCAPE_RESET}"
        )


class EmptyRow:
    @staticmethod
    def name_width():
        return 0

    @staticmethod
    def print(_):
        print()


def parse_args():
    parser = argparse.ArgumentParser(
        description="Compares two test result xml files",
        epilog="Example: python3 cmp.py old_results.xml new_results.xml -v --threshold 5.0 -t ia32-generic-qemu",
    )

    parser.add_argument("file_old", metavar="FILE_OLD", help="Path to the old test results file.")
    parser.add_argument("file_new", metavar="FILE_NEW", help="Path to the new test results file.")

    parser.add_argument(
        "-b",
        "--benchmark",
        dest="benchmark_files",
        action="append",
        default=[],
        metavar="FILE",
        help="Only compare tests specified in benchmark file. Can be specified multiple times.",
    )
    parser.add_argument(
        "-t",
        "--target",
        dest="targets",
        action="append",
        default=[],
        metavar="TARGET",
        help="Filter by target. Can be specified multiple times.",
    )
    parser.add_argument(
        "-d",
        "--directory",
        dest="locations",
        action="append",
        default=[],
        metavar="DIR",
        help="Filter by directory. Can be specified multiple times.",
    )
    parser.add_argument(
        "-s",
        "--suite",
        dest="suites",
        action="append",
        default=[],
        metavar="SUITE",
        help="Filter by suite. Can be specified multiple times.",
    )
    parser.add_argument(
        "-c",
        "--case",
        dest="cases",
        action="append",
        default=[],
        metavar="TESTCASE",
        help="Filter by case. Can be specified multiple times.",
    )

    parser.add_argument(
        "--threshold",
        dest="threshold_relative",
        type=float,
        default=10.0,
        metavar="PERCENT",
        help="Set relative difference threshold (default: %(default)s%%). When set explicitly, enables filtering.",
    )
    parser.add_argument(
        "--threshold-absolute",
        type=float,
        default=0.1,
        metavar="VALUE",
        help="Set absolute difference threshold (default: %(default)s). When set explicitly, enables filtering.",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Increase verbosity level. Can be used multiple times (e.g., -vvv).",
    )

    parser.add_argument(
        "--status-diff",
        action="store_true",
        help="Show missing elements and differences in status.",
    )

    parser.add_argument(
        "--show-fails",
        action="store_true",
        help="Show failings tests",
    )

    args = parser.parse_args()

    args.threshold_filter = (
        "--threshold" in sys.argv
        or any(arg.startswith("--threshold=") for arg in sys.argv)
        or "--threshold-absolute" in sys.argv
        or any(arg.startswith("--threshold-absolute=") for arg in sys.argv)
    )

    args.verbose = min(args.verbose, 3)

    return args


def process_args(args):
    benchmarks = {}

    for benchmark in args.benchmark_files:
        try:
            with open(benchmark, "r", encoding="utf8") as file:
                data = yaml.safe_load(file)
                benchmarks.update(data)
        except FileNotFoundError:
            print(f"Error: The file '{benchmark}' was not found.")
        except yaml.YAMLError as e:
            print(f"Error parsing YAML file: {e}")
    return Args(
        parse_xml(args.file_old),
        parse_xml(args.file_new),
        args.verbose,
        benchmarks,
        args.targets,
        args.locations,
        args.suites,
        args.cases,
        args.threshold_absolute,
        args.threshold_relative,
        args.threshold_filter,
        args.status_diff,
        args.show_fails,
    )


def split_path(s):
    index = s.find("/")
    index = max(index, s.find("/", index + 1))
    return s[: index + 1], s[index + 1:]


def parse_xml(filename):
    try:
        xml = JUnitXml.fromfile(filename)
    except FileNotFoundError:
        print(f"{ESCAPE_BOLD}{ESCAPE_RED}Error:{ESCAPE_RESET} {filename} not found.")
        sys.exit(1)
    except Exception as e:
        print(f"{ESCAPE_BOLD}{ESCAPE_RED}Error:{ESCAPE_RESET} Failed to parse XML file: {e}")
        sys.exit(1)

    testsuites = {}
    for suite in xml:
        if not suite.name or ":" not in suite.name:
            print(
                f"{ESCAPE_BOLD}{ESCAPE_YELLOW}Warning:{ESCAPE_RESET} Skipping suite with malformed name: '{suite.name}'"
            )
            continue

        target, path = suite.name.split(":", 1)
        if target not in testsuites:
            testsuites[target] = {}

        location, suite_name = split_path(path)
        if not location:
            location = suite_name

        testsuite = Testsuite(target, location, suite_name)

        for case in suite:
            name = case.name.split(":", 1)[-1]
            classname = case.classname if case.classname else ""
            basename = name[len(classname) + 1:] if name.startswith(classname) else name

            status = Status.OK
            if case.result:
                if isinstance(case.result[0], Skipped):
                    status = Status.SKIP
                elif isinstance(case.result[0], (Failure, Error)):
                    status = Status.FAIL

            testcase = Testcase(basename, float(case.time), status)
            testsuite.cases[basename] = testcase
        if location not in testsuites[target]:
            testsuites[target][location] = {}
        testsuites[target][location][suite_name] = testsuite
    return testsuites


def remove_non_common(old, new):
    only_old = []
    for element in old.keys():
        if element not in new:
            only_old.append(element)
    for element in only_old:
        del old[element]

    only_new = []
    for element in new.keys():
        if element not in old:
            only_new.append(element)
    for element in only_new:
        del new[element]
    return only_old, only_new


def difference_and_percentage(old, new):
    difference = new - old
    percentage = 100 * difference / old if old > 0 else 0 if difference == 0 else INF
    return difference, percentage


def change_dir(difference, percentage, args):
    if difference is None:
        return ChangeDirection.MISSING
    if percentage < -args.threshold_relative and difference < -args.threshold_absolute:
        return ChangeDirection.DECREASE
    if percentage > args.threshold_relative and difference > args.threshold_absolute:
        return ChangeDirection.INCREASE
    return ChangeDirection.UNCHANGED


CHANGE_COLORS = {
    ChangeDirection.INCREASE: ESCAPE_RED,
    ChangeDirection.DECREASE: ESCAPE_GREEN,
    ChangeDirection.UNCHANGED: "",
    ChangeDirection.MISSING: "",
}


def compare_cases(cases_old, cases_new):
    cases = []
    for name, case in cases_old.items():
        cases.append(ComparedCaseNode.from_cases(case, cases_new.get(name, None)))
    for name, case in cases_new.items():
        if name not in cases_old:
            case_cmp = ComparedCaseNode.from_cases(None, case)
            cases.append(case_cmp)
    return cases


def compare_level(data_old, data_new, args, depth=0, path=None):
    if depth == Level.CASE:
        cases = compare_cases(data_old.cases, data_new.cases)
        cases = [case for case in cases if filter(path + [case.name], args)]
        output = ComparedSuiteNode(
            name=path[-1],
            children=cases,
            time_old=sum(
                case.time_old for case in cases if case.status_old == Status.OK and case.status_new == Status.OK
            ),
            time_new=sum(
                case.time_new for case in cases if case.status_old == Status.OK and case.status_new == Status.OK
            ),
            single_case=(
                len(cases) == 1
                and cases[0].name
                in [
                    "",
                    path[-1],
                ]
                and cases[0].status_old == Status.OK
                and cases[0].status_new == Status.OK
            ),
        )

        return output
    only_old, only_new = remove_non_common(data_old, data_new)
    only_old = [item for item in only_old if filter((path or []) + [item], args)]
    only_new = [item for item in only_new if filter((path or []) + [item], args)]
    output = ComparedContainerNode(
        name=path[-1] if path else None,
        time_old=0,
        time_new=0,
        children=[],
        only_old=only_old,
        only_new=only_new,
    )
    for child, child_data in data_old.items():
        new_path = (path or []) + [child]
        if not filter(new_path, args):
            continue
        child_data_new = data_new[child]
        child_output = compare_level(
            child_data,
            child_data_new,
            args,
            depth + 1,
            new_path,
        )
        if child_output.children:
            output.children.append(child_output)
            output.time_old += child_output.time_old
            output.time_new += child_output.time_new
    return output


def filter_benchmark(path, args):
    if not args.benchmarks:
        return True
    for benchmark in args.benchmarks.values():
        if len(path) > Level.LOCATION and path[Level.LOCATION] != benchmark["location"]:
            continue
        if len(path) > Level.SUITE and path[Level.SUITE] not in benchmark["suites"]:
            continue
        if len(path) > Level.CASE and path[Level.CASE] not in benchmark["suites"][path[Level.SUITE]]["cases"]:
            continue
        return True
    return False


def filter(path, args):
    if len(path) > Level.TARGET and args.targets and path[Level.TARGET] not in args.targets:
        return False
    if len(path) > Level.LOCATION and args.locations and path[Level.LOCATION] not in args.locations:
        return False
    if len(path) > Level.SUITE and args.suites and path[Level.SUITE] not in args.suites:
        return False
    if len(path) > Level.CASE and args.cases and path[Level.CASE] not in args.cases:
        return False
    return filter_benchmark(path, args)


def filter_display(data, level, args: Args):
    if level == Level.CASE:
        return filter_case_display(data, args)
    if args.threshold_filter:
        difference, percentage = difference_and_percentage(data.time_old, data.time_new)
        return change_dir(difference, percentage, args) in (ChangeDirection.INCREASE, ChangeDirection.DECREASE)
    return True


def filter_case_display(case, args):
    if case.status_old != Status.OK or case.status_new != Status.OK:
        return False
    if args.threshold_filter:
        difference, percentage = difference_and_percentage(case.time_old, case.time_new)
        return change_dir(difference, percentage, args) in (ChangeDirection.INCREASE, ChangeDirection.DECREASE)
    return True


def print_rows(rows):
    if not rows:
        return
    max_name_len = max(row.name_width() for row in rows)
    max_name_len = max(max_name_len, 4)
    for row in rows:
        row.print(max_name_len)


def print_fails(fails, args, level=Level.TARGET, parent=None):
    styles = [ESCAPE_BOLD + ESCAPE_ITALIC, ESCAPE_BOLD, ESCAPE_BOLD, ""]
    prefixes = ["Target: ", "-", " -", "  -"]
    if level == Level.CASE:
        if len(fails) > 1 or fails[0] != "" and fails[0] != parent:
            for name in fails:
                print(f"{styles[level]}{prefixes[level]}{name}{ESCAPE_RESET}")
        return
    for name, element in fails.items():
        print(f"{styles[level]}{prefixes[level]}{name}{ESCAPE_RESET}")
        if args.verbose > level:
            print_fails(element, args, level + 1, parent)
            if level == 0:
                print()


def generate_status_rows(statuses, args, level=Level.TARGET):
    styles = [ESCAPE_BOLD + ESCAPE_ITALIC, ESCAPE_BOLD, ESCAPE_BOLD, ""]
    separators = ["║", "║", "┃", "┆"]
    prefixes = ["Target: ", "-", " -", "  -"]
    rows = []
    for node in statuses:
        if args.verbose == level and node.status_old == node.status_new:
            continue
        current_row = StatusRow(
            styles[level],
            separators[level],
            f"{prefixes[level]}{node.name}",
            node.status_old,
            node.status_new,
        )
        children_rows = None
        if args.verbose > level:
            children_rows = generate_status_rows(node.children, args, level + 1)
        if node.status_old != node.status_new or children_rows:
            if level == 0 and (args.verbose > 0 or not rows):
                if rows:
                    rows.append(EmptyRow())
                rows.append(StatusRowHeader())
            rows.append(current_row)
            if args.verbose > level:
                rows.extend(children_rows)
    return rows


def generate_time_rows(times, args, level=Level.TARGET):
    styles = [ESCAPE_BOLD + ESCAPE_ITALIC, ESCAPE_BOLD, ESCAPE_BOLD, ""]
    separators = ["║", "║", "┃", "┆"]
    prefixes = ["Target: ", "-", " -", "  -"]
    rows = []
    for node in times:
        go_deeper = args.verbose > level and (level < Level.SUITE or not node.single_case)
        if not go_deeper and not filter_display(node, level, args):
            continue
        current_row = TimeRow(node, styles[level], separators[level], prefixes[level], args)
        children_rows = None
        if go_deeper:
            children_rows = generate_time_rows(node.children, args, level + 1)
        if not go_deeper or children_rows:
            if level == 0 and (args.verbose > 0 or not rows):
                if rows:
                    rows.append(EmptyRow())
                rows.append(TimeRowHeader())
            rows.append(current_row)
            if go_deeper:
                rows.extend(children_rows)
    return rows


def find_missing(results, level=Level.TARGET):
    if level == Level.CASE:
        return [
            ComparedStatusNode(name=case.name, status_old=case.status_old, status_new=case.status_new)
            for case in results.children
            if case.status_old != case.status_new
        ]
    only_old = [
        ComparedStatusNode(name=result, status_old=Status.PRESENT, status_new=Status.MISSING, children=[])
        for result in results.only_old
    ]
    only_new = [
        ComparedStatusNode(name=result, status_old=Status.MISSING, status_new=Status.PRESENT, children=[])
        for result in results.only_new
    ]
    with_children = [
        ComparedStatusNode(name=result.name, status_old=Status.PRESENT, status_new=Status.PRESENT, children=children)
        for result, children in ((result, find_missing(result, level + 1)) for result in results.children)
        if children
    ]
    return only_old + only_new + with_children


def find_fails(results, args, unfiltered=False, level=Level.TARGET, path=None):
    if level == Level.CASE:
        return [
            case.name
            for case in results.cases.values()
            if case.status == Status.FAIL and (unfiltered or filter((path or []) + [case.name], args))
        ]
    return {
        child_name: children
        for child_name, children in (
            (child_name, find_fails(child, args, unfiltered, level + 1, (path or []) + [child_name]))
            for child_name, child in results.items()
        )
        if children and (unfiltered or filter((path or []) + [child_name], args))
    }


def count_fails(fails, level=Level.TARGET):
    if level == Level.CASE:
        return len(fails)
    return sum(count_fails(child, level + 1) for child in fails.values())


def main():
    args: Args = process_args(parse_args())
    for file, name in [(args.file_old, "old"), (args.file_new, "new")]:
        fails = find_fails(file, args, unfiltered=True)
        if fails:
            fails_filtered = find_fails(file, args)
            if args.show_fails:
                print(f"Failed tests in {name} file:")
                print_fails(fails_filtered, args)
            else:
                print(
                    f"{ESCAPE_BOLD}{ESCAPE_YELLOW}Warning:{ESCAPE_RESET} "
                    f"{count_fails(fails)} tests failed in the {name} file "
                    f"({count_fails(fails_filtered)} matching filters)"
                )
                print("Use --show-fails to list the failing tests")
    output = compare_level(args.file_old, args.file_new, args)
    if args.status_diff:
        status_diff = find_missing(output)
        print_rows(generate_status_rows(status_diff, args))
    elif not args.show_fails:
        rows = generate_time_rows(output.children, args)
        print_rows(rows)


if __name__ == "__main__":
    main()
