#!/usr/bin/env python

import argparse
import enum
import sys
from dataclasses import dataclass

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
OK = f"{ESCAPE_GREEN}  OK  {ESCAPE_RESET}"
SKIP = f"{ESCAPE_GRAY} SKIP {ESCAPE_RESET}"
FAIL = f"{ESCAPE_RED} FAIL {ESCAPE_RESET}"


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
    show_missing: bool


def parse_args():
    parser = argparse.ArgumentParser(
        description="Compares two test result xml files",
        epilog="Example: python cmp.py old_results.xml new_results.xml -v --threshold 5.0 -t ia32-generic-qemu",
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
        help="Set relative difference threshold (default: %(default)s%%).",
    )
    parser.add_argument(
        "--threshold-absolute",
        type=float,
        default=0.1,
        metavar="VALUE",
        help="Set absolute difference threshold (default: %(default)s).",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Increase verbosity level. Can be used multiple times (e.g., -vvv).",
    )

    parser.add_argument(
        "--show-missing",
        action="store_true",
        help="Show which elements are pressent in one file but missig in the other.",
    )

    args = parser.parse_args()

    args.threshold_filter = "--threshold" in sys.argv

    args.verbose = min(args.verbose, 3)

    return args


def process_args(args):
    benchmarks = {}

    if args.file_new is None:
        print("Usage:\n\tcmp.py OLD_FILE NEW_FILE")
        sys.exit()

    for benchmark in args.benchmark_files:
        try:
            with open(benchmark, "r", encoding="utf8") as file:
                data = yaml.safe_load(file)
                benchmarks.update(data)
        except FileNotFoundError:
            print("Error: The file 'config.yaml' was not found.")
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
        args.show_missing,
    )


def split_path(s):
    index = s.find("/")
    index = max(index, s.find("/", index + 1))
    return s[: index + 1], s[index + 1 :]


def parse_xml(filename):
    try:
        xml = JUnitXml.fromfile(filename)
    except FileNotFoundError:
        print(f"{ESCAPE_BOLD}{ESCAPE_RED}Error:{ESCAPE_RESET} {filename} not found.")
        sys.exit()
    except Exception as e:
        print(f"{ESCAPE_BOLD}{ESCAPE_RED}Error:{ESCAPE_RESET} Failed to parse XML file: {e}")
        sys.exit()

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

        testsuite = {
            "target": target,
            "location": location,
            "name": suite_name,
            "cases": {},
        }
        for case in suite:
            name = case.name
            name = name[name.find(":") + 1 :] if ":" in name else name
            classname = case.classname if case.classname else ""
            basename = name[len(classname) + 1 :] if name.startswith(classname) else name

            status = OK
            if case.result:
                if isinstance(case.result[0], Skipped):
                    status = SKIP
                elif isinstance(case.result[0], (Failure, Error)):
                    status = FAIL

            testcase = {
                "name": basename,
                "time": float(case.time) if case.time is not None else 0.0,
                "status": status,
            }
            testsuite["cases"][basename] = testcase
        if location not in testsuites[target]:
            testsuites[target][location] = {}
        testsuites[target][location][suite_name] = testsuite
    return testsuites


class ChangeDirection(enum.Enum):
    INCREASE = 1
    DECREASE = -1
    UNCHANGED = 0
    MISSING = 2


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


def filter_target(target, args):
    return len(args.targets) == 0 or target in args.targets


def filter_location(location, args):
    if len(args.benchmarks) > 0:
        for benchmark in args.benchmarks.values():
            if location == benchmark["location"]:
                return True
        return False
    return len(args.locations) == 0 or location in args.locations


def filter_suite(location, suite, args):
    if len(args.benchmarks) > 0:
        for benchmark in args.benchmarks.values():
            return location == benchmark["location"] and suite in benchmark["suites"]
    return len(args.suites) == 0 or suite in args.suites


def filter_case(location, suite, case, args):
    if len(args.benchmarks) > 0:
        for benchmark in args.benchmarks.values():
            return (
                location == benchmark["location"]
                and suite in benchmark["suites"]
                and case["name"] in benchmark["suites"][suite]["cases"]
            )
    return len(args.cases) == 0 or case["name"] in args.cases


