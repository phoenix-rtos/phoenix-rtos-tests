import copy
import sys

import cmp
import pytest


@pytest.fixture(scope="module")
def mock_xml_files(tmp_path_factory):
    """Creates mock old and new XML files with a wide range of scenarios."""
    tmp_dir = tmp_path_factory.mktemp("data")
    old_xml_path = tmp_dir / "old.xml"
    new_xml_path = tmp_dir / "new.xml"

    old_xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<testsuites
    tests="16"
    failures="4"
    errors="0"
    skipped="3"
    time="53.300"
>
    <testsuite
        name="armv7a7-imx6ull-evk:phoenix-rtos-tests/kernel/common"
        tests="3"
        failures="0"
        errors="0"
        skipped="0"
        time="35.000"
        timestamp="2025-08-04T01:40:00"
        hostname="rpi"
    >
        <testcase
            name="armv7a7-imx6ull-evk:phoenix-rtos-tests/kernel/common.test_A_regression"
            classname="phoenix-rtos-tests/kernel/common"
            time="10.0"
        />
        <testcase
            name="armv7a7-imx6ull-evk:phoenix-rtos-tests/kernel/common.test_B_improvement"
            classname="phoenix-rtos-tests/kernel/common"
            time="20.0"
        />
        <testcase
            name="armv7a7-imx6ull-evk:phoenix-rtos-tests/kernel/common.test_C_removed"
            classname="phoenix-rtos-tests/kernel/common"
            time="5.0"
        />
    </testsuite>
    <testsuite
        name="armv7a7-imx6ull-evk:phoenix-rtos-tests/drivers/serial"
        tests="1"
        failures="0"
        errors="0"
        skipped="0"
        time="1.000"
        timestamp="2025-08-04T01:41:00"
        hostname="rpi"
    >
        <testcase
            name="armv7a7-imx6ull-evk:phoenix-rtos-tests/drivers/serial.test_status_change"
            classname="phoenix-rtos-tests/drivers/serial"
            time="1.0"
        />
    </testsuite>
    <testsuite
        name="ia32-generic-pc:phoenix-rtos-tests/libc/stdlib"
        tests="2"
        failures="1"
        errors="0"
        skipped="1"
        time="0.300"
        timestamp="2025-08-04T01:42:00"
        hostname="rpi"
    >
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/libc/stdlib.test_rand_fail"
            classname="phoenix-rtos-tests/libc/stdlib"
            time="0.1"
        >
            <failure message="Assertion failed" />
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/libc/stdlib.test_qsort_skipped"
            classname="phoenix-rtos-tests/libc/stdlib"
            time="0.2"
        >
            <skipped message="Hardware not available"/>
        </testcase>
    </testsuite>
    <testsuite
        name="ia32-generic-pc:phoenix-rtos-tests/micropython/core"
        tests="4"
        failures="2"
        errors="0"
        skipped="0"
        time="12.000"
        timestamp="2025-08-04T01:43:00"
        hostname="rpi"
    >
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_ok_to_fail"
            classname="phoenix-rtos-tests/micropython/core"
            time="2.0"
        />
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_fail_to_ok"
            classname="phoenix-rtos-tests/micropython/core"
            time="3.0"
        >
             <failure/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_fail_to_skip"
            classname="phoenix-rtos-tests/micropython/core"
            time="4.0"
        >
             <failure/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_skip_to_fail"
            classname="phoenix-rtos-tests/micropython/core"
            time="5.0"
        >
             <skipped/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_ok_to_skip"
            classname="phoenix-rtos-tests/micropython/core"
            time="6.0"
        >
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_skip_to_ok"
            classname="phoenix-rtos-tests/micropython/core"
            time="7.0"
        >
             <skipped/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_fail_to_fail"
            classname="phoenix-rtos-tests/micropython/core"
            time="7.0"
        >
             <failure/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_skip_to_skip"
            classname="phoenix-rtos-tests/micropython/core"
            time="7.0"
        >
             <skipped/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_no_change"
            classname="phoenix-rtos-tests/micropython/core"
            time="3.0"
        />
    </testsuite>
    <testsuite
        name="ia32-generic-pc:phoenix-rtos-tests/libc/single_case_suite"
        tests="1"
        failures="0"
        errors="0"
        skipped="0"
        time="5.000"
        timestamp="2025-08-04T01:44:00"
        hostname="rpi"
    >
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/libc/single_case_suite.single_case_suite"
            classname="phoenix-rtos-tests/libc/single_case_suite"
            time="5.0"
        />
    </testsuite>
