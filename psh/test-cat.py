# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh cat command test
#
# Copyright 2021-2023 Phoenix Systems
# Author: Damian Loewnau, Mateusz Bloch
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh
from psh.tools.common import assert_dir_created, assert_file_created, assert_deleted_rec


ROOT_TEST_DIR = "test_cat_dir"


def assert_cat_h(p):
    help = ("Usage: cat [options] [files]", "  -h:  shows this help message")
    psh.assert_cmd(p, "cat -h", expected=help, result="success")


def assert_cat_err(p):
    fname = "nonexistentFile"

    statement = rf"cat: {fname}: ENOENT|No such file or directory"
    psh.assert_cmd(p, f"cat {fname}", expected=statement, result="fail", is_regex=True)

    statement = rf"cat: {ROOT_TEST_DIR}: EISDIR|Is a directory"
    psh.assert_cmd(p, f"cat {ROOT_TEST_DIR}", expected=statement, result="fail", is_regex=True)


def assert_cat_basic(p):
    fname = ROOT_TEST_DIR + "/textFile.txt"
    secFname = ROOT_TEST_DIR + "/textFile2.txt"
    nonExistName = "nonexistentFile"
    fspecial = ROOT_TEST_DIR + "/text!#$?@File.txt"

    text = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
    secText = "Curabitur eu velit a sem porttitor blandit."
    ascii = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,?!:;'()[]{}<>@# "

    assert_file_created(p, fname)
    assert_file_created(p, secFname)
    assert_file_created(p, fname)

    # Empty file
    psh.assert_cmd(p, f"cat {fname}", expected="")

    # Printing content of file
    psh.assert_cmd(p, f'echo "{text}" > {fname}')
    psh.assert_cmd(p, f"cat {fname}", expected=text)

    psh.assert_cmd(p, f'echo "{secText}" > {secFname}')
    psh.assert_cmd(p, f"cat {secFname}", expected=secText)

    # Concrate two text files
    psh.assert_cmd(p, f"cat {fname} {secFname}", expected=(text, secText))

    # Print basic characters
    psh.assert_cmd(p, f'echo "{ascii}" > {secFname}')
    psh.assert_cmd(p, f"cat {secFname}", expected=ascii)

    # Concrate existing file with non existing
    statement = rf"{text + psh.EOL}cat: {nonExistName}: ENOENT|No such file or directory"
    psh.assert_cmd(p, f"cat {fname} {nonExistName}", expected=statement, result="fail", is_regex=True)

    # Printing content of file with special characters
    psh.assert_cmd(p, f'echo "{text}" > {fspecial}')
    psh.assert_cmd(p, f"cat {fspecial}", expected=text)


@psh.run
def harness(p):
    assert_dir_created(p, ROOT_TEST_DIR)

    assert_cat_h(p)
    assert_cat_err(p)
    assert_cat_basic(p)

    assert_deleted_rec(p, ROOT_TEST_DIR)
