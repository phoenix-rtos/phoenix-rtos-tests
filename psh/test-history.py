import string

import psh.tools.psh as psh
from psh.tools.randwrapper import TestRandom
from psh.tools.common import CHARS, get_rand_strings


def assert_sent(p, cmds, msg='', repeat=False):
    i = 2
    expected = ['   1  history']
    for cmd in cmds:
        for _ in range(3 if repeat else 1):
            p.sendline(cmd)
            psh.assert_prompt(p, msg=f'No prompt seen after passing the following command: {cmd}')

        line = f"{i:>4}  {cmd}"
        expected.append(line)
        i += 1
    expected.append(f'  {i}  history')
    psh.assert_cmd(p, 'history', expected=expected, result='success', msg=msg)


def assert_help(p):
    assert_msg = "Prompt hasn't been displayed properly, when calling with -h argument"
    psh.assert_prompt_after_cmd(p, 'history -h', result='success', msg=assert_msg)

    assert_msg = "Prompt hasn't been displayed properly, when calling with wrong arguments"
    psh.assert_prompt_after_cmd(p, 'history -z', result='fail', msg=assert_msg)
    psh.assert_prompt_after_cmd(p, 'history -xx', result='fail', msg=assert_msg)
    psh.assert_prompt_after_cmd(p, 'history @!$ qrw $*%', result='fail', msg=assert_msg)
    psh.assert_prompt_after_cmd(p, 'history ' + ''.join(CHARS), result='fail', msg=assert_msg)


def assert_clear(p):
    psh.assert_cmd(p, 'history -c', result='dont-check', msg='Unexpected output after history -c command')
    # we don't want to have echo $? in history
    psh.assert_cmd(p, 'history', expected='  1  history', result='dont-check', msg='Wrong history output after clear')


def assert_rand_cmds(p, psh_cmds, random_wrapper: TestRandom):
    cmds = get_rand_strings(CHARS, 50, random_wrapper)

    cmds = list(set(cmds) - set(psh_cmds))
    msg = 'Wrong history output after running random unknown commands'
    assert_sent(p, cmds, msg)


def assert_psh_cmds(p, psh_cmds, with_repeats=False):
    # psh prompt should appear after passing the following commands without arguments:
    cmds = ['bind', 'cat', 'cd', 'cp', 'date', 'df', 'dmesg', 'echo', 'exec', 'help',
            'history', 'hm', 'kill', 'ls', 'mem', 'mkdir', 'mount', 'nc', 'nslookup', 'ntpclient',
            'ping', 'ps', 'pwd', 'sync', 'sysexec', 'touch', 'tty', 'umount', 'uptime', 'wget']

    if with_repeats:
        msg = 'Wrong history output after running available psh commands multiple time'
        assert_sent(p, cmds, msg, repeat=True)
    else:
        msg = 'Wrong history output after running available psh commands'
        assert_sent(p, cmds, msg)


def assert_multiarg_cmds(p, random_wrapper: TestRandom):
    cmds = ['bind', 'cat', 'help', 'mem', 'uptime', 'unknown_cmd_1', 'unknown_cmd_2', 'unknown_cmd_3']
    for idx, cmd in enumerate(cmds):
        args = get_rand_strings(CHARS, 4, random_wrapper)
        cmds[idx] = ' '.join((cmd, *args))
    msg = 'Wrong history output after running commands with multiple arguments'
    assert_sent(p, cmds, msg=msg)


def assert_multicall_random(p, random_wrapper: TestRandom):
    cmds = get_rand_strings(CHARS, 20, random_wrapper)
    msg = 'Wrong history output after multiple time running same commands'
    assert_sent(p, cmds, msg, repeat=True)


def assert_empty(p):
    msg = 'Wrong history output after sending multiple whitespaces'
    p.sendline('')
    for _ in range(10):
        p.sendline(string.whitespace)
    psh.assert_cmd(p, 'history', expected='  1  history', msg=msg)


@psh.run
def harness(p):
    psh_cmds = psh.get_commands(p)
    random_wrapper = TestRandom(seed=1)

    assert_help(p)
    assert_clear(p)

    assert_rand_cmds(p, psh_cmds, random_wrapper)
    assert_clear(p)

    assert_psh_cmds(p, psh_cmds)
    assert_clear(p)

    assert_psh_cmds(p, psh_cmds, with_repeats=True)
    assert_clear(p)

    assert_multicall_random(p, random_wrapper)
    assert_clear(p)

    assert_empty(p)
    assert_clear(p)

    assert_multiarg_cmds(p, random_wrapper)
    assert_clear(p)