</testsuites>
"""

    new_xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<testsuites
    tests="17"
    failures="4"
    errors="0"
    skipped="3"
    time="59.800"
>
    <testsuite
        name="armv7a7-imx6ull-evk:phoenix-rtos-tests/kernel/common"
        tests="3"
        failures="0"
        errors="0"
        skipped="0"
        time="33.000"
        timestamp="2025-08-04T02:40:00"
        hostname="rpi"
    >
        <testcase
            name="armv7a7-imx6ull-evk:phoenix-rtos-tests/kernel/common.test_A_regression"
            classname="phoenix-rtos-tests/kernel/common"
            time="15.0"
        />
        <testcase
            name="armv7a7-imx6ull-evk:phoenix-rtos-tests/kernel/common.test_B_improvement"
            classname="phoenix-rtos-tests/kernel/common"
            time="15.0"
        />
        <testcase
            name="armv7a7-imx6ull-evk:phoenix-rtos-tests/kernel/common.test_D_added"
            classname="phoenix-rtos-tests/kernel/common"
            time="3.0"
        />
    </testsuite>
    <testsuite
        name="armv7a7-imx6ull-evk:phoenix-rtos-tests/drivers/serial"
        tests="1"
        failures="0"
        errors="0"
        skipped="1"
        time="1.100"
        timestamp="2025-08-04T02:41:00"
        hostname="rpi"
    >
        <testcase
            name="armv7a7-imx6ull-evk:phoenix-rtos-tests/drivers/serial.test_status_change"
            classname="phoenix-rtos-tests/drivers/serial"
            time="1.1"
        >
             <skipped message="Now skipped"/>
        </testcase>
    </testsuite>
    <testsuite
        name="ia32-generic-pc:phoenix-rtos-tests/libc/stdlib"
        tests="2"
        failures="1"
        errors="0"
        skipped="0"
        time="0.400"
        timestamp="2025-08-04T02:42:00"
        hostname="rpi"
    >
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/libc/stdlib.test_rand_fail"
            classname="phoenix-rtos-tests/libc/stdlib"
            time="0.1"
        >
            <failure message="Assertion failed" />
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/libc/stdlib.test_qsort_skipped"
            classname="phoenix-rtos-tests/libc/stdlib"
            time="0.3"
        />
    </testsuite>
    <testsuite
        name="ia32-generic-pc:phoenix-rtos-tests/micropython/core"
        tests="4"
        failures="1"
        errors="0"
        skipped="1"
        time="12.300"
        timestamp="2025-08-04T02:43:00"
        hostname="rpi"
    >
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_ok_to_fail"
            classname="phoenix-rtos-tests/micropython/core"
            time="2.1"
        >
            <failure/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_fail_to_ok"
            classname="phoenix-rtos-tests/micropython/core"
            time="3.1"
        />
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_fail_to_skip"
            classname="phoenix-rtos-tests/micropython/core"
            time="4.1"
        >
             <skipped/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_skip_to_fail"
            classname="phoenix-rtos-tests/micropython/core"
            time="5.0"
        >
             <failure/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_ok_to_skip"
            classname="phoenix-rtos-tests/micropython/core"
            time="6.0"
        >
             <skipped/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_skip_to_ok"
            classname="phoenix-rtos-tests/micropython/core"
            time="7.0"
        >
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_fail_to_fail"
            classname="phoenix-rtos-tests/micropython/core"
            time="7.0"
        >
             <failure/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_skip_to_skip"
            classname="phoenix-rtos-tests/micropython/core"
            time="7.0"
        >
             <skipped/>
        </testcase>
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/micropython/core.test_no_change"
            classname="phoenix-rtos-tests/micropython/core"
            time="3.0"
        />
    </testsuite>
    <testsuite
        name="riscv64-generic-qemu:phoenix-rtos-tests/fs/fat"
        tests="1"
        failures="0"
        errors="0"
        skipped="0"
        time="7.000"
        timestamp="2025-08-04T02:44:00"
        hostname="rpi"
    >
        <testcase
            name="riscv64-generic-qemu:phoenix-rtos-tests/fs/fat.test_fat_mount"
            classname="phoenix-rtos-tests/fs/fat"
            time="7.0"
        />
    </testsuite>
    <testsuite
        name="ia32-generic-pc:phoenix-rtos-tests/libc/single_case_suite"
        tests="1"
        failures="0"
        errors="0"
        skipped="0"
        time="6.000"
        timestamp="2025-08-04T02:45:00"
        hostname="rpi"
    >
        <testcase
            name="ia32-generic-pc:phoenix-rtos-tests/libc/single_case_suite.single_case_suite"
            classname="phoenix-rtos-tests/libc/single_case_suite"
            time="6.0"
        />
    </testsuite>
</testsuites>
"""
    old_xml_path.write_text(old_xml_content)
    new_xml_path.write_text(new_xml_content)

    return str(old_xml_path), str(new_xml_path)


