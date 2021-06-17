import random
import string

from collections import namedtuple

from psh.tools import run_psh, assert_only_prompt

File = namedtuple('File', ['name', 'owner', 'is_dir'])


def ls(p, dir=''):
    p.sendline(f'ls -la {dir}')
    p.expect_exact('ls -la')
    if dir:
        p.expect_exact(f' {dir}')

    p.expect_exact('\n')

    files = []
    while True:
        idx = p.expect([r'\(psh\)\% ', r'(.*?)(\r+)\n'])
        if idx == 0:
            break

        line = p.match.group(0)

        try:
            permissions, _, owner, _, _, _, _, _, name = line.split()
        except ValueError:
            assert False, f'wrong ls output: {line}'

        # Name is printed with ascii escaped characters - remove them
        if name.startswith('\x1b[34m'):
            name = name[5:]
        if name.endswith('\x1b[0m'):
            name = name[:-4]

        f = File(name, owner, permissions[0] == 'd')
        files.append(f)

    return files


def mkdir(p, dir):
    p.sendline(f'mkdir {dir}')
    p.expect_exact(f'mkdir {dir}')
    p.expect_exact('\n')

    idx = p.expect([r'\(psh\)\% ', r'failed to create (.*) directory'])

    if idx == 1:
        # Failed, read one more time (psh)%
        p.expect_exact('(psh)% ')

    return idx != 1


def assert_dir_present(dir, files):
    for file in files:
        if dir == file.name:
            assert file.owner == 'root', f'{dir} not owned by root'
            assert file.is_dir, f'{dir} is not directory'
            break
    else:
        assert False, f'failed to find {dir}'


def assert_dir_created(p, dir):
    assert mkdir(p, dir), f'failed to create {dir}'

    path = dir.rsplit('/', 1)
    if len(path) == 2:
        dir = path[1]
        path = path[0]
    else:
        path = '/'

    files = ls(p, path)
    assert_dir_present(dir, files)


def assert_random_dirs(p, pool, count=20):
    assert mkdir(p, '/random_dirs'), 'failed to create /random_dirs'

    dirs = {''.join(random.choices(pool, k=random.randint(8, 16))) for _ in range(count)}
    for dir in dirs:
        assert mkdir(p, f'/random_dirs/{dir}'), f'failed to create {dir}'

    files = ls(p, '/random_dirs')
    # +2 because of . and ..
    assert len(files) == len(dirs) + 2, f'random dirs failed ({len(files)} != {len(dirs) + 2})'

    for dir in dirs:
        assert_dir_present(dir, files)


def assert_first_prompt(p):
    prompt = '\r\x1b[0J' + '(psh)% '

    got = p.read(len(prompt))
    assert got == prompt, f'Expected:\n{prompt}\nGot:\n{got}'


def harness(p):
    chars = list(set(string.printable) - set(string.whitespace) - set('/'))

    run_psh(p)
    assert_only_prompt(p)

    assert_dir_created(p, 'example_dir')
    files = ls(p)
    assert files == ls(p, 'example_dir/..')

    assert_dir_created(p, 'example_dir/another_dir')
    assert_dir_created(p, ''.join(chars))

    assert_random_dirs(p, chars)

    assert not mkdir(p, '/')
    assert not mkdir(p, '/example_dir')
