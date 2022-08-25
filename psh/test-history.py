import string

import psh.tools.psh as psh
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
    psh.assert_cmd(p, 'history', expected, msg)


def assert_help(p):
    assert_msg = "Prompt hasn't been displayed properly, when calling with -h argument"
    psh.assert_prompt_after_cmd(p, 'history -h', assert_msg)
    psh.assert_cmd_successed(p)

    assert_msg = "Prompt hasn't been displayed properly, when calling with wrong arguments"
    psh.assert_prompt_after_cmd(p, 'history -z', assert_msg)
    psh.assert_cmd_failed(p)
    # Temporary disabled, because of the following issue:
    # https://github.com/phoenix-rtos/phoenix-rtos-project/issues/521
    # psh.assert_prompt_after_cmd(p, 'history -xx', assert_msg)
    # psh.assert_cmd_failed(p)
    psh.assert_prompt_after_cmd(p, 'history @!$ qrw $*%', assert_msg)
    psh.assert_cmd_failed(p)
    # the order in CHARS list can be random
    # temporarily because of #521 issue we use sorted version
    psh.assert_prompt_after_cmd(p, 'history ' + ''.join(sorted(CHARS)), assert_msg)
    psh.assert_cmd_failed(p)


def assert_clear(p):
    psh.assert_cmd(p, 'history -c', msg='Unexpected output after history -c command')
    psh.assert_cmd(p, 'history', '  1  history', msg='Wrong history output after clear')


def assert_rand_cmds(p, psh_cmds):
    cmds = get_rand_strings(CHARS, 50)

    cmds = list(set(cmds) - set(psh_cmds))
    msg = 'Wrong history output after running random unknown commands'
    assert_sent(p, cmds, msg)


def assert_psh_cmds(p, psh_cmds, with_repeats=False):
    # perf, pm disabled because of issues, double history is printed as one
    omitted = {'edit', 'exit', 'perf', 'pm', 'top', 'reboot', 'history'}

    remaining = list(set(psh_cmds) - omitted)

    if with_repeats:
        msg = 'Wrong history output after running available psh commands multiple time'
        assert_sent(p, remaining, msg, repeat=True)
    else:
        msg = 'Wrong history output after running available psh commands'
        assert_sent(p, remaining, msg)


def assert_multiarg_cmds(p):
    cmds = ['bind', 'cat', 'help', 'mem', 'uptime', 'unknown_cmd_1', 'unknown_cmd_2', 'unknown_cmd_3']
    for idx, cmd in enumerate(cmds):
        args = get_rand_strings(CHARS, 4)
        cmds[idx] = ' '.join((cmd, *args))
    msg = 'Wrong history output after running commands with multiple arguments'
    assert_sent(p, cmds, msg)


def assert_multicall_random(p):
    cmds = get_rand_strings(CHARS, 20)
    msg = 'Wrong history output after multiple time running same commands'
    assert_sent(p, cmds, msg, repeat=True)


def assert_empty(p):
    msg = 'Wrong history output after sending multiple whitespaces'
    p.sendline('')
    for _ in range(10):
        p.sendline(string.whitespace)
    psh.assert_cmd(p, 'history', '  1  history', msg)


def harness(p):
    psh.init(p)

    psh_cmds = psh.get_commands(p)

    assert_help(p)
    assert_clear(p)

    assert_rand_cmds(p, psh_cmds)
    assert_clear(p)

    assert_psh_cmds(p, psh_cmds)
    assert_clear(p)

    assert_psh_cmds(p, psh_cmds, with_repeats=True)
    assert_clear(p)

    assert_multicall_random(p)
    assert_clear(p)

    assert_empty(p)
    assert_multiarg_cmds(p)
