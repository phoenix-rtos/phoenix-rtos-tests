# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh date command test
#
# Copyright 2022, 2023 Phoenix Systems
# Author: Mateusz Niewiadomski, Mateusz Bloch
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh

DFLT_DATE_REG = r"\w{3}, \d{2} \w{3} (?:\d{2}|\d{4}),? \d{2}:\d{2}:\d{2}(?:\w{3}|\w{4})?\r+\n"
SPEC_DATE_REG = r"Sun, 13 Sep (?:20|2020),? 12:26:4\d(?:\w{3}|\w{4})?\r+\n"  # arbitrary date of epoch 1600000000 GMT
HUGE_DATE_REG = r"Tue, 19 Jan (?:38|2038),? 03:14:0\d(?:\w{3}|\w{4})?\r+\n"  # arbitrary date of epoch 2147483647 GMT
ZERO_DATE_REG = r"Thu, 01 Jan (?:70|1970),? 00:00:0\d(?:\w{3}|\w{4})?\r+\n"  # arbitrary date of epoch 0 GMT


# Utility functions
def invalid_format(format):
    return f"date: invalid format '{str(format)}'"


def invalid_date(format):
    return f"date: invalid date '{format}'"


def unrec_arg(arg):
    return f"Unrecognized argument: {arg}"


# Test parts
def test_corrects(p):
    """Testing correct `date` commands"""

    # Test help
    psh.assert_cmd(p, "date -h", expected=r"Usage(.*?)FORMAT(.*?)\r+\n", is_regex=True)

    # Test printing and formatting
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date +%Y", expected=r"\b(19|20)\d{2}\r+\n", is_regex=True)
    psh.assert_cmd(p, "date +%H:%M:%Sformat", expected=r"\d{2}:\d{2}:\d{2}format\r+\n", is_regex=True)

    # Test setting to value
    psh.assert_cmd(p, "date -s @1600000000", expected=SPEC_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date", expected=SPEC_DATE_REG, is_regex=True)

    # Test setting to 0
    psh.assert_cmd(p, "date -s @0", expected=ZERO_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date", expected=ZERO_DATE_REG, is_regex=True)

    # Test date parsing
    psh.assert_cmd(p, "date -d @1600000000", expected=SPEC_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # Test date parsing for 0
    psh.assert_cmd(p, "date -d @0", expected=ZERO_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)


def test_incorrect_dateprint(p):
    """Test incorrect or rare commandlines for printing date"""

    # Incorrect FORMAT passed when printing date
    psh.assert_cmd(p, "date operand1", expected=invalid_format("operand1"), result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # Incorrect FORMAT passed when printing date
    psh.assert_cmd(p, "date +operand1", expected="operand1")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # too many arguments passed to print, should print first redundant
    psh.assert_cmd(p, "date operand1 operand2 operand3", expected=unrec_arg("operand2"), result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # nonexistent format '%k' passed to print
    psh.assert_cmd(p, "date +%Y%k%Y", expected=r"\b(19|20)\d{2}%k(19|20)\d{2}\r+\n", is_regex=True)
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)


def test_incorrect_datewrite(p):
    """Test incorrect commandlines for setting date (no edge cases)"""

    # No argument passed
    psh.assert_cmd(p, "date -s", expected="date: option requires an argument -- s", result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # invalid time value
    psh.assert_cmd(p, "date -s 123456789operand1", expected=invalid_date("123456789operand1"), result="fail")

    # too many arguments
    psh.assert_cmd(p, "date -s 1600000000 +format operand1", expected=unrec_arg("operand1"), result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # no integer number
    psh.assert_cmd(p, "date -s @1600000000.1234567890", expected=invalid_date("@1600000000.1234567890"), result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # Issue: #801 https://github.com/phoenix-rtos/phoenix-rtos-project/issues/801
    return

    # negative number
    psh.assert_cmd(p, "date -s @-1600000000", expected=SPEC_DATE_REG, result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)


def test_incorrect_dateparse(p):
    # No argument passed
    psh.assert_cmd(p, "date -d", expected="date: option requires an argument -- d", result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # too many arguments
    psh.assert_cmd(p, "date -d @1600000000 +format operand1", expected=unrec_arg("operand1"), result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # invalid time value
    psh.assert_cmd(p, "date -d @1600000000operand1", expected=invalid_date("@1600000000operand1"), result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # no '@' sign
    psh.assert_cmd(p, "date -d 1600000000", expected=invalid_date("1600000000"), result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # no integer number
    psh.assert_cmd(p, "date -d @1600000000.1234567890", expected=invalid_date("@1600000000.1234567890"), result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # Issue: #801 https://github.com/phoenix-rtos/phoenix-rtos-project/issues/801
    return

    # negative number
    psh.assert_cmd(p, "date -d @-1600000000", expected=SPEC_DATE_REG, result="fail")
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)


def test_edges(p):
    """Test edge cases for epoch value passed to `date -s epoch"""

    # maximum value for int32
    psh.assert_cmd(p, "date -s @2147483647", expected=HUGE_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date", expected=HUGE_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date +%Y", expected="2038")
    psh.assert_cmd(p, "date -s @0", expected=DFLT_DATE_REG, is_regex=True)

    psh.assert_cmd(p, "date -d @2147483647", expected=HUGE_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # beyond int32 value
    psh.assert_cmd(p, "date -s @2147483648", expected=HUGE_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date", expected=HUGE_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date +%Y", expected="2038")
    psh.assert_cmd(p, "date -s @0", expected=DFLT_DATE_REG, is_regex=True)

    psh.assert_cmd(p, "date -d @2147483648", expected=HUGE_DATE_REG, is_regex=True)
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)

    # Issue: #370 https://github.com/phoenix-rtos/phoenix-rtos-project/issues/370
    return

    psh.assert_cmd(
        p,
        "date -s @10000000000000000",
        expected=r"Sat, 25 Jan (?:55|316889355),? \d{2}:\d{2}:\d{2}(?:\w{3}|\w{4})?\r+\n",
        is_regex=True,
    )
    psh.assert_cmd(p, "date", expected=r"Sat, 25 Jan (?:55|316889355),? \d{2}:\d{2}:\d{2}\r+\n", is_regex=True)
    psh.assert_cmd(p, "date +%Y", expected="316889355")
    psh.assert_cmd(p, "date -s @0", DFLT_DATE_REG, is_regex=True)

    psh.assert_cmd(
        p,
        "date -d @10000000000000000",
        expected=r"Sat, 25 Jan 55 \d{2}:\d{2}:\d{2}(?:\w{3}|\w{4})?\r+\n",
        is_regex=True,
    )
    psh.assert_cmd(p, "date", expected=DFLT_DATE_REG, is_regex=True)


@psh.run
def harness(p):
    test_corrects(p)
    test_incorrect_dateprint(p)
    test_incorrect_datewrite(p)
    test_incorrect_dateparse(p)
    test_edges(p)
