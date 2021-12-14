from psh.tools.common import CHARS, assert_created, assert_random
import psh.tools.psh as psh


def assert_visible(p, fname):
    files = psh.ls(p)
    msg = f"{fname} isn't visible in root directory"
    assert files == psh.ls(p, f'{fname}/..'), msg


def harness(p):
    testdir_name = 'test_mkdir_dir'
    psh.init(p)

    assert_created(p, 'mkdir', testdir_name)
    assert_visible(p, testdir_name)

    assert_created(p, 'mkdir', f'{testdir_name}/another_dir')
    assert_created(p, 'mkdir', f'{testdir_name}/' + ''.join(CHARS))
    assert_random(p, CHARS, 'mkdir', f'{testdir_name}/test_mkdir_random', count=20)

    psh.assert_cmd(p, 'mkdir /', 'mkdir: failed to create / directory')
    psh.assert_cmd(p, f'mkdir {testdir_name}', f'mkdir: failed to create {testdir_name} directory')
