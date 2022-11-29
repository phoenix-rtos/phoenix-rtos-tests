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
    psh.assert_cmd(p, 'echo', expected='\r+\n', result='success', msg='empty echo fail', is_regex=True)
    psh.assert_cmd(p, 'echo loremipsum', expected='loremipsum', result='success', msg='simple text not echoed back')
    msg = 'multiple argument not echoed back'
    psh.assert_cmd(p, 'echo lorem ipsum dolor ales', expected='lorem ipsum dolor ales', result='success', msg=msg)

    # Return value check
    psh.assert_cmd(p, 'echo $?', expected='0', result='success', msg='last exit status was not zero')
    p.sendline("exec")  # force execution of bad command
    psh.assert_cmd(p, 'echo $?', expected='-22', result='success', msg='last exit status was not expected -22')

    # Mingle checks
    psh.assert_cmd(p, 'echo $ lorem ipsum', expected=' lorem ipsum', result='success', msg='bad empty $ interpretation')
    psh.assert_cmd(p, 'echo ? lorem', expected='? lorem', result='success', msg='bad empty ? interpretation')
    msg = 'multiple return values print fail'
    psh.assert_cmd(p, 'echo lorem $? $? ipsum $?', expected='lorem 0 0 ipsum 0', result='success', msg=msg)
    psh.assert_cmd(p, 'echo lorem $ $ ipsum',
                   expected='lorem   ipsum',
                   result='success',
                   msg='echo bad $ interpretation')

    psh.assert_cmd(p, 'echo lorem $ipsum dolor$ales',
                   expected='lorem  dolor',
                   result='success',
                   msg='echo bad $ interpretation')

    msg = 'echo can`t print two variables consecutively'
    psh.assert_cmd(p, 'echo $lorem$? ipsum', expected='0 ipsum', result='success', msg=msg)

    # Eating '"' check
    psh.assert_cmd(p, 'echo "lorem ipsum"', expected='lorem ipsum', result='success', msg='bad \'"\' eating')
    psh.assert_cmd(p, 'echo "lorem ""$lorem $?"', expected='lorem  0', result='success', msg='bad \'"\' eating')
