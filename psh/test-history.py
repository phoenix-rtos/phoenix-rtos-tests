from psh.tools.common import CHARS, get_rand_names
import psh.tools.psh as psh
import string


def assert_sent(p, cmds, msg='', multi=False):
    i = 2
    expected = ['   1  history']
    for cmd in cmds:
        for _ in range(3 if multi else 1):
            p.sendline(cmd)
            psh.assert_prompt(p, msg=f'No prompt seen after passing the following command: {cmd}')
        if i >= 10:
            line = f'  {i}  ' + cmd
        else:
            line = f'   {i}  ' + cmd
        expected.append(line)
        i = i + 1
    expected.append(f'  {i}  history')
    psh.assert_cmd(p, 'history', expected, msg)


def assert_help(p):
    help_msg = ['usage: history [options] or no args to print command history',
                '  -c:  clears command history',
                '  -h:  shows this help message']
    assert_msg = "Help message hasn't been displayed properly, when calling with -h argument"
    psh.assert_cmd(p, 'history -h', help_msg, assert_msg)

    assert_msg = "Help message hasn't been displayed properly, when calling with wrong arguments"
    psh.assert_cmd(p, 'history -z', ['history: illegal option -- z'] + help_msg, assert_msg)
    psh.assert_cmd(p, 'history @!$ qrw $*%', help_msg, assert_msg)
    psh.assert_cmd(p, 'history ' + ''.join(CHARS), help_msg, assert_msg)


def assert_clear(p):
    psh.assert_cmd(p, 'history -c', msg='Unexpected output after history -c command')
    psh.assert_cmd(p, 'history', '  1  history', msg='Wrong history output after clear')


def assert_rand_cmds(p):
    cmds = get_rand_names(CHARS, 50)
    msg = 'Wrong history output after running random unknown commands'
    assert_sent(p, cmds, msg)


def assert_psh_cmds(p, double=False):
    cmds = psh.get_commands(p)
    assert_clear(p)
    omitted = ['edit', 'exit', 'perf', 'top', 'reboot']
    for ocmd in omitted:
        cmds.remove(ocmd)

    if double:
        msg = 'Wrong history output after running available psh commands multiple time'
        assert_sent(p, cmds, msg, multi=True)
    else:
        msg = 'Wrong history output after running available psh commands'
        assert_sent(p, cmds, msg)


def assert_multiarg_cmds(p):
    args = get_rand_names(CHARS, 4)
    cmds = ['bind', 'cat', 'help', 'mem', 'uptime', 'unknown_cmd_1', 'unknown_cmd_2', 'unknown_cmd_3']
    assert_clear(p)
    for idx, cmd in enumerate(cmds):
        cmds[idx] = cmd + ' ' + ' '.join(args)
    msg = 'Wrong history output after running commands with multiple arguments'
    assert_sent(p, cmds, msg)


def assert_multi_call(p):
    assert_clear(p)
    cmds = get_rand_names(CHARS, 20)
    msg = 'Wrong history output after multiple time running same commands'
    assert_sent(p, cmds, msg, multi=True)
    assert_psh_cmds(p, double=True)


def assert_empty(p):
    assert_clear(p)
    msg = 'Wrong history output after sending multiple whitespaces'
    p.sendline('')
    for _ in range(10):
        p.sendline(string.whitespace)
    psh.assert_cmd(p, 'history', '  1  history', msg)


def harness(p):
    psh.init(p)

    assert_help(p)
    assert_clear(p)
    assert_rand_cmds(p)
    assert_psh_cmds(p)
    assert_multi_call(p)
    assert_empty(p)
    assert_multiarg_cmds(p)
