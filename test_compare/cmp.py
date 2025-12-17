#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys
from abc import ABC, abstractmethod
from dataclasses import dataclass, field, replace
from enum import Enum, IntEnum
from typing import ClassVar

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
    ROOT = -1
    TARGET = 0
    LOCATION = 1
    SUITE = 2
    CASE = 3

    def __add__(self, other):
        return Level(self.value + other)

    @property
    def prefix(self):
        if self == self.ROOT:
            raise ValueError("Root level does not have a prefix")
        prefixes = ["Target: ", "-", " -", "  -"]
        return prefixes[self]

    @property
    def style(self):
        if self == self.ROOT:
            raise ValueError("Root level does not have a style")
        styles = [ESCAPE_BOLD + ESCAPE_ITALIC, ESCAPE_BOLD, ESCAPE_BOLD, ""]
        return styles[self]

    @property
    def separator(self):
        if self == self.ROOT:
            raise ValueError("Root level does not have a separator")
        separators = ["║", "║", "┃", "┆"]
        return separators[self]

    @property
    def failure_class(self):
        classes = [
            FailuresRootNode,
            FailuresTargetNode,
            FailuresLocationNode,
            FailuresSuiteNode,
            FailuresCaseNode,
        ]
        return classes[self.value + 1]

    @property
    def compared_class(self):
        classes = [
            ComparedRootNode,
            ComparedTargetNode,
            ComparedLocationNode,
            ComparedSuiteNode,
            ComparedCaseNode,
        ]
        return classes[self.value + 1]

    @property
    def compared_status_class(self):
        classes = [
            ComparedRootStatusNode,
            ComparedTargetStatusNode,
            ComparedLocationStatusNode,
            ComparedSuiteStatusNode,
            ComparedCaseStatusNode,
        ]
        return classes[self.value + 1]


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
class Path:
    level: Level
    target: str | None = None
    location: str | None = None
    suite: str | None = None
    case: str | None = None

    FIELD_MAP: ClassVar[dict] = {
        Level.TARGET: "target",
        Level.LOCATION: "location",
        Level.SUITE: "suite",
        Level.CASE: "case",
    }

    def __add__(self, next_name: str):
        """Return a new Path advanced one level deeper with `next_name` set."""
        next_level = self.level + 1
        if next_level > Level.CASE:
            raise ValueError("Cannot go deeper than CASE level")

        # Figure out which field to set based on the next level
        field_name = self.FIELD_MAP[next_level]

        # Create a new Path with updated field and level
        return replace(self, level=next_level, **{field_name: next_name})

    @property
    def name(self) -> str:
        return getattr(self, self.FIELD_MAP[self.level])

    def should_process(self, args: Args):
        """
        Determine whether a node should be considered for comparison.

        This function applies general command-line filters (targets, locations, suites, cases)
        and benchmark filters to decide whether a node should be included in comparison.
        """
        if self.level >= Level.TARGET and args.targets and self.target not in args.targets:
            return False
        if self.level >= Level.LOCATION and args.locations and self.location not in args.locations:
            return False
        if self.level >= Level.SUITE and args.suites and self.suite not in args.suites:
            return False
        if self.level >= Level.CASE and args.cases and self.case not in args.cases:
            return False

        if not args.benchmarks:
            return True
        for benchmark in args.benchmarks.values():
            if self.level >= Level.LOCATION and self.location != benchmark["location"]:
                continue
            if self.level >= Level.SUITE and self.suite not in benchmark["suites"]:
                continue
            if self.level >= Level.CASE and self.case not in benchmark["suites"][self.suite]["cases"]:
                continue
            return True
        return False


class Row(ABC):
    @abstractmethod
    def name_width(self): ...

    @abstractmethod
    def print(self, max_name_len): ...


@dataclass
class TimeRow(Row):
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
        self.name = f"{prefix}{data.path.name}"
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
class StatusRow(Row):
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


class StatusRowHeader(Row):
    def name_width(self):
        return len("NAME")

    def print(self, max_name_len):
        print(f"{ESCAPE_BOLD}{'':>{max_name_len}}║{'OLD':^9}║{'NEW':^9}║{ESCAPE_RESET}")
        print(f"{ESCAPE_BOLD}{ESCAPE_UNDERLINE}{'NAME':^{max_name_len}}║{'STATUS':^9}║{'STATUS':^9}║{ESCAPE_RESET}")


