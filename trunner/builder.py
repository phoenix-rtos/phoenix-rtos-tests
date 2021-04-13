import logging
import os
import pathlib
import shutil
import subprocess
import sys

from .config import PHRTOS_PROJECT_DIR


class TargetBuilder:
    """A base class that builds image needed to run all test cases"""

    TARGETS = ('ia32-generic',)
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

    def prebuild_action(self, user_syspage=None, **kwargs):
        pass

    def postbuild_action(self, user_syspage=None, **kwargs):
        pass

    def run_command(self, args, live_output=True, exit_at_error=True):
        proc = subprocess.Popen(
            args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=self.env,
            cwd=PHRTOS_PROJECT_DIR
        )

        while live_output:
            output = proc.stdout.readline().decode('utf-8')
            if proc.poll() is not None and output == '':
                break
            if output:
                logging.info(output)

        out, err = proc.communicate()
        if proc.returncode != 0:
            logging.error(f"Command {' '.join(args)} for {self.target} failed!\n")
        else:
            logging.debug(f"Command {' '.join(args)} for {self.target} success!\n")

        logging.error(err.decode('utf-8'))
        if not live_output:
            logging.info(out.decode('utf-8'))

        if proc.returncode != 0 and exit_at_error:
            sys.exit(1)

        return proc.returncode, out, err

    def build(self, user_syspage=None):
        syspage = user_syspage if user_syspage else []
        syspage.extend(TargetBuilder.SYSPAGE[self.target])
        syspage = list(set(syspage))
        self.env['SYSPAGE'] = " ".join(syspage)

        self.prebuild_action(user_syspage)

        logging.info(f"Building {self.env['TARGET']} with syspage: {self.env['SYSPAGE']}\n")
        self.run_command(['./phoenix-rtos-build/build.sh',
                          'clean',
                          'core',
                          'fs',
                          'test',
                          'image',
                          'project'])

        self.postbuild_action(user_syspage)


class IA32Builder(TargetBuilder):
    """This class builds image for the IA32 architecture"""

    def __init__(self):
        super().__init__('ia32-generic')

    def prebuild_action(self, user_syspage=None, **kwargs):
        # We run test binaries from filesystem, set only default syspage
        self.env['SYSPAGE'] = " ".join(self.SYSPAGE[self.target])


class TargetBuilderFactory:
    """This class creates TargetBuilder based on a target name"""

    @staticmethod
    def create(target):
        if target == 'ia32-generic':
            return IA32Builder()

        return TargetBuilder(target)
