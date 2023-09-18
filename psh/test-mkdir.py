import psh.tools.psh as psh
from psh.tools.randwrapper import TestRandom
from psh.tools.common import CHARS, assert_dir_created, assert_random_dirs, assert_deleted_rec

ROOT_TEST_DIR = 'test_mkdir_dir'


def assert_visible(p, fname):
    files = psh.ls(p)
    msg = f"{fname} isn't visible in root directory"
    assert files == psh.ls(p, f'{fname}/..'), msg


@psh.run
def harness(p):
    random_wrapper = TestRandom(seed=1)

    assert_dir_created(p, ROOT_TEST_DIR)
    assert_visible(p, ROOT_TEST_DIR)

    assert_dir_created(p, f'{ROOT_TEST_DIR}/another_dir')
    # there are some targets, where max fname length equals 64
    assert_dir_created(p, f'{ROOT_TEST_DIR}/' + ''.join(CHARS[:50]))
    assert_dir_created(p, f'{ROOT_TEST_DIR}/' + ''.join(CHARS[50:]))

    assert_random_dirs(p, CHARS, f'{ROOT_TEST_DIR}/random/', random_wrapper, count=20)

    psh.assert_prompt_after_cmd(p, 'mkdir /', result='fail')
    psh.assert_prompt_after_cmd(p, f'mkdir {ROOT_TEST_DIR}', result='fail')

    assert_deleted_rec(p, ROOT_TEST_DIR)