def filter_display(data, args: Args):
    if args.threshold_filter:
        difference, percentage = difference_and_percentage(data["time_old"], data["time_new"])
        return change_dir(difference, percentage, args) in (ChangeDirection.INCREASE, ChangeDirection.DECREASE)
    return True


def filter_case_display(case, args):
    if case["time_old"] is None or case["time_new"] is None:
        return False
    if args.threshold_filter:
        difference, percentage = difference_and_percentage(case["time_old"], case["time_new"])
        return change_dir(difference, percentage, args) in (ChangeDirection.INCREASE, ChangeDirection.DECREASE)
    return True


def compare_cases(cases_old, cases_new):
    cases = []
    for name, case in cases_old.items():
        case_cmp = {}
        case_cmp["name"] = name
        case_cmp["time_old"] = case["time"]
        case_cmp["status_old"] = case["status"]
        if name in cases_new:
            case_cmp["time_new"] = cases_new[name]["time"]
            case_cmp["status_new"] = cases_new[name]["status"]
            case_cmp["difference"] = case_cmp["time_new"] - case_cmp["time_old"]
            case_cmp["percentage"] = (
                case_cmp["difference"] / case_cmp["time_old"] * 100
                if case_cmp["time_old"]
                else 0 if case_cmp["difference"] == 0 else INF
            )
        else:
            case_cmp["time_new"] = None
            case_cmp["status_new"] = "--"
            case_cmp["difference"] = None
            case_cmp["percentage"] = None
        cases.append(case_cmp)
    for name, case in cases_new.items():
        if name not in cases_old:
            case_cmp = {}
            case_cmp["name"] = name
            case_cmp["time_old"] = None
            case_cmp["status_old"] = "--"
            case_cmp["time_new"] = case["time"]
            case_cmp["status_new"] = case["status"]
            case_cmp["difference"] = None
            case_cmp["percentage"] = None
            cases.append(case_cmp)
    return cases


def difference_and_percentage(old, new):
    difference = new - old
    percentage = 100 * difference / old if old > 0 else 0 if difference == 0 else INF
    return difference, percentage


def make_row(data, style, separator, prefix, args):
    difference, percentage = difference_and_percentage(data["time_old"], data["time_new"])
    color = CHANGE_COLORS[change_dir(difference, percentage, args)]
    return Row(
        style,
        separator,
        f"{prefix}{data["name"]}",
        f"{data["time_old"]:.3f}",
        f"{data["time_new"]:.3f}",
        color,
        f"{difference:+.3f}",
        f"{percentage:+.2f}" if percentage < 10000 else "-.--",
    )


def make_case_status_row(case):
    return StatusRow(
        "",
        "┆",
        f"  -{case["name"]}",
        case["status_old"],
        case["status_new"],
    )


def make_case_row(case, args):
    difference, percentage = difference_and_percentage(case["time_old"], case["time_new"])
    color = CHANGE_COLORS[change_dir(difference, percentage, args)]
    return Row(
        "",
        "┆",
        f"  -{case["name"]}",
        f"{case["time_old"]:.3f}",
        f"{case["time_new"]:.3f}",
        color,
        f"{difference:+.3f}",
        f"{percentage:+.2f}" if percentage < 10000 else "-.--",
    )


@dataclass
class Row:
    style: str
    separator: str
    name: str
    time_old: str
    time_new: str
    color: str
    difference: str
    percentage: str


@dataclass
class StatusRow:
    style: str
    separator: str
    name: str
    status_old: str
    status_new: str