class TimeRowHeader(Row):
    def name_width(self):
        return len("NAME")

    def print(self, max_name_len):
        print(f"{ESCAPE_BOLD}{'':>{max_name_len}}║{'OLD':^9}║{'NEW':^9}║{'DIFFERENCE':^20}{ESCAPE_RESET}")
        print(
            f"{ESCAPE_BOLD}{ESCAPE_UNDERLINE}{'NAME':^{max_name_len}}║"
            f"{'TIME':^9}║{'TIME':^9}║{'TIME':^10}║{'PERCENTAGE':^9}{ESCAPE_RESET}"
        )


class EmptyRow(Row):
    def name_width(self):
        return 0

    def print(self, _):
        print()


@dataclass
class ComparedStatusNode(ABC):
    path: Path
    status_old: Status
    status_new: Status
    children: [] = field(default_factory=list)

    def generate_status_rows(self, args: Args) -> list[Row]:
        rows = []
        for node in self.children:
            if args.verbose == node.path.level and node.status_old == node.status_new:
                continue
            current_row = StatusRow(
                node.path.level.style,
                node.path.level.separator,
                f"{node.path.level.prefix}{node.path.name}",
                node.status_old,
                node.status_new,
            )
            children_rows = None
            if args.verbose > node.path.level:
                children_rows = node.generate_status_rows(args)
            if node.status_old != node.status_new or children_rows:
                if node.path.level == 0 and (args.verbose > 0 or not rows):
                    if rows:
                        rows.append(EmptyRow())
                    rows.append(StatusRowHeader())
                rows.append(current_row)
                if args.verbose > node.path.level:
                    rows.extend(children_rows)
        return rows


@dataclass
class ComparedRootStatusNode(ComparedStatusNode):
    pass


@dataclass
class ComparedTargetStatusNode(ComparedStatusNode):
    pass


@dataclass
class ComparedLocationStatusNode(ComparedStatusNode):
    pass


@dataclass
class ComparedSuiteStatusNode(ComparedStatusNode):
    pass


@dataclass
class ComparedCaseStatusNode(ComparedStatusNode):
    pass


@dataclass
class ComparedNode(ABC):
    path: Path
    time_old: float
    time_new: float

    def should_display(self, args: Args) -> bool:
        """
        Determine whether a comparison result should be shown in the output.

        This function is used after comparisons are made to decide whether a node
        should appear in the displayed results, based on thresholds or case-level status.

        Args:
            args (Args): Parsed command-line arguments controlling display behavior.

        Returns:
            bool: True if the node should be displayed, False otherwise.
        """
        if args.threshold_filter:
            difference, percentage = difference_and_percentage(self.time_old, self.time_new)
            return change_dir(difference, percentage, args) in (ChangeDirection.INCREASE, ChangeDirection.DECREASE)
        return True

    @abstractmethod
    def generate_time_rows(self, args: Args) -> list[Row]: ...

    @abstractmethod
    def find_missing(self) -> list[ComparedStatusNode]: ...


@dataclass
class ComparedAgregateNode(ComparedNode, ABC):
    children: [] = field(default_factory=list)

    def generate_time_rows(self, args: Args) -> list[Row]:
        rows = []
        for node in self.children:
            go_deeper = args.verbose > node.path.level and (node.path.level < Level.SUITE or not node.single_case)
            if not go_deeper and not node.should_display(args):
                continue
            current_row = TimeRow(node, node.path.level.style, node.path.level.separator, node.path.level.prefix, args)
            children_rows = None
            if go_deeper:
                children_rows = node.generate_time_rows(args)
            if not go_deeper or children_rows:
                if node.path.level == 0 and (args.verbose > 0 or not rows):
                    if rows:
                        rows.append(EmptyRow())
                    rows.append(TimeRowHeader())
                rows.append(current_row)
                if go_deeper:
                    rows.extend(children_rows)
        return rows

    def find_missing(self) -> list[ComparedStatusNode]:
        children = [result for child in self.children if (result := child.find_missing())]
        return (
            self.path.level.compared_status_class(
                path=self.path, status_old=Status.PRESENT, status_new=Status.PRESENT, children=children
            )
            if children
            else None
        )


