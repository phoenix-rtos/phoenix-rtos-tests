import random
import string

from collections import namedtuple

File = namedtuple('File', ['name', 'owner', 'is_dir'])


def ls(p, dir=''):
    files = []

    p.sendline(f'ls -la {dir}')
    p.expect_exact('ls -la')
    if dir:
        p.expect_exact(f' {dir}')

    p.expect_exact('\n')

    while True:
        idx = p.expect([r'\(psh\)\% ', r'(.*)(\r+)\n'])
        if idx == 0:
            break

        line = p.match.group(0)

        try:
            permissions, _, owner, _, _, _, _, _, name = line.split()
        except ValueError:
            print('psh mkdir: wrong ls output')
            return []

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

    idx = p.expect([r'\(psh\)\% ', r'mkdir: failed to create (.*) directory'])

    if idx == 1:
        # Failed, read one more time (psh)%
        p.expect_exact('(psh)% ')

    return idx != 1


def dir_present(dir, files):
    for file in files:
        if dir == file.name:
            if file.owner != 'root':
                print(f'psh ls: {dir} not owned by root')
                return False
            if not file.is_dir:
                print(f'psh ls: {dir} is not directory')
                return False

            break
    else:
        print(f'psh ls: failed to find {dir}')
        return False

    return True


def assert_dir_created(p, dir):
    if not mkdir(p, dir):
        print(f'psh mkdir: failed to create {dir}')
        return False

    path = dir.rsplit('/', 1)
    if len(path) == 2:
        dir = path[1]
        path = path[0]
    else:
        path = '/'

    files = ls(p, path)
    if not files:
        return False

    return dir_present(dir, files)


def random_dirs(p, pool, count=20):
    if not mkdir(p, '/random_dirs'):
        print('psh mkdir: failed to create /random_dirs')
        return False

    dirs = {''.join(random.choices(pool, k=random.randint(8, 16))) for _ in range(count)}
    for dir in dirs:
        if not mkdir(p, f'/random_dirs/{dir}'):
            print(f'psh mkdir: failed to create {dir}')
            return False

    files = ls(p, '/random_dirs')
    if not files:
        return False

    if len(files) > len(dirs) + 2:      # +2 because of . and ..
        print('psh ls: too many dirs')
        return False

    for dir in dirs:
        if not dir_present(dir, files):
            return False

    return True


def get_first_prompt(p):
    prompt = '\r\x1b[0J' + '(psh)% '

    got = p.read(len(prompt))
    if got != prompt:
        print(f'Expected:\n{prompt}\nGot:\n{got}')
        return False

    return True


def harness(p):
    chars = list(set(string.printable) - set(string.whitespace) - set('/'))

    if not get_first_prompt(p):
        return False

    if not assert_dir_created(p, 'example_dir'):
        return False

    files = ls(p)
    if not files:
        return False
    if not files == ls(p, 'example_dir/..'):
        return False

    if not assert_dir_created(p, 'example_dir/another_dir'):
        return False

    if not assert_dir_created(p, ''.join(chars)):
        return False

    # Random dirs
    if not random_dirs(p, chars):
        return False

    # Should fail
    if mkdir(p, '/'):
        return False

    if mkdir(p, '/example_dir'):
        return False

    return True
