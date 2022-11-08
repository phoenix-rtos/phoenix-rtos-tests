# Phoenix-RTOS
#
# phoenix-rtos-tests
#
# psh echo command test
#
# Copyright 2022 Phoenix Systems
# Author: Mateusz Niewiadomski
#
# This file is part of Phoenix-RTOS.
#
# %LICENSE%
#

import psh.tools.psh as psh


@psh.run
def harness(p):
    # Simple check
    psh.assert_cmd(p, 'echo', '\r+\n', 'empty echo fail', is_regex=True)
    psh.assert_cmd(p, 'echo loremipsum', 'loremipsum', 'simple text not echoed back')
    psh.assert_cmd(p, 'echo lorem ipsum dolor ales', 'lorem ipsum dolor ales', 'multiple argument not echoed back')

    # Return value check
    psh.assert_cmd(p, 'echo $?', '0', 'last exit status was not zero')
    p.sendline("exec")  # force execution of bad command
    psh.assert_cmd(p, 'echo $?', '-22', 'last exit status was not expected -22')

    # Mingle checks
    psh.assert_cmd(p, 'echo $ lorem ipsum', ' lorem ipsum', 'bad empty $ interpretation')
    psh.assert_cmd(p, 'echo ? lorem', '? lorem', 'bad empty ? interpretation')
    psh.assert_cmd(p, 'echo lorem $? $? ipsum $?', 'lorem 0 0 ipsum 0', 'multiple return values print fail')
    psh.assert_cmd(p, 'echo lorem $ $ ipsum', 'lorem   ipsum', 'echo bad $ interpretation')
    psh.assert_cmd(p, 'echo lorem $ipsum dolor$ales', 'lorem  dolor', 'echo bad $ interpretation')
    psh.assert_cmd(p, 'echo $lorem$? ipsum', '0 ipsum', 'echo can`t print two variables consecutively')

    # Eating '"' check
    psh.assert_cmd(p, 'echo "lorem ipsum"', 'lorem ipsum', 'bad \'"\' eating')
    psh.assert_cmd(p, 'echo "lorem ""$lorem $?"', 'lorem  0', 'bad \'"\' eating')
