import psh.tools.psh as psh
import pexpect
from time import sleep


TESTS_SOURCE_DIR = '_build/ia32-generic/azure_sdk/azure-iot-sdk-c/cmake/c-utility/tests'


def ls_simple(pexpect_proc, dir=''):
    ''' Returns list of file names from the specified directory'''
    files = []
    err_msg = f"ls: can't access {dir}: no such file or directory"
    pexpect_proc.sendline(f'ls {dir}')
    idx = pexpect_proc.expect_exact([f'ls {dir}', pexpect.TIMEOUT, pexpect.EOF])
    assert idx == 0, f"ls {dir} command hasn't been sent properly"

    while True:
        idx = pexpect_proc.expect([
            r'\d*m(\w+)(\s+)',
            err_msg,
            r'\(psh\)\% '])
        if idx == 0:
            groups = pexpect_proc.match.groups()
            files.append(groups[0])
        elif idx == 1:
            assert False, err_msg
        elif idx == 2:
            break

    return files

def harness(p):
    # wait for net setup
    sleep(1)
    psh.init(p)
    c_utility_tests = []
    c_utility_tests = ls_simple(p, 'az')
    failed = []

    c_utility_screwed = []
    c_utility_screwed.append('condition_ut_exe')
    c_utility_screwed.append('http_proxy_io_ut_exe')
    c_utility_screwed.append('uws_client_ut_exe')

    print(f'The total number of tests: {len(c_utility_tests)}')
    print(c_utility_tests)
    print('-------')
    for test in c_utility_tests:
        if test in c_utility_screwed:
            continue
        p.sendline(f'/az/{test}')
        p.expect_exact(f'/az/{test}')
        val = p.expect([r'0 failed', r'\d+ failed'])
        if val == 1:
            failed.append(test)
        p.expect_exact('(psh)% ')

    failed_nr = len(failed) + len(c_utility_screwed)
    print(f'Failed tests: {failed_nr}')
    print(failed)