def print_rows(rows):
    max_name_len = max(len(row.name) for row in rows)
    max_name_len = max(max_name_len, 4)
    for row in rows:
        if row.name == "":
            print()
            continue
        if row.name == "H":
            print(f"{ESCAPE_BOLD}{"":>{max_name_len}}║{"OLD":^9}║{"NEW":^9}║{"DIFFERENCE":^20}{ESCAPE_RESET}")
            print(
                f"{ESCAPE_BOLD}{ESCAPE_UNDERLINE}{"NAME":^{max_name_len}}║"
                f"{"TIME":^9}║{"TIME":^9}║{"TIME":^10}║{"PERCENTAGE":^9}{ESCAPE_RESET}"
            )
            continue
        print(
            f"{row.style}{row.name:<{max_name_len}}{ESCAPE_RESET}{row.separator}"
            f"{row.style}{row.time_old:>8}s{ESCAPE_RESET}{row.separator}"
            f"{row.style}{row.time_new:>8}s{ESCAPE_RESET}{row.separator}"
            f"{row.style}{row.color}{row.difference:>9}s{ESCAPE_RESET}{row.separator}"
            f"{row.style}{row.color}{row.percentage:>8}%{ESCAPE_RESET}"
        )


def print_status_rows(rows):
    max_name_len = max(len(row.name) for row in rows)
    max_name_len = max(max_name_len, 4)
    for row in rows:
        if row.name == "":
            print()
            continue
        if row.name == "H":
            print(f"{ESCAPE_BOLD}{"":>{max_name_len}}║{"OLD":^6}║{"NEW":^6}║{ESCAPE_RESET}")
            print(
                f"{ESCAPE_BOLD}{ESCAPE_UNDERLINE}{"NAME":^{max_name_len}}║"
                f"{"STATUS":^6}║{"STATUS":^6}║{ESCAPE_RESET}"
            )
            continue
        print(
            f"{row.style}{row.name:<{max_name_len}}{ESCAPE_RESET}{row.separator}"
            f"{row.style}{row.status_old:^6}{ESCAPE_RESET}{row.separator}"
            f"{row.style}{row.status_new:^6}{ESCAPE_RESET}{row.separator}"
        )


