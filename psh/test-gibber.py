# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh gibbering input testing
#
# Copyright 2021, 2022 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import string
import psh.tools.psh as psh

EXPECTED = 'Unknown command!'


def assert_printable(p):
    chars = list(set(string.printable) - set(string.whitespace))
    chars = ''.join(chars) + ' '
    msg = 'Wrong output when sending all printable characters'

    psh.assert_cmd(p, chars, EXPECTED, msg)


def assert_unprintable(p):
    # 3: ^C is terminating character for qemu, so we skip it
    # 4: ^D terminates psh process, so the another psh process is launched
    # 28: FS character skipped, to prevent EOF in reader buffer
    # 10: LF, double prompt causes double prompt
    # 12: NP, new page causes printing prompt
    # 13: CR with other codes like SO may cause sending /r/n

    chars = ''.join(chr(x) for x in sorted({*range(0, 32)} - {3, 4, 10, 12, 13, 28}))
    output = psh.assert_prompt_after_cmd(p, chars, "Prompt not seen when sending non-printable characters")
    assert output == '', f"Unexpected output when sending non-printable characters: {output!r}"


def assert_not_ascii(p):
    chars = ''.join(chr(x) for x in sorted({*range(128, 256)}))
    psh.assert_prompt_after_cmd(p, chars, "Prompt not seen when sending non-ascii characters")


def harness(p):
    psh.init(p)

    assert_printable(p)
    assert_unprintable(p)
    assert_not_ascii(p)