@dataclass
class ComparedContainerNode(ComparedAgregateNode, ABC):
    only_old: [] = field(default_factory=list)
    only_new: [] = field(default_factory=list)

    def find_missing(self) -> list[ComparedStatusNode]:
        only_old = [
            path.level.compared_status_class(
                path=path, status_old=Status.PRESENT, status_new=Status.MISSING, children=[]
            )
            for path in (self.path + result for result in self.only_old)
        ]
        only_new = [
            path.level.compared_status_class(
                path=path, status_old=Status.MISSING, status_new=Status.PRESENT, children=[]
            )
            for path in (self.path + result for result in self.only_new)
        ]
        result = super().find_missing() or self.path.level.compared_status_class(
            path=self.path, status_old=Status.PRESENT, status_new=Status.PRESENT
        )
        result.children = only_old + only_new + result.children
        return result if result.children else None


@dataclass
class ComparedRootNode(ComparedContainerNode):
    def find_missing(self) -> list[ComparedStatusNode]:
        return super().find_missing() or self.path.level.compared_status_class(
            path=self.path, status_old=Status.PRESENT, status_new=Status.PRESENT
        )


@dataclass
class ComparedTargetNode(ComparedContainerNode):
    pass


@dataclass
class ComparedLocationNode(ComparedContainerNode):
    pass


@dataclass
class ComparedSuiteNode(ComparedAgregateNode):
    single_case: bool = False


@dataclass
class ComparedCaseNode(ComparedNode):
    status_old: Status
    status_new: Status
    difference: float = None
    percentage: float = None

    @classmethod
    def from_cases(cls, case1, case2):
        case_cmp = cls(
            path=case1.path if case1 else case2.path,
            time_old=case1.time if case1 else None,
            status_old=case1.status if case1 else Status.NONE,
            time_new=case2.time if case2 else None,
            status_new=case2.status if case2 else Status.NONE,
        )
        case_cmp.difference, case_cmp.percentage = (
            difference_and_percentage(case_cmp.time_old, case_cmp.time_new) if case1 and case2 else (None, None)
        )
        return case_cmp

    def should_display(self, args: Args) -> bool:
        if self.status_old != Status.OK or self.status_new != Status.OK:
            return False
        if args.threshold_filter:
            difference, percentage = difference_and_percentage(self.time_old, self.time_new)
            return change_dir(difference, percentage, args) in (ChangeDirection.INCREASE, ChangeDirection.DECREASE)
        return True

    def generate_time_rows(self, args: Args) -> list[Row]:
        return []

    def find_missing(self) -> ComparedStatusNode | None:
        return (
            ComparedCaseStatusNode(self.path, self.status_old, self.status_new)
            if self.status_old != self.status_new
            else None
        )


@dataclass
class FailuresNode(ABC):
    path: Path

    @abstractmethod
    def count_failures(self): ...

    @abstractmethod
    def print(self, args: Args): ...


@dataclass
class FailuresContainerNode(FailuresNode, ABC):
    children: list[FailuresNode]

    def count_failures(self):
        return sum(child.count_failures() for child in self.children)

    def print(self, args: Args):
        style = self.path.level.style
        prefix = self.path.level.prefix
        if args.verbose >= self.path.level:
            print(f"{style}{prefix}{self.path.name}{ESCAPE_RESET}")
            for child in self.children:
                child.print(args)


@dataclass
class FailuresRootNode(FailuresContainerNode):
    def print(self, args: Args):
        for child in self.children:
            child.print(args)


@dataclass
class FailuresTargetNode(FailuresContainerNode):
    pass


@dataclass
class FailuresLocationNode(FailuresContainerNode):
    pass


@dataclass
class FailuresSuiteNode(FailuresContainerNode):
    def print(self, args: Args):
        styles = [ESCAPE_BOLD + ESCAPE_ITALIC, ESCAPE_BOLD, ESCAPE_BOLD, ""]
        prefixes = ["Target: ", "-", " -", "  -"]
        style = styles[self.path.level]
        prefix = prefixes[self.path.level]
        if args.verbose >= self.path.level:
            print(f"{style}{prefix}{self.path.name}{ESCAPE_RESET}")
            if not (len(self.children) == 1 and self.children[0].path.name in ("", self.path.name)):
                for child in self.children:
                    child.print(args)


@dataclass
class FailuresCaseNode(FailuresNode):
    def count_failures(self):
        return 1

    def print(self, args):
        styles = [ESCAPE_BOLD + ESCAPE_ITALIC, ESCAPE_BOLD, ESCAPE_BOLD, ""]
        prefixes = ["Target: ", "-", " -", "  -"]
        style = styles[self.path.level]
        prefix = prefixes[self.path.level]

        if args.verbose >= self.path.level:
            print(f"{style}{prefix}{self.path.name}{ESCAPE_RESET}")


