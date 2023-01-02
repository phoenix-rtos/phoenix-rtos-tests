# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh help command test
#
# Copyright 2023 Phoenix Systems
# Author: Damian Loewnau
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh

# matches any line, for example - 'Available commands:'
INITIAL_LINE_PATTERN = rf'[^\r\n]+{psh.EOL}'
# matches command with any explanation + newline
HELP_LINE_PATTERN = rf'((\s*?)(?P<cmd>\w+)(\s+)-[^\r\n]+{psh.EOL})'


@psh.run
def harness(p):
    # asserts that at least 15 help commands are displayed in proper format
    psh.assert_cmd(p,
                   'help',
                   expected=rf'{INITIAL_LINE_PATTERN}{HELP_LINE_PATTERN}{{15,}}', is_regex=True)

    # psh help does not fail when passing arguments
    psh.assert_cmd(p,
                   'help -a arg1 --arg2 -arg3',
                   expected=rf'{INITIAL_LINE_PATTERN}{HELP_LINE_PATTERN}{{15,}}', is_regex=True)
