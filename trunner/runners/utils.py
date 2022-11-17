#
# Phoenix-RTOS test runner
#
# Handlers for utilities used by Test Runner
#
# Copyright 2021, 2022 Phoenix Systems
# Authors: Jakub Sarzyński, Mateusz Niewiadomski, Damian Loewnau
#

import logging
import os
import pexpect
import re
import signal
import time
import threading

from abc import ABC, abstractmethod

from trunner.tools.color import Color
from .common import boot_dir, wait_for_dev, is_github_actions


def append_output(progname, message, output):
    progname = progname.upper()

    msg = message
    msg += Color.colorify(f'\n{progname} OUTPUT:\n', Color.BOLD)
    msg += f'------------------------\n{output}\n------------------------\n'

    return msg


class GdbError(Exception):
    def __init__(self, error, gdb_output='', gdbserv_output=''):

        msg = f'{Color.BOLD}GDB ERROR:{Color.END} {error}\n'
        if gdb_output:
            msg = append_output('gdb', msg, gdb_output)
        if gdbserv_output:
            msg = append_output('JlinkGdbServer', msg, gdbserv_output)

        super().__init__(msg)


class ProcessHandlerError(Exception):
    def __init__(self, progname, message, output=''):

        msg = f'{Color.BOLD}{progname.upper()} ERROR:{Color.END} {message}\n'
        if output:
            msg += f'{Color.BOLD}{progname.upper()} OUTPUT:{Color.END}\n {output}\n'

        super().__init__(msg)


class PsuError(ProcessHandlerError):
    def __init__(self, error):
        super().__init__('psu', error)


class PhoenixdError(ProcessHandlerError):
    def __init__(self, error, output=''):
        super().__init__('phoenixd', error, output)


class OpenocdError(ProcessHandlerError):
    def __init__(self, error, output=''):
        super().__init__('openocd', error, output)


class ProcessHandler(ABC):
    """Handler for the specific process"""

    def __init__(self, target, progname, args=[], cwd=None):
        if cwd is None:
            cwd = boot_dir(target)
        self.progname = progname
        self.args = args
        self.cwd = cwd
        self.proc = None

    def read_output(self, progname):
        """ This method is intended to read the program output"""
        if is_github_actions():
            logging.info(f'::group::Run {progname}\n')

        while True:
            line = self.proc.readline()
            if not line:
                break

            logging.info(line)

        if is_github_actions():
            logging.info('::endgroup::\n')

    @abstractmethod
    def run(self):
        """ This method is intended to run the program"""
        pass


class BackgroundProcessHandler(ABC):
    """ Handler for the specific process intended to run in background"""

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


class Psu(ProcessHandler):
    """Handler for psu process"""

    def __init__(self, target, script, cwd=None):
        super().__init__(target, 'psu', [f'{script}'], cwd)

    def run(self):
        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            self.progname,
            self.args,
            cwd=self.cwd,
            encoding='ascii'
        )

        self.read_output(progname='psu')
        self.proc.wait()
        if self.proc.exitstatus != 0:
            raise PsuError(' Loading plo using psu failed, psu exit status was not equal 0!')


class Openocd(ProcessHandler):
    """Handler for openocd process"""

    def __init__(self, target, file, interface, cwd=None):
        # tested only for stm32l4x6 target and stlink interface
        if 'stm32l4x6' in target:
            openocd_target = 'stm32l4x'
        else:
            raise OpenocdError('Not supported target!')

        if interface != 'stlink':
            raise OpenocdError('Not supported interface!')

        args = [
            '-f', f'interface/{interface}.cfg',
            '-f', f'target/{openocd_target}.cfg',
            '-c', 'reset_config srst_only srst_nogate connect_assert_srst',
            '-c', f'program {file} 0x08000000 verify reset exit'
            ]

        super().__init__(target, 'openocd', args, cwd)

    def run(self):
        self.proc = pexpect.spawn(
            self.progname,
            self.args,
            cwd=self.cwd,
            encoding='ascii'
        )

        self.read_output(progname='openocd')
        self.proc.wait()
        if self.proc.exitstatus != 0:
            raise OpenocdError('Flashing system image using openocd failed, openocd exit status was not equal 0!')


class GdbScript(ProcessHandler):
    """ Handler for gdb-multiarch process that executes a script """

    def __init__(self, target, file, script, cwd=None):
        args = [f'{file}', '-x', f'{script}', '-batch']
        super().__init__(target, 'gdb-multiarch', args, cwd)
        self.jlink_device = ''
        if 'zynq7000' in target:
            self.jlink_device = 'Zynq 7020'
        else:
            raise GdbError('JLink device not set!')

    def read_output(self):
        if is_github_actions():
            logging.info('::group::Run gdb\n')

        self.proc.expect_exact(pexpect.EOF)
        logging.info(f'{self.proc.before}\n')

        if is_github_actions():
            logging.info('::endgroup::\n')

    def run(self):
        with JLinkGdbServer(self.jlink_device) as gdbserv:
            # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
            self.proc = pexpect.spawn(
                self.progname,
                self.args,
                cwd=self.cwd,
                encoding='ascii',
                timeout=8
            )

            exit_status = self.proc.wait()
            self.read_output()

            if exit_status != 0:
                raise GdbError('Gdb process failed!', gdb_output=self.proc.before, gdbserv_output=gdbserv.output())

            self.proc.close()

    def close(self):
        self.proc.close()
        if self.proc.exitstatus != 0:
            logging.error('gdb failed!\n')
            raise GdbError('Loading plo using gdb failed, gdb exit status was not equal 0!')