@dataclass
class ResultNode(ABC):
    path: Path


@dataclass
class ContainerResultNode(ResultNode, ABC):
    children: dict[str, ResultNode] = field(default_factory=dict)
    COMPARED_CLASS: ClassVar[type[ComparedNode]]
    FAILURE_CLASS: ClassVar[type[FailuresNode]]

    def failures(self, args: Args, filtered: bool = False) -> FailuresNode:
        if self.children and (not filtered or self.path.should_process(args)):
            node = self.FAILURE_CLASS(
                self.path, [fails for child in self.children.values() if (fails := child.failures(args, filtered))]
            )
            return node if node.children else None
        return None

    def compare(self, other, args):
        only_old, only_new = remove_non_common(self.children, other.children)
        only_old = [item for item in only_old if (self.path + item).should_process(args)]
        only_new = [item for item in only_new if (self.path + item).should_process(args)]
        output = self.COMPARED_CLASS(
            path=self.path,
            time_old=0,
            time_new=0,
            children=[],
            only_old=only_old,
            only_new=only_new,
        )
        for child, child_data in self.children.items():
            if not child_data.path.should_process(args):
                continue
            child_data_new = other.children[child]
            child_output = child_data.compare(child_data_new, args)
            if child_output.children:
                output.children.append(child_output)
                output.time_old += child_output.time_old
                output.time_new += child_output.time_new
        return output


@dataclass
class RootResultNode(ContainerResultNode):
    COMPARED_CLASS: ClassVar[type[ComparedNode]] = ComparedRootNode
    FAILURE_CLASS: ClassVar[type[FailuresNode]] = FailuresRootNode


@dataclass
class Target(ContainerResultNode):
    COMPARED_CLASS: ClassVar[type[ComparedNode]] = ComparedTargetNode
    FAILURE_CLASS: ClassVar[type[FailuresNode]] = FailuresTargetNode


@dataclass
class Location(ContainerResultNode):
    COMPARED_CLASS: ClassVar[type[ComparedNode]] = ComparedLocationNode
    FAILURE_CLASS: ClassVar[type[FailuresNode]] = FailuresLocationNode


@dataclass
class Testsuite(ContainerResultNode):
    COMPARED_CLASS: ClassVar[type[ComparedNode]] = ComparedSuiteNode
    FAILURE_CLASS: ClassVar[type[FailuresNode]] = FailuresSuiteNode

    def compare(self, other, args):
        cases = []
        for name, case in self.children.items():
            cases.append(ComparedCaseNode.from_cases(case, other.children.get(name, None)))
        for name, case in other.children.items():
            if name not in self.children:
                case_cmp = ComparedCaseNode.from_cases(None, case)
                cases.append(case_cmp)
        cases = [case for case in cases if case.path.should_process(args)]
        output = self.COMPARED_CLASS(
            path=self.path,
            children=cases,
            time_old=sum(
                case.time_old for case in cases if case.status_old == Status.OK and case.status_new == Status.OK
            ),
            time_new=sum(
                case.time_new for case in cases if case.status_old == Status.OK and case.status_new == Status.OK
            ),
            single_case=(
                len(cases) == 1
                and cases[0].path.case
                in [
                    "",
                    self.path.suite,
                ]
                and cases[0].status_old == Status.OK
                and cases[0].status_new == Status.OK
            ),
        )

        return output


@dataclass
class Testcase(ResultNode):
    time: float
    status: Status

    def failures(self, args: Args, filtered: bool = False):
        if (not filtered or self.path.should_process(args)) and self.status == Status.FAIL:
            return FailuresCaseNode(self.path)
        return None


@dataclass
class Args:
    file_old: RootResultNode
    file_new: RootResultNode
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


def process_args(args: argparse.Namespace) -> Args:
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


def split_path(s: str) -> tuple[str, str]:
    """
    Split path into directory and test suite.

    Directory is typically up to the second '/' but there might be less than 2 '/' characters in the path,
    in that case the directory name is shorter.
    """
    index = s.find("/")
    index = max(index, s.find("/", index + 1))
    return s[: index + 1], s[index + 1:]