@pytest.fixture(scope="module")
def mock_xml_file_no_tests(tmp_path_factory):
    """Creates mock XML file with no tests."""
    tmp_dir = tmp_path_factory.mktemp("data")
    xml_path = tmp_dir / "empty.xml"

    xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<testsuites
    tests="0"
    failures="0"
    errors="0"
    skipped="0"
    time="0.000"
>
</testsuites>
"""
    xml_path.write_text(xml_content)

    return str(xml_path)


@pytest.fixture(scope="module")
def mock_benchmark_file(tmp_path_factory):
    """Creates a mock benchmark.yml file for filter testing."""
    benchmark_dir = tmp_path_factory.mktemp("benchmark")
    benchmark_file = benchmark_dir / "benchmark.yml"
    benchmark_content = """
micro_kernel_benchmark:
  location: "phoenix-rtos-tests/kernel/"
  suites:
    common:
      cases:
        - "test_A_regression"
"""
    benchmark_file.write_text(benchmark_content)
    return str(benchmark_file)


@pytest.fixture(scope="module")
def mock_nonexistent_benchmark_file(tmp_path_factory):
    """Creates a mock benchmark.yml file for filter testing."""
    benchmark_dir = tmp_path_factory.mktemp("benchmark")
    benchmark_file = benchmark_dir / "nonexistent_benchmark.yml"
    benchmark_content = """
nonexistent_benchmark:
  location: "nonexistent/nothing/"
  suites:
    example:
      cases:
        - "some_case"
"""
    benchmark_file.write_text(benchmark_content)
    return str(benchmark_file)


@pytest.fixture
def unprocessed_args(mock_xml_files):
    """Creates an unprocessed args object from argparse, containing file paths."""
    old_file, new_file = mock_xml_files
    sys.argv = ["cmp.py", old_file, new_file]
    return cmp.parse_args()


@pytest.fixture
def base_args(unprocessed_args):
    """Creates a base, fully processed Args object for tests that don't need to manipulate arguments."""
    return cmp.process_args(unprocessed_args)


@pytest.fixture
def compared_data(base_args):
    """Runs the main comparison logic with base arguments and returns the resulting data structure."""

    return cmp.compare_level(base_args.file_old, base_args.file_new, base_args)


def find_node_by_path(nodes, path_parts):
    """Finds and returns a node in compared tests structure."""
    if not path_parts:
        return None
    current_name = path_parts[0]
    remaining_path = path_parts[1:]
    for node in nodes:
        if node.name == current_name:
            if not remaining_path:
                return node
            if isinstance(node, (cmp.ComparedAgregateNode, cmp.ComparedStatusNode)):
                return find_node_by_path(node.children, remaining_path)
    return None


def test_find_fails(base_args):
    """Tests that find_fails correctly identifies all failing tests."""
    args = copy.deepcopy(base_args)
    fails_new = cmp.find_fails(args.file_new, args)
    assert cmp.count_fails(fails_new) == 4
    assert "ia32-generic-pc" in fails_new
    assert "phoenix-rtos-tests/micropython/" in fails_new["ia32-generic-pc"]
    assert "core" in fails_new["ia32-generic-pc"]["phoenix-rtos-tests/micropython/"]
    assert "test_ok_to_fail" in fails_new["ia32-generic-pc"]["phoenix-rtos-tests/micropython/"]["core"]
    assert "test_skip_to_fail" in fails_new["ia32-generic-pc"]["phoenix-rtos-tests/micropython/"]["core"]
    assert "test_fail_to_fail" in fails_new["ia32-generic-pc"]["phoenix-rtos-tests/micropython/"]["core"]
    assert "ia32-generic-pc" in fails_new
    assert "phoenix-rtos-tests/libc/" in fails_new["ia32-generic-pc"]
    assert "stdlib" in fails_new["ia32-generic-pc"]["phoenix-rtos-tests/libc/"]
    assert "test_rand_fail" in fails_new["ia32-generic-pc"]["phoenix-rtos-tests/libc/"]["stdlib"]


