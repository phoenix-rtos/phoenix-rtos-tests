#
# Phoenix-RTOS test runner
#
# Wrappers for programs used by Test Runner
#
# Copyright 2021, 2022 Phoenix Systems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import logging
import os
import pexpect
import signal
import threading

from abc import ABC, abstractmethod

from trunner.tools.color import Color
from .common import boot_dir, wait_for_dev


def is_github_actions():
    return os.getenv('GITHUB_ACTIONS', False)


class StandardWrapper(ABC):
    """Wrapper for the specific program"""

    def __init__(self, target, progname, args, cwd=None):
        if cwd is None:
            cwd = boot_dir(target)
        self.progname = progname
        self.args = args
        self.cwd = cwd
        self.proc = None

    @abstractmethod
    def read_output(self):
        """ This method is intended to read the program output"""
        pass

    @abstractmethod
    def run(self):
        """ This method is intended to run the program"""
        pass


class BackgroundWrapper(ABC):
    """ Wrapper for the specific program intended to run in background"""

    def __init__(
        self
    ):
        self.proc = None
        self.output_buffer = ''
        self.reader_thread = None

    @abstractmethod
    def _reader(self):
        """ This method is intended to read the program output in separate thread"""
        pass

    @abstractmethod
    def run(self):
        """ This method is intended to run the program"""
        pass

    @abstractmethod
    def output(self):
        """ This method is intended to store the program output"""
        pass

    def kill(self):
        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)
        self.reader_thread.join(timeout=10)
        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)

    def __enter__(self):
        self.run()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.kill()


class Psu(StandardWrapper):
    """Wrapper for psu program"""

    def __init__(self, target, script, cwd=None):
        super().__init__(target, 'psu', [f'{script}'], cwd)

    def read_output(self):
        if is_github_actions():
            logging.info('::group::Run psu\n')

        while True:
            line = self.proc.readline()
            if not line:
                break

            logging.info(line)

        if is_github_actions():
            logging.info('::endgroup::\n')

    def run(self):
        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            self.progname,
            self.args,
            cwd=self.cwd,
            encoding='ascii'
        )

        self.read_output()
        self.proc.wait()
        if self.proc.exitstatus != 0:
            logging.error('psu failed!\n')
            raise Exception(' Loading plo using psu failed!')


class Gdb(StandardWrapper):
    """Wrapper for gdb-multiarch program"""

    def __init__(self, target, file, script, cwd=None):
        args = [f'{file}', '-x', f'{script}']
        super().__init__(target, 'gdb-multiarch', args, cwd)

    def read_output(self):
        if is_github_actions():
            logging.info('::group::Run gdb\n')

        self.proc.expect_exact("(gdb) ")

        # TODO: investigate why send('c\r\n') does not work
        self.proc.send('c')
        self.proc.send('\r\n')

        logging.info(f'{self.proc.before}\n')
        self.proc.close()

        if is_github_actions():
            logging.info('::endgroup::\n')

    def run(self):
        with GdbServer():
            # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
            self.proc = pexpect.spawn(
                self.progname,
                self.args,
                cwd=self.cwd,
                encoding='ascii'
            )

            self.read_output()
            self.proc.close()

    def close(self):
        self.proc.close()
        if self.proc.exitstatus != 0:
            logging.error('gdb failed!\n')
            raise Exception('Loading plo using gdb failed!')


def phd_error_msg(message, output):
    msg = message
    msg += Color.colorify('\nPHOENIXD OUTPUT:\n', Color.BOLD)
    msg += output

    return msg


def phd_warning_msg(message, warning):
    msg = message
    msg += Color.colorify('\nPHOENIXD WARNING:\n', Color.BOLD)
    msg += warning

    return msg


class PhoenixdError(Exception):
    pass


class Phoenixd(BackgroundWrapper):
    """ Wrapper for phoenixd program"""

    def __init__(
        self,
        target,
        port,
        baudrate=460800,
        dir='.',
        cwd=None,
        wait_dispatcher=True,
        long_flashing=False
    ):
        if cwd is None:
            cwd = boot_dir(target)
        self.cwd = cwd
        self.port = port
        self.baudrate = baudrate
        self.dir = dir
        self.wait_dispatcher = wait_dispatcher
        self.dispatcher_event = None
        self.long_flashing = long_flashing
        super().__init__()

    def _reader(self):
        """ This method is intended to be run as a separated thread. It reads output of proc
            line by line and saves it in the output_buffer. Additionally, if wait_dispatcher is true,
            it searches for a line stating that message dispatcher has started """

        while True:
            line = self.proc.readline()

            if not line:
                break
            self.output_buffer += line

            if self.wait_dispatcher and not self.dispatcher_event.is_set():
                msg = f'Starting message dispatcher on [{self.port}]'
                if msg in line:
                    self.dispatcher_event.set()
                    if self.long_flashing:
                        break

        # faster alternative for catching the whole output
        if self.long_flashing:
            last_msg = "ret=0"
            try:
                self.proc.expect_exact(last_msg, timeout=140)
            # The exception will be caught in plo, we only have to provide phoenixd output
            except (pexpect.EOF, pexpect.TIMEOUT):
                warning = f"The last message ({last_msg}) hasn't been seen.' \
                    'Probably the phoenixd program has been interrupted!\n"
                self.output_buffer += self.proc.before
                self.output_buffer = phd_warning_msg(self.output_buffer, warning)
                return self

            self.output_buffer += self.proc.before
            self.output_buffer += last_msg

    def run(self):
        try:
            wait_for_dev(self.port, timeout=10)
        except TimeoutError as exc:
            raise PhoenixdError(f'couldn\'t find {self.port}') from exc

        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            'phoenixd',
            ['-p', self.port,
             '-b', str(self.baudrate),
             '-s', self.dir],
            cwd=self.cwd,
            encoding='ascii',
        )

        self.dispatcher_event = threading.Event()
        self.reader_thread = threading.Thread(target=self._reader)
        self.reader_thread.start()

        if self.wait_dispatcher:
            # Reader thread will notify us that message dispatcher has just started
            dispatcher_ready = self.dispatcher_event.wait(timeout=5)
            if not dispatcher_ready:
                self.kill()
                msg = 'message dispatcher did not start!'
                raise PhoenixdError(msg)

        return self.proc

    def output(self):
        output = self.output_buffer
        if is_github_actions():
            output = '::group::phoenixd output\n' + output + '\n::endgroup::\n'

        return output


class GdbServer(BackgroundWrapper):
    """ Wrapper for JlinkGdbServer program"""

    def __init__(
        self,
        device='Zynq 7020'
    ):
        self.device = device
        super().__init__()

    def _reader(self):
        """ This method is intended to be run as a separated thread. It reads output of proc
            line by line and saves it in the output_buffer."""

        while True:
            line = self.proc.readline()
            if not line:
                break

            self.output_buffer += line

    def run(self):
        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            'JLinkGDBServer',
            ['-device', self.device],
            encoding='ascii'
        )

        self.reader_thread = threading.Thread(target=self._reader)
        self.reader_thread.start()

        return self.proc

    def output(self):
        output = self.output_buffer
        if is_github_actions():
            output = '::group::gdbserver output\n' + output + '\n::endgroup::\n'

        return output