def main():
    args: Args = process_args(parse_args())

    only_old_targets, only_new_targets = remove_non_common(args.file_old, args.file_new)

    output = {
        "time_old": 0,
        "time_new": 0,
        "targets": [],
        "only_old_targets": only_old_targets,
        "only_new_targets": only_new_targets,
    }
    for target, locations in args.file_old.items():
        if not filter_target(target, args):
            continue
        new_locations = args.file_new[target]
        only_old_locations, only_new_locations = remove_non_common(locations, new_locations)
        target_output = {
            "name": target,
            "time_old": 0,
            "time_new": 0,
            "locations": [],
            "only_old_locations": only_old_locations,
            "only_new_locations": only_new_locations,
        }
        for location, suites in locations.items():
            if not filter_location(location, args):
                continue
            new_suites = args.file_new[target][location]
            only_old_suites, only_new_suites = remove_non_common(suites, new_suites)
            location_output = {
                "name": location,
                "time_old": 0,
                "time_new": 0,
                "suites": [],
                "only_old_suites": only_old_suites,
                "only_new_suites": only_new_suites,
            }
            for name, suite in suites.items():
                if not filter_suite(suite["location"], suite["name"], args):
                    continue
                suite_output = {"name": name, "time_old": 0, "time_new": 0, "cases": [], "single_case": False}
                new_suite = new_suites[name]
                cases = compare_cases(suite["cases"], new_suite["cases"])
                suite_output["single_case"] = len(suite["cases"]) == 1 and next(iter(suite["cases"])) in [
                    "",
                    suite["name"],
                ]
                cases = [case for case in cases if filter_case(suite["location"], suite["name"], case, args)]
                suite_output["time_old"] = sum(
                    case["time_old"]
                    for case in cases
                    if case["time_old"] is not None
                    and case["time_new"] is not None
                    and case["status_old"] == OK
                    and case["status_new"] == OK
                )
                suite_output["time_new"] = sum(
                    case["time_new"]
                    for case in cases
                    if case["time_old"] is not None
                    and case["time_new"] is not None
                    and case["status_old"] == OK
                    and case["status_new"] == OK
                )
                suite_output["cases"] = cases
                location_output["suites"].append(suite_output)
                location_output["time_old"] += suite_output["time_old"]
                location_output["time_new"] += suite_output["time_new"]
            target_output["locations"].append(location_output)
            target_output["time_old"] += location_output["time_old"]
            target_output["time_new"] += location_output["time_new"]
        output["targets"].append(target_output)
        output["time_old"] += target_output["time_old"]
        output["time_new"] += target_output["time_new"]
    if args.show_missing:
        if only_old_targets:
            print("Targets misssing in new file:")
            for target in only_old_targets:
                print(f"  {target}")
        if only_new_targets:
            print("Targets misssing in old file:")
            for target in only_new_targets:
                print(f"  {target}")
        if args.verbose >= 1:
            if only_old_locations:
                print("Directories misssing in new file:")
                for location in only_old_locations:
                    print(f"  {location}")
            if only_new_locations:
                print("Directories misssing in old file:")
                for location in only_new_locations:
                    print(f"  {location}")
        if args.verbose >= 2:
            if only_old_suites:
                print("Suites misssing in new file:")
                for suite in only_old_locations:
                    print(f"  {suite}")
            if only_new_suites:
                print("Suites misssing in old file:")
                for suite in only_new_locations:
                    print(f"  {suite}")
    rows = []
    status_rows = []
    for target in output["targets"]:
        if args.verbose >= 1 or len(rows) == 0:
            rows.append(Row("", "", "H", "", "", "", "", ""))
            status_rows.append(StatusRow("", "", "H", "", ""))
        if filter_display(target, args) or args.verbose >= 1:
            rows.append(make_row(target, ESCAPE_BOLD + ESCAPE_ITALIC, "║", "Target: ", args))
            status_rows.append(StatusRow(ESCAPE_BOLD + ESCAPE_ITALIC, "║", f"Target: {target["name"]}", "", ""))
        if args.verbose < 1:
            continue
        for location in target["locations"]:
            if filter_display(location, args) or args.verbose >= 2:
                rows.append(make_row(location, ESCAPE_BOLD, "║", "-", args))
                status_rows.append(StatusRow(ESCAPE_BOLD, "║", f"-{location["name"]}", "", ""))
            if args.verbose < 2:
                continue
            for suite in location["suites"]:
                if filter_display(suite, args) or args.verbose >= 3:
                    rows.append(make_row(suite, ESCAPE_BOLD, "┃", " -", args))
                    status_rows.append(StatusRow(ESCAPE_BOLD, "┃", f" -{suite["name"]}", "", ""))
                if args.verbose < 3:
                    continue
                if suite["single_case"]:
                    case = suite["cases"][0]
                    if not filter_display(case, args):
                        del rows[-1]
                    if case["status_old"] != case["status_new"]:
                        status_rows.append(make_case_status_row(case))
                        status_rows[-1].style = ESCAPE_BOLD
                        status_rows[-1].separator = "┃"
                        status_rows[-1].name = " " + status_rows[-1].name
                    else:
                        del status_rows[-1]
                    continue
                for case in suite["cases"]:
                    if case["status_old"] != case["status_new"]:
                        status_rows.append(make_case_status_row(case))
                    if not filter_case_display(case, args):
                        continue
                    rows.append(make_case_row(case, args))
                if rows[-1].name.startswith(" -"):
                    del rows[-1]
                if status_rows[-1].name.startswith(" -"):
                    del status_rows[-1]
            if rows[-1].name.startswith("-"):
                del rows[-1]
            if status_rows[-1].name.startswith("-"):
                del status_rows[-1]
        if rows[-1].name.startswith("Target: "):
            del rows[-1]
        if status_rows[-1].name.startswith("Target: "):
            del status_rows[-1]
        while len(rows) > 0 and rows[-1].name in ("", "H"):
            del rows[-1]
        while len(status_rows) > 0 and status_rows[-1].name in ("", "H"):
            del status_rows[-1]
        if len(rows) > 0:
            rows.append(Row("", "", "", "", "", "", "", ""))
        if len(status_rows) > 0:
            status_rows.append(Row("", "", "", "", "", "", "", ""))
    if not args.show_missing:
        print_rows(rows)
    elif args.verbose == 3:
        print_status_rows(status_rows)


if __name__ == "__main__":
    main()