class GdbInteractive(ProcessHandler):
    """ Handler for gdb-multiarch process in interactive mode """

    def __init__(self, target, port, cwd=None):
        self.port = port
        super().__init__(target, 'gdb-multiarch', cwd=cwd)

    def handle_pexpect_error(self, exc):
        r_searched = r"[\d+]: (?:re.compile\()?b?['\"](.*)['\"]\)?"
        searched_patterns = re.findall(r_searched, exc.value)

        self.exception += Color.colorify('EXPECTED:\n', Color.BOLD)
        for idx, pattern in enumerate(searched_patterns):
            self.exception += f'\t{idx}: {pattern}\n'

    def expect_prompt(self):
        self.proc.expect_exact("(gdb) ")

    def connect(self):
        self.expect_prompt()
        self.proc.sendline(f'target remote localhost: {self.port}')
        self.expect_prompt()

    def load(self, test_path, addr):
        self.proc.sendline(f'restore {test_path} binary {addr}')
        self.proc.expect_exact("Restoring binary file")
        self.expect_prompt()

    def go(self):
        self.proc.sendline("c")
        self.proc.expect_exact("Continuing.")
        # When closing right after continuing plo will get stuck
        time.sleep(0.5)

    def run(self):
        self.proc = pexpect.spawn(
            self.progname,
            encoding='ascii',
            timeout=8
        )
        self.proc.logfile = open("/tmp/phoenix_gdb_log", "a")

    def close(self):
        self.proc.close()
        if self.proc.exitstatus != 0:
            raise GdbError('Gdb exit status was not equal 0 after closing gdb in interactive mode!')

    def __enter__(self):
        self.run()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


class Phoenixd(BackgroundProcessHandler):
    """ Handler for phoenixd process"""

    def __init__(
        self,
        target,
        port,
        dir='.',
        cwd=None,
    ):
        if cwd is None:
            cwd = boot_dir(target)
        self.cwd = cwd
        self.port = port
        self.dir = dir
        self.dispatcher_event = None
        super().__init__()

    def _reader(self):
        """ This method is intended to be run as a separate thread.
        It searches for a line stating that message dispatcher has started
        and then browses phoenixd output that may be needed for proper working """

        msg = f'Starting message dispatcher on [{self.port}]'

        try:
            self.proc.expect_exact(msg, timeout=4)
        # The main thread will catch that dispatcher didn't start
        except (pexpect.EOF, pexpect.TIMEOUT):
            return

        self.dispatcher_event.set()

        self.output_buffer = self.proc.before
        # Phoenixd requires reading its output constantly to work properly, especially during long copy operations
        self.proc.expect(pexpect.EOF, timeout=None)
        # EOF occurs always after killing the phoenixd process
        self.output_buffer += self.proc.before

    def run(self):
        try:
            wait_for_dev(self.port, timeout=10)
        except TimeoutError as exc:
            raise PhoenixdError(f'couldn\'t find {self.port}') from exc

        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            'phoenixd',
            ['-p', self.port,
             '-s', self.dir],
            cwd=self.cwd,
            encoding='ascii'
        )

        self.dispatcher_event = threading.Event()
        self.reader_thread = threading.Thread(target=self._reader)
        self.reader_thread.start()

        # Reader thread will notify us that message dispatcher has just started
        dispatcher_ready = self.dispatcher_event.wait(timeout=5)
        if not dispatcher_ready:
            self.kill()
            raise PhoenixdError('Message dispatcher did not start!', self.output_buffer)

        return self.proc

    def output(self):
        if is_github_actions():
            output = '::group::phoenixd output\n' + self.output_buffer + '\n::endgroup::\n'
        else:
            output = self.output_buffer

        return output

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.kill()


class OpenocdGdbServer(BackgroundProcessHandler):
    """ Handler for OpenocdGdbServer process"""

    def __init__(
        self,
        target,
        interface
    ):
        super().__init__()

        self.listening_event = threading.Event()

        if 'stm32l4x6' in target:
            self.openocd_target = 'stm32l4x'
        else:
            raise OpenocdError('Not supported target!')
        if interface == 'stlink':
            self.interface = interface
        else:
            raise OpenocdError('Not supported interface!')

    def _reader(self):
        """ This method is intended to be run as a separated thread. It reads output of proc
            line by line and saves it in the output_buffer."""

        while True:
            line = self.proc.readline()
            if not line:
                break

            if 'Info : Listening on port 3333 for gdb connections' in line:
                self.listening_event.set()

            self.output_buffer += line

    def run(self):
        # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            'openocd',
            ['-f', f'interface/{self.interface}.cfg',
             '-f', f'target/{self.openocd_target}.cfg',
             '-c', 'reset_config srst_only srst_nogate connect_assert_srst',
             '-c', 'init;reset'],
            encoding='ascii'
        )

        self.reader_thread = threading.Thread(target=self._reader)
        self.reader_thread.start()

        listening_ready = self.listening_event.wait(timeout=5)
        if not listening_ready:
            self.kill()
            raise OpenocdError('Openocd gdb listening did not start!', )

        return self.proc

    def output(self):
        output = self.output_buffer
        if is_github_actions():
            output = '::group::gdbserver output\n' + output + '\n::endgroup::\n'

        return output


class JLinkGdbServer(BackgroundProcessHandler):
    """ Handler for JLinkGdbServer process"""

    def __init__(
        self,
        device
    ):
        self.device = device
        super().__init__()

    def _reader(self):
        """ This method is intended to be run as a separate thread. It reads output of proc
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
