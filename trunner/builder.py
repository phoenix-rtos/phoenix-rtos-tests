import logging
import os
import shutil
import subprocess
import sys

from .config import PHRTOS_PROJECT_DIR, DEFAULT_TARGETS


class TargetBuilder:
    """A base class that builds image needed to run all test cases"""

    TARGETS = DEFAULT_TARGETS
    SYSPAGE = {
        'ia32-generic': [
            'uart16550',
            'pc-ata',
            'psh'
        ]
    }

    def __init__(self, target):
        if target not in TargetBuilder.TARGETS:
            raise ValueError(f"invalid target: {target}")

        self.env = os.environ.copy()
        self.target = target
        self.fs_path = PHRTOS_PROJECT_DIR / f"_fs/{self.target}"

        self.env['TARGET'] = self.target
        self.env['CONSOLE'] = 'serial'
        self.env['SYSPAGE'] = ' '.join(TargetBuilder.SYSPAGE[self.target])

    def __str__(self):
        return self.target

    def fs_mkdir(self, path):
        if path.anchor == '/':
            path = path.relative_to('/')
        abs_path = self.fs_path / path
        abs_path.mkdir(exist_ok=True)

    def fs_install(self, path, file, mode):
        if path.anchor == '/':
            path = path.relative_to('/')
        abs_path = self.fs_path / path
        shutil.copy(file, abs_path)
        abs_path.joinpath(file.name).chmod(mode)

    def run_command(self, args, live_output=True, exit_at_error=True):
        proc = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=self.env,
            cwd=PHRTOS_PROJECT_DIR
        )

        while live_output:
            output = proc.stdout.readline().decode('ascii')
            if proc.poll() is not None and output == '':
                break
            if output:
                logging.info(output)

        out, err = proc.communicate()
        if proc.returncode != 0:
            logging.error(f"Command {' '.join(args)} for {self.target} failed!\n")
        else:
            logging.debug(f"Command {' '.join(args)} for {self.target} success!\n")

        logging.error(err.decode('ascii'))
        if not live_output:
            logging.info(out.decode('ascii'))

        if proc.returncode != 0 and exit_at_error:
            sys.exit(1)

        return proc.returncode, out, err

    def build(self):
        logging.info(f"Building {self.env['TARGET']} with syspage: {self.env['SYSPAGE']}\n")
        self.run_command(['./phoenix-rtos-build/build.sh',
                          'clean',
                          'core',
                          'fs',
                          'test',
                          'image',
                          'project'])