def parse_xml(filename: str) -> RootResultNode:
    try:
        xml = JUnitXml.fromfile(filename)
    except FileNotFoundError:
        print(f"{ESCAPE_BOLD}{ESCAPE_RED}Error:{ESCAPE_RESET} {filename} not found.")
        sys.exit(1)
    except Exception as e:
        print(f"{ESCAPE_BOLD}{ESCAPE_RED}Error:{ESCAPE_RESET} Failed to parse XML file: {e}")
        sys.exit(1)

    root = RootResultNode(Path(Level.ROOT))
    for suite in xml:
        # suite.name format is 'target:path', where target is the target name
        # and path is equal to case.classname for each case
        if not suite.name or ":" not in suite.name:
            print(
                f"{ESCAPE_BOLD}{ESCAPE_YELLOW}Warning:{ESCAPE_RESET} Skipping suite with malformed name: '{suite.name}'"
            )
            continue

        target_name, path = suite.name.split(":", 1)
        if target_name not in root.children:
            root.children[target_name] = Target(Path(Level.TARGET, target_name))
        target = root.children[target_name]

        location_name, suite_name = split_path(path)
        if not location_name:
            location_name = suite_name
        if location_name not in target.children:
            target.children[location_name] = Location(Path(Level.LOCATION, target_name, location_name))
        location = target.children[location_name]

        if suite_name not in location.children:
            location.children[suite_name] = Testsuite(Path(Level.SUITE, target_name, location_name, suite_name))
        testsuite = location.children[suite_name]

        for case in suite:
            # case.name is 'target:path.testcase', where target and path are the same as in suite
            # and testscase it the name of the test case
            if not case.name or ":" not in case.name:
                print(
                    f"{ESCAPE_BOLD}{ESCAPE_YELLOW}Warning:{ESCAPE_RESET} "
                    f"Skipping case with malformed name: '{case.name}'"
                )
                continue
            name = case.name.split(":", 1)[-1]
            if not case.classname:
                print(
                    f"{ESCAPE_BOLD}{ESCAPE_YELLOW}Warning:{ESCAPE_RESET} "
                    f"Skipping case with malformed classname: '{case.classname}'"
                )
                continue

            if not name.startswith(case.classname):
                print(
                    f"{ESCAPE_BOLD}{ESCAPE_YELLOW}Warning:{ESCAPE_RESET} "
                    f"Skipping case with malformed name: '{case.name}'"
                )
                continue
            basename = name[len(case.classname) + 1:]

            status = Status.OK
            if case.result:
                if isinstance(case.result[0], Skipped):
                    status = Status.SKIP
                elif isinstance(case.result[0], (Failure, Error)):
                    status = Status.FAIL

            testcase = Testcase(
                Path(Level.CASE, target_name, location_name, suite_name, basename), float(case.time), status
            )
            testsuite.children[basename] = testcase
    return root


def remove_non_common(old: dict, new: dict) -> tuple[list, list]:
    """
    Remove elements occurring in only one dict, return the removed elements.

    If element (directory, suite, case) is present in only one of the dicts, there is nothing to compare it to.
    """
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


def difference_and_percentage(old: float | None, new: float | None) -> tuple[float | None, float | None]:
    difference = new - old
    percentage = (100 * difference / old) if old > 0 else (0 if difference == 0 else INF)
    return difference, percentage


def change_dir(difference: float | None, percentage: float | None, args: Args) -> ChangeDirection:
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


def print_rows(rows: list[TimeRow | StatusRow | EmptyRow | StatusRowHeader | TimeRowHeader]) -> None:
    if not rows:
        return
    max_name_len = max(row.name_width() for row in rows)
    for row in rows:
        row.print(max_name_len)


def main() -> None:
    args: Args = process_args(parse_args())
    for file, name in [(args.file_old, "old"), (args.file_new, "new")]:
        fails = file.failures(args)
        if fails:
            fails_filtered = file.failures(args, filtered=True)
            if args.show_fails:
                print(f"Failed tests in {name} file:")
                fails_filtered.print(args)
            else:
                print(
                    f"{ESCAPE_BOLD}{ESCAPE_YELLOW}Warning:{ESCAPE_RESET} "
                    f"{fails.count_failures()} tests failed in the {name} file "
                    f"({fails_filtered.count_failures()} matching filters)"
                )
                print("Use --show-fails to list the failing tests")
    output = args.file_old.compare(args.file_new, args)
    if args.status_diff:
        status_diff = output.find_missing()
        print_rows(status_diff.generate_status_rows(args))
    elif not args.show_fails:
        rows = output.generate_time_rows(args)
        print_rows(rows)


if __name__ == "__main__":
    main()