def test_find_missing_and_status_changes(compared_data):
    """Tests all status and structural changes."""
    results = cmp.find_missing(compared_data)

    ok_to_fail_node = find_node_by_path(
        results, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core", "test_ok_to_fail"]
    )
    assert ok_to_fail_node.status_old == cmp.Status.OK and ok_to_fail_node.status_new == cmp.Status.FAIL

    fail_to_ok_node = find_node_by_path(
        results, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core", "test_fail_to_ok"]
    )
    assert fail_to_ok_node.status_old == cmp.Status.FAIL and fail_to_ok_node.status_new == cmp.Status.OK

    fail_to_skip_node = find_node_by_path(
        results, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core", "test_fail_to_skip"]
    )
    assert fail_to_skip_node.status_old == cmp.Status.FAIL and fail_to_skip_node.status_new == cmp.Status.SKIP

    skip_to_fail_node = find_node_by_path(
        results, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core", "test_skip_to_fail"]
    )
    assert skip_to_fail_node.status_old == cmp.Status.SKIP and skip_to_fail_node.status_new == cmp.Status.FAIL

    ok_to_skip_node = find_node_by_path(
        results, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core", "test_ok_to_skip"]
    )
    assert ok_to_skip_node.status_old == cmp.Status.OK and ok_to_skip_node.status_new == cmp.Status.SKIP

    skip_to_ok_node = find_node_by_path(
        results, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core", "test_skip_to_ok"]
    )
    assert skip_to_ok_node.status_old == cmp.Status.SKIP and skip_to_ok_node.status_new == cmp.Status.OK

    assert (
        find_node_by_path(results, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core", "test_skip_to_skip"])
        is None
    )

    assert (
        find_node_by_path(results, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core", "test_fail_to_fail"])
        is None
    )


def test_generate_status_rows_output(base_args, compared_data):
    """Tests the text output for status differences, including verbosity."""
    args = copy.deepcopy(base_args)
    args.status_diff = True
    args.verbose = 3
    status_diff = cmp.find_missing(compared_data)
    rows = cmp.generate_status_rows(status_diff, args)

    def find_row_by_name(rows, name_substring):
        for row in rows:
            if hasattr(row, "name") and name_substring in row.name:
                return row
        return None

    row1 = find_row_by_name(rows, "test_status_change")
    assert row1 is not None
    assert row1.status_old == cmp.Status.OK
    assert row1.status_new == cmp.Status.SKIP

    row2 = find_row_by_name(rows, "test_C_removed")
    assert row2 is not None
    assert row2.status_old == cmp.Status.OK
    assert row2.status_new == cmp.Status.NONE

    args.verbose = 0
    rows_v0 = cmp.generate_status_rows(status_diff, args)

    assert len(rows_v0) < len(rows)
    assert find_row_by_name(rows_v0, "test_status_change") is None
    assert find_row_by_name(rows_v0, "armv7a7-imx6ull-evk") is None
    assert find_row_by_name(rows_v0, "riscv64-generic-qemu") is not None


def test_generate_time_rows_output(base_args, compared_data):
    """Tests the text output for time differences, including thresholds, colors, and filtering."""
    args = copy.deepcopy(base_args)

    args.verbose = 3
    args.threshold_filter = False
    rows = cmp.generate_time_rows(compared_data.children, args)

    def find_row_by_name(rows, name_substring):
        for row in rows:
            if hasattr(row, "name") and name_substring in row.name:
                return row
        return None

    row_A = find_row_by_name(rows, "test_A_regression")
    assert row_A is not None
    assert row_A.time_old == "10.000"
    assert row_A.time_new == "15.000"
    assert row_A.difference == "+5.000"
    assert row_A.percentage == "+50.00"
    assert row_A.color == cmp.ESCAPE_RED

    row_B = find_row_by_name(rows, "test_B_improvement")
    assert row_B is not None
    assert row_B.time_old == "20.000"
    assert row_B.time_new == "15.000"
    assert row_B.difference == "-5.000"
    assert row_B.percentage == "-25.00"
    assert row_B.color == cmp.ESCAPE_GREEN

    row_no_change = find_row_by_name(rows, "test_no_change")
    assert row_no_change is not None
    assert row_no_change.difference == "+0.000"
    assert row_no_change.percentage == "+0.00"
    assert row_no_change.color == ""

    args.threshold_filter = True

    args.threshold_relative = 40.0
    args.threshold_absolute = 4.0

    rows_filtered = cmp.generate_time_rows(compared_data.children, args)

    assert find_row_by_name(rows_filtered, "test_A_regression") is not None

    assert find_row_by_name(rows_filtered, "test_B_improvement") is None

    assert find_row_by_name(rows_filtered, "test_no_change") is None

    args.verbose = 0
    args.threshold_relative = 60.0
    rows_filtered_v0 = cmp.generate_time_rows(compared_data.children, args)
    assert find_row_by_name(rows_filtered_v0, "armv7a7-imx6ull-evk") is None

    args.threshold_relative = 20.0
    rows_filtered_v0_low = cmp.generate_time_rows(compared_data.children, args)
    assert find_row_by_name(rows_filtered_v0_low, "armv7a7-imx6ull-evk") is None


def test_benchmark_filtering(unprocessed_args, mock_benchmark_file):
    """Tests filtering by --benchmark file."""
    args = copy.deepcopy(unprocessed_args)

    args.benchmark_files = [mock_benchmark_file]
    processed_args = cmp.process_args(args)

    filtered_data = cmp.compare_level(processed_args.file_old, processed_args.file_new, processed_args)

    included_path = ["armv7a7-imx6ull-evk", "phoenix-rtos-tests/kernel/", "common", "test_A_regression"]
    included_node = find_node_by_path(filtered_data.children, included_path)
    assert included_node is not None, "Node specified in benchmark.yml should be present"

    excluded_sibling_path = ["armv7a7-imx6ull-evk", "phoenix-rtos-tests/kernel/", "common", "test_B_improvement"]
    excluded_sibling_node = find_node_by_path(filtered_data.children, excluded_sibling_path)
    assert excluded_sibling_node is None, "Node not in benchmark.yml should be absent"

    excluded_target_node = find_node_by_path(filtered_data.children, ["ia32-generic-pc"])
    assert excluded_target_node is None, "Target not in benchmark.yml should be absent"


def test_threshold_zero_filters_all(base_args, compared_data):
    """Tests filtering with high relative threshold and zero absolute threshold."""
    args = copy.deepcopy(base_args)
    args.threshold_relative = 1000
    args.threshold_absolute = 0
    args.threshold_filter = True
    rows = cmp.generate_time_rows(compared_data.children, args)
    assert rows == []


def test_only_new_file_present(mock_xml_file_no_tests, unprocessed_args):
    """Tests comparison when old file has no tests but new file does."""
    raw_args = copy.deepcopy(unprocessed_args)
    raw_args.file_old = mock_xml_file_no_tests
    args = cmp.process_args(raw_args)
    results = cmp.compare_level(args.file_old, args.file_new, args)
    assert results.only_new == ["armv7a7-imx6ull-evk", "ia32-generic-pc", "riscv64-generic-qemu"]
    assert results.only_old == []
    assert results.children == []


def test_benchmark_file_with_no_matches(unprocessed_args, mock_nonexistent_benchmark_file):
    """Tests if results are empty when benchmark file matches nothing."""
    raw_args = copy.deepcopy(unprocessed_args)
    raw_args.benchmark_files = [mock_nonexistent_benchmark_file]
    args = cmp.process_args(raw_args)
    results = cmp.compare_level(args.file_old, args.file_new, args)
    assert results.children == []


@pytest.mark.parametrize(
    "filter_attr,filter_value,should_find",
    [
        ("targets", ["ia32-generic-pc"], True),
        ("targets", ["non-existent"], False),
        ("locations", ["phoenix-rtos-tests/micropython/"], True),
        ("locations", ["phoenix-rtos-tests/non-existent/"], False),
        ("suites", ["core"], True),
        ("suites", ["non-existent"], False),
        ("cases", ["test_ok_to_fail"], True),
        ("cases", ["non-existent"], False),
    ],
)
def test_filtering_by_individual_args(base_args, filter_attr, filter_value, should_find):
    """Tests that each filtering arg includes/excludes nodes correctly."""
    args = copy.deepcopy(base_args)
    setattr(args, filter_attr, filter_value)
    results = cmp.compare_level(args.file_old, args.file_new, args)
    found_node = find_node_by_path(
        results.children, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core", "test_ok_to_fail"]
    )
    assert (found_node is not None) == should_find


def test_single_case_suite_is_not_expanded(base_args, compared_data):
    """
    Tests that a suite with a single test case that matches the suite's name
    is correctly identified as a 'single_case' and is not expanded in the output.
    """
    args = copy.deepcopy(base_args)
    args.verbose = 3

    path = ["ia32-generic-pc", "phoenix-rtos-tests/libc/", "single_case_suite"]
    node = find_node_by_path(compared_data.children, path)
    assert node is not None, "The 'single_case_suite' node was not found."
    assert node.single_case is True, "The suite should be flagged as 'single_case'."

    rows = cmp.generate_time_rows(compared_data.children, args)

    suite_name_to_find = " -single_case_suite"
    suite_row_found = any(hasattr(row, "name") and row.name == suite_name_to_find for row in rows)
    assert suite_row_found, f"The collapsed row for '{suite_name_to_find.strip()}' should be displayed."

    case_name_to_find = "  -single_case_suite"
    case_row_found = any(hasattr(row, "name") and row.name == case_name_to_find for row in rows)
    assert not case_row_found, "The inner test case for a 'single_case' suite should not be expanded."


def test_suite_filtered_to_one_case_is_expanded(unprocessed_args):
    """
    Tests that a suite with multiple cases, when filtered down to a single case
    that does not match the 'single_case' naming criteria, is expanded correctly.
    """
    args = copy.deepcopy(unprocessed_args)
    args.cases = ["test_A_regression"]
    args.verbose = 3
    processed_args = cmp.process_args(args)

    compared_data = cmp.compare_level(processed_args.file_old, processed_args.file_new, processed_args)

    path = ["armv7a7-imx6ull-evk", "phoenix-rtos-tests/kernel/", "common"]
    node = find_node_by_path(compared_data.children, path)
    assert node is not None, "The 'common' suite node was not found."
    assert len(node.children) == 1, "The 'common' suite should be filtered to one child case."
    assert node.single_case is False, "The suite should not be flagged as 'single_case' as the case name differs."

    rows = cmp.generate_time_rows(compared_data.children, args)
    suite_row_found = any(hasattr(row, "name") and row.name == " -common" for row in rows)
    case_row_found = any(hasattr(row, "name") and row.name == "  -test_A_regression" for row in rows)
    assert suite_row_found, "The suite row for 'common' should be displayed."
    assert case_row_found, "The inner test case 'test_A_regression' should be expanded and displayed."


@pytest.fixture(scope="module")
def mock_xml_edge_cases(tmp_path_factory):
    """Creates mock XML files for testing calculation and parsing edge cases."""
    tmp_dir = tmp_path_factory.mktemp("data_edge")
    old_xml_path = tmp_dir / "old_edge.xml"
    new_xml_path = tmp_dir / "new_edge.xml"

    # Old file: baseline times for edge cases
    old_xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
    <testsuite
        name="edge:calculations/special"
        tests="4"
        time="10.020">
        <testcase
            name="edge:calculations/special.zero_time_start"
            classname="calculations/special"
            time="0.0"
        />
        <testcase
            name="edge:calculations/special.boundary_threshold"
            classname="calculations/special"
            time="10.0"
        />
        <testcase
            name="edge:calculations/special.opposing_threshold"
            classname="calculations/special"
            time="0.02"
        />
    </testsuite>
    <testsuite
        name="edge:empty/suite"
        tests="0"
        time="0.0"
    />
</testsuites>
"""
    # New file: changed times to trigger edge case logic
    new_xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
    <testsuite
        name="edge:calculations/special"
        tests="4"
        time="11.130"
    >
        <testcase
            name="edge:calculations/special.zero_time_start"
            classname="calculations/special"
            time="0.1"
        />
        <testcase
            name="edge:calculations/special.boundary_threshold"
            classname="calculations/special"
            time="11.0"
        />
        <testcase
            name="edge:calculations/special.opposing_threshold"
            classname="calculations/special"
            time="0.03"
        />
    </testsuite>
    <testsuite
        name="edge:empty/suite"
        tests="0"
        time="0.0"
    />
</testsuites>
"""
    old_xml_path.write_text(old_xml_content)
    new_xml_path.write_text(new_xml_content)

    return str(old_xml_path), str(new_xml_path)


@pytest.fixture(scope="module")
def mock_xml_malformed_suite(tmp_path_factory):
    """Creates a mock XML file with a malformed suite name (missing colon)."""
    tmp_dir = tmp_path_factory.mktemp("data_malformed")
    xml_path = tmp_dir / "malformed.xml"
    xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
    <testsuite name="this-suite-has-no-colon">
        <testcase
            name="some.case"
            time="1.0"
        />
    </testsuite>
    <testsuite name="ia32-generic-pc:normal/suite">
        <testcase
            name="another.case"
            time="2.0"
        />
    </testsuite>
</testsuites>
"""
    xml_path.write_text(xml_content)
    return str(xml_path)


@pytest.fixture(scope="module")
def mock_invalid_yaml_file(tmp_path_factory):
    """Creates a benchmark.yml file with invalid YAML syntax."""
    benchmark_dir = tmp_path_factory.mktemp("benchmark_invalid")
    benchmark_file = benchmark_dir / "invalid_benchmark.yml"
    # This content is invalid because a tab character is used for indentation.
    benchmark_content = "invalid_benchmark:\n\tsuites: ['some_suite']"
    benchmark_file.write_text(benchmark_content)
    return str(benchmark_file)


def test_calculation_with_zero_time_start(mock_xml_edge_cases):
    """Tests percentage calculation where the old time was zero."""
    old_file, new_file = mock_xml_edge_cases
    sys.argv = ["cmp.py", old_file, new_file, "-vvv"]
    args = cmp.process_args(cmp.parse_args())
    compared_data = cmp.compare_level(args.file_old, args.file_new, args)
    rows = cmp.generate_time_rows(compared_data.children, args)

    zero_time_row = next((r for r in rows if hasattr(r, "name") and "zero_time_start" in r.name), None)
    assert zero_time_row is not None, "Row for zero_time_start test case not found."
    assert zero_time_row.percentage == "-.--", "Percentage for 0 -> 0.1 should be a placeholder, not inf."


def test_calculation_on_threshold_boundary(mock_xml_edge_cases):
    """Tests that a change exactly equal to a threshold is considered UNCHANGED."""
    old_file, new_file = mock_xml_edge_cases
    sys.argv = ["cmp.py", old_file, new_file, "-vvv"]
    args = cmp.process_args(cmp.parse_args())
    compared_data = cmp.compare_level(args.file_old, args.file_new, args)
    rows = cmp.generate_time_rows(compared_data.children, args)

    boundary_row = next((r for r in rows if hasattr(r, "name") and "boundary_threshold" in r.name), None)
    assert boundary_row is not None, "Row for boundary_threshold test case not found."
    assert boundary_row.color == "", "Change on threshold boundary should not be colored red."


def test_calculation_with_opposing_thresholds(mock_xml_edge_cases):
    """Tests that a large relative change is ignored if the absolute change is too small."""
    old_file, new_file = mock_xml_edge_cases
    sys.argv = ["cmp.py", old_file, new_file, "-vvv"]
    args = cmp.process_args(cmp.parse_args())
    compared_data = cmp.compare_level(args.file_old, args.file_new, args)
    rows = cmp.generate_time_rows(compared_data.children, args)

    opposing_row = next((r for r in rows if hasattr(r, "name") and "opposing_threshold" in r.name), None)
    assert opposing_row is not None, "Row for opposing_threshold test case not found."
    assert opposing_row.color == "", "Change below absolute threshold should be uncolored."


def test_parsing_malformed_suite_name(mock_xml_malformed_suite, capsys):
    """Tests that a suite with a name missing a colon is skipped with a warning."""
    cmp.parse_xml(mock_xml_malformed_suite)
    captured = capsys.readouterr()
    assert "Warning:" in captured.out
    assert "Skipping suite with malformed name: 'this-suite-has-no-colon'" in captured.out


def test_parsing_empty_test_suite(mock_xml_edge_cases):
    """Tests that an empty test suite is parsed without errors and correctly pruned from the final report."""
    old_file, new_file = mock_xml_edge_cases
    sys.argv = ["cmp.py", old_file, new_file]
    args = cmp.process_args(cmp.parse_args())
    compared_data = cmp.compare_level(args.file_old, args.file_new, args)
    assert find_node_by_path(compared_data.children, ["edge", "empty/", "suite"]) is None


def test_handling_invalid_yaml_file(mock_invalid_yaml_file, unprocessed_args, capsys):
    """Tests that a user-friendly error is printed for a broken benchmark YAML file."""
    args = copy.deepcopy(unprocessed_args)
    args.benchmark_files = [mock_invalid_yaml_file]
    cmp.process_args(args)
    captured = capsys.readouterr()
    assert "Error parsing YAML file:" in captured.out


def test_combining_multiple_filters(base_args):
    """Tests that providing multiple different filter arguments correctly narrows the result."""
    args = copy.deepcopy(base_args)
    args.targets = ["ia32-generic-pc"]
    args.suites = ["core"]
    results = cmp.compare_level(args.file_old, args.file_new, args)

    found_node = find_node_by_path(results.children, ["ia32-generic-pc", "phoenix-rtos-tests/micropython/", "core"])
    assert found_node is not None
    not_found_node = find_node_by_path(results.children, ["ia32-generic-pc", "phoenix-rtos-tests/libc/", "stdlib"])
    assert not_found_node is None


def test_interaction_of_benchmark_and_cli_filters(unprocessed_args, mock_benchmark_file):
    """Tests that CLI filters are applied as a logical AND with benchmark file filters."""
    args = copy.deepcopy(unprocessed_args)
    args.benchmark_files = [mock_benchmark_file]
    args.cases = ["test_A_regression"]
    processed_args = cmp.process_args(args)
    results = cmp.compare_level(processed_args.file_old, processed_args.file_new, processed_args)
    node_found = find_node_by_path(
        results.children, ["armv7a7-imx6ull-evk", "phoenix-rtos-tests/kernel/", "common", "test_A_regression"]
    )
    assert node_found is not None, "Node should be found when CLI and benchmark filters overlap."

    args.cases = ["test_B_improvement"]
    processed_args = cmp.process_args(args)
    results = cmp.compare_level(processed_args.file_old, processed_args.file_new, processed_args)
    node_not_found = find_node_by_path(
        results.children, ["armv7a7-imx6ull-evk", "phoenix-rtos-tests/kernel/", "common", "test_B_improvement"]
    )
    assert node_not_found is None, "Node should NOT be found when it's not part of the benchmark."
    node_not_found = find_node_by_path(
        results.children, ["armv7a7-imx6ull-evk", "phoenix-rtos-tests/kernel/", "common", "test_A_improvement"]
    )
    assert node_not_found is None, "Node should NOT be found when CLI filter excludes it from benchmark scope."


@pytest.fixture(scope="module")
def mock_xml_location_empty_string(tmp_path_factory):
    """Creates mock XML files for testing handling of location being an empty string"""
    tmp_dir = tmp_path_factory.mktemp("data_edge")
    old_xml_path = tmp_dir / "old_edge.xml"
    new_xml_path = tmp_dir / "new_edge.xml"

    # Old file: baseline times for edge cases
    old_xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
    <testsuite
        name="ia32-generic-qemu:32q4uib253"
        tests="4"
        time="10.020">
        <testcase
            name="ia32-generic-qemu:32q4uib253.a"
            classname="32q4uib253"
            time="0.0"
        />
        <testcase
            name="ia32-generic-qemu:32q4uib253.b"
            classname="32q4uib253"
            time="10.0"
        />
        <testcase
            name="ia32-generic-qemu:32q4uib253.c"
            classname="32q4uib253"
            time="0.02"
        />
    </testsuite>
    <testsuite
        name="ia32-generic-qemu:flash"
        tests="1"
        time="0.02"
    >
        <testcase
            name="ia32-generic-qemu:flash"
            classname="flash"
            time="0.02"
        />
    </testsuite>
</testsuites>
"""
    # New file: changed times to trigger edge case logic
    new_xml_content = """<?xml version="1.0" encoding="UTF-8"?>
<testsuites>
    <testsuite
        name="ia32-generic-qemu:32q4uib253"
        tests="4"
        time="11.130"
    >
        <testcase
            name="ia32-generic-qemu:32q4uib253.a"
            classname="32q4uib253"
            time="0.1"
        />
        <testcase
            name="ia32-generic-qemu:32q4uib253.b"
            classname="32q4uib253"
            time="11.0"
        />
        <testcase
            name="ia32-generic-qemu:32q4uib253.c"
            classname="32q4uib253"
            time="0.03"
        />
    </testsuite>
    <testsuite
        name="ia32-generic-qemu:flash"
        tests="1"
        time="0.03"
    >
        <testcase
            name="ia32-generic-qemu:flash"
            classname="flash"
            time="0.03"
        />
    </testsuite>
</testsuites>
"""
    old_xml_path.write_text(old_xml_content)
    new_xml_path.write_text(new_xml_content)

    return str(old_xml_path), str(new_xml_path)


def test_location_empty_string(unprocessed_args, mock_xml_location_empty_string):
    """Tests handling of location being an empty string."""
    raw_args = copy.deepcopy(unprocessed_args)
    raw_args.file_old, raw_args.file_new = mock_xml_location_empty_string
    args = cmp.process_args(raw_args)
    args.verbose = 1

    compared_data = cmp.compare_level(args.file_old, args.file_new, args)
    rows = cmp.generate_time_rows(compared_data.children, args)

    def find_location_row_by_name(rows, name):
        for row in rows:
            if hasattr(row, "name") and row.name == ("-" + name):
                return row
        return None

    row1 = find_location_row_by_name(rows, "32q4uib253")
    assert row1 is not None, "When location is an empty string, it should be replaced with the suite name"

    row2 = find_location_row_by_name(rows, "flash")
    assert row2 is not None, "When location is an empty string, it should be replaced with the suite name"
