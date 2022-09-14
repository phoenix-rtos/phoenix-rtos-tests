#
# Phoenix-RTOS test runner
#
# STM32l4 runner
#
# Copyright 2021 Phoenix Systems
# Authors: Mateusz Niewiadomski
#

import os
import re
import select
import subprocess
import sys
import time

import pexpect.fdpexpect
import serial
from pexpect.exceptions import TIMEOUT, EOF

from .common import DeviceRunner, Color, RebootError, GPIO
from .common import boot_dir
from .common import LOG_PATH, Psu, Phoenixd, PhoenixdError, PloError, PloTalker, DeviceRunner, Runner, RebootError
from .common import GPIO, phd_error_msg, rootfs, is_github_actions


import threading
from abc import ABC, abstractmethod
import signal
import logging


def append_output(progname, message, output):
    progname = progname.upper()

    msg = message
    msg += Color.colorify(f'\n{progname} OUTPUT:\n', Color.BOLD)
    msg += f'------------------------\n{output}\n------------------------\n'

    return msg


def count_psize(binary_path: str, page_size, page_mask):
    wc = subprocess.run(['wc', '-c', binary_path], capture_output=True)
    out = wc.stdout.decode()
    print(out)
    m = re.search(r'(\d+?)\s', out)
    bytes = m.group(0)
    prog_size = (int(bytes) + page_size - 1) & page_mask

    print(hex(prog_size))
    return prog_size
    # print('----')
    # print(m.group(0))
    # print('----------')
    # print(wc.stdout)
    # print('---------')
    # re.match




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


class OpenocdGdbServer(BackgroundProcessHandler):
    """ Handler for OpenocdGdbServer process"""

    def __init__(
        self,
    ):
        super().__init__()
        self.listening_event = threading.Event()

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
            ['-f', 'interface/stlink.cfg',
             '-f', 'target/stm32l4x.cfg',
             '-c', 'reset_config srst_only srst_nogate connect_assert_srst',
             '-c', 'init;reset'],
            encoding='ascii'
        )
        print('after running openocd!!!')

        self.reader_thread = threading.Thread(target=self._reader)
        self.reader_thread.start()

        listening_ready = self.listening_event.wait(timeout=5)
        if not listening_ready:
            self.kill()
            print('openocd gdb listening did not start')

        return self.proc

    def output(self):
        output = self.output_buffer
        if is_github_actions():
            output = '::group::gdbserver output\n' + output + '\n::endgroup::\n'

        return output



class ProcessHandler(ABC):
    """Handler for the specific process"""

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


class GdbError(Exception):
    def __init__(self, error, gdb_output='', gdbserv_output=''):

        msg = f'{Color.BOLD}GDB ERROR:{Color.END} {error}\n'
        if gdb_output:
            msg = append_output('gdb', msg, gdb_output)
        if gdbserv_output:
            msg = append_output('JlinkGdbServer', msg, gdbserv_output)

        super().__init__(msg)


class Gdb(ProcessHandler):
    """Handler for gdb-multiarch process"""

    def __init__(self, target, file=None, script=None, cwd=None):
        args = [] #args = [f'{file}', '-x', f'{script}']
        super().__init__(target, 'gdb-multiarch', args, cwd)
        self.jlink_device = ''
        if 'zynq7000' in target:
            self.jlink_device = 'Zynq 7020'

    def read_output(self):
        if is_github_actions():
            logging.info('::group::Run gdb\n')

        # When using docker, the additional newline may be needed
        idx = self.proc.expect_exact(["(gdb) ", "Type <RET> for more"])

        if idx == 1:
            logging.info(self.proc.before)
            self.proc.send('\r\n')
            self.proc.expect_exact("(gdb) ")

        self.proc.send('c\r\n')

        logging.info(f'{self.proc.before}\n')
        self.proc.close()

        if is_github_actions():
            logging.info('::endgroup::\n')

    def expect_prompt(self):
        self.proc.expect_exact("(gdb) ")

    def connect(self, port):
        self.proc.sendline(f'target remote localhost: {port}')
        self.expect_prompt()

    def load(self, test_path, addr):
        self.proc.sendline(f'restore {test_path} binary {addr}')
        self.proc.expect_exact("Restoring binary file")
        self.expect_prompt()

    def go(self):
        self.proc.sendline("c")
        self.proc.expect_exact("Continuing.")

    def run(self, test_path):
        # with OpenocdGdbServer() as gdbserv:
            # Use pexpect.spawn to run a process as PTY, so it will flush on a new line
        self.proc = pexpect.spawn(
            self.progname,
            encoding='ascii',
            timeout=8
        )
        self.proc.logfile = open("/tmp/gdb_log", "a")


        try:
            self.expect_prompt()
            self.connect(port=3333)
            self.load(test_path, addr=0x20030000)
            self.go()
        except pexpect.TIMEOUT:
            print('gdb failed')
            print(self.proc.before)
        # When closing right after continuing plo will get stuck
        time.sleep(0.5)
        self.close()

    def close(self):
        self.proc.close()
        if self.proc.exitstatus != 0:
            logging.error('gdb failed!\n')
            raise GdbError('Loading plo using gdb failed, gdb exit status was not equal 0!')


    def kill(self):
        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)
        # self.reader_thread.join(timeout=10)
        if self.proc.isalive():
            os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)

def hostpc_reboot():
    """Reboots a tested device by a human."""

    print("Please reset the board by pressing `RESET B2` button and press enter")
    print("\tNote that You should press the enter key just after reset button")

    if sys.stdin in select.select([sys.stdin], [], [], 30)[0]:
        sys.stdin.readline()
    else:
        raise RebootError('It took too long to wait for a key pressing')


class STM32L4Runner(DeviceRunner):
    """ Interface to run test cases on STM32L4 target using RaspberryPi.

    The ST-Link programming board should be connected to USB port
    and a target should be connected to ST-Link and powered
    """

    SIZE_PAGE=0x200
    PAGE_MASK=0xfffffe00

    class oneByOne_fdspawn(pexpect.fdpexpect.fdspawn):
        """ Workaround for Phoenix-RTOS on stm32l4 targets not processing characters fast enough

        Inherits from and is passed to harness instead of pexpect.fdpexpect.fdspawn
        It redefines send() with addition of delay after each byte sent. Delay set to 30ms/byte

        Issue regarding cause of this class existence: github.com/phoenix-rtos/phoenix-rtos-project/issues/235
        """

        def send(self, string):
            ret = 0
            for char in string:
                ret += super().send(char)
                time.sleep(0.03)
            return ret

    def __init__(self, target, serial, log=False):
        self.is_rpi_host = True
        self.is_cut_power_used = False # True #False
        if self.is_rpi_host:
            self.reset_gpio = GPIO(17)
            self.reset_gpio.high()
            self.power_gpio = GPIO(2)
            self.power_gpio.high()
        super().__init__(target, serial, log)

    def _restart_by_jtag(self):
        self.reset_gpio.low()
        time.sleep(0.050)
        self.reset_gpio.high()

    def _restart_by_poweroff(self):
        self.power_gpio.low()
        time.sleep(0.050)
        self.power_gpio.high()
        time.sleep(0.050)

    def rpi_reboot(self, cut_power=False):
        """Reboots a tested device using Raspberry Pi as host-generic-pc."""
        if cut_power:
            # pass
            self._restart_by_poweroff()
        else:
            self._restart_by_jtag()

    def reboot(self, cut_power=False):
        """Reboots a tested device."""
        if self.is_rpi_host:
            self.rpi_reboot(cut_power)
        else:
            hostpc_reboot()
    
    def flash(self):
        """ Flashing with openocd as a separate process """
        binary_path = os.path.join(boot_dir(self.target), 'phoenix.disk')
        openocd_cmd = [
            'openocd',
            '-f', 'interface/stlink.cfg',
            '-f', 'target/stm32l4x.cfg',
            '-c', "reset_config srst_only srst_nogate connect_assert_srst",
            '-c',  "program {progname} 0x08000000 verify reset exit".format(progname=binary_path)
            ]

        # openocd prints important information (counterintuitively) on stderr, not stdout. However pipe both for user
        openocd_process = subprocess.Popen(
            openocd_cmd,
            stdout=sys.stdout.fileno(),
            stderr=sys.stdout.fileno()
            )

        if openocd_process.wait() != 0:
            print(Color.colorify("OpenOCD error: cannot flash target", Color.BOLD))
            sys.exit(1)

    def load(self, test):
        """Loads test ELF into syspage using plo"""
        load_dir = str(rootfs(test.target) / 'bin')
        try:
            self.reboot(cut_power=self.is_cut_power_used)
            with OpenocdGdbServer() as openocd:
                with PloTalker(self.serial_port, replaced_fdspawn=self.oneByOne_fdspawn) as plo:
                    if self.logpath:
                        plo.plo.logfile = open(self.logpath, "a")
                    plo.interrupt_counting()
                    plo.wait_prompt()
                    if not test.exec_cmd and not test.syspage_prog:
                        return True
                    # plo.close()
                    prog = test.exec_cmd[0] if test.exec_cmd else test.syspage_prog
                    test_path = f'{load_dir}/{prog}'
                    Gdb(self.target).run(test_path)

                    # print('run gdb and connect to gdb server')
                    # time.sleep(10)
                    # print('ok it should be ready')


                    # gdb.close()
                    # plo.close()
                    # plo2 = plo.open()
                    # time.sleep(5)
                    print('before sending help')
                    plo.plo.send('help\r\n')
                    plo.plo.send('\r\n')
                    # plo.plo.send('\r\n')
                    plo.plo.expect_exact('plo')
                    print('it has plo prompt!!')
                    # plo.cmd('help')
                    # print(f'test.exec_cmd[0] = {test.exec_cmd[0]}')

                    psize = count_psize(f'{test_path}', self.SIZE_PAGE, self.PAGE_MASK)
                    print(f'alias {prog} 0x30000 {psize}')
                    plo.cmd(f'alias {prog} 0x30000 {psize}')
                    # plo.cmd('app ram test-setjmp hello ram ram')

                    plo.app('ram', prog, 'ram', 'ram')
        except (TIMEOUT, EOF, PloError, RebootError) as exc:
            if isinstance(exc, TIMEOUT) or isinstance(exc, EOF):
                test.exception = Color.colorify('EXCEPTION PLO\n', Color.BOLD)
                test.handle_pyexpect_error(plo.plo, exc)
            else:  # RebootError, PloError or PhoenixdError
                test.exception = str(exc)
                test.exception += 'opencod output------------' + openocd.output() + '----------------'
                test.fail()
            return False

        return True

    def run(self, test):
        if test.skipped():
            return

        if not self.load(test):
            return
        super().run(test, replaced_fdspawn=self.oneByOne_fdspawn)








        # if test.skipped():
        #     return

        # if not self.load(test):
        #     return
            
        # try:
        #     self.serial = serial.Serial(self.serial_port, baudrate=self.serial_baudrate)
        # except serial.SerialException:
        #     test.handle_exception()
        #     return

        # # Create pexpect.fdpexpect.fdspawn with modified send() by using oneByOne_fdspawn class
        # # codec_errors='ignore' - random light may cause out-of-ascii characters to appear when using optical port
        # proc = self.oneByOne_fdspawn(
        #     self.serial,
        #     encoding='ascii',
        #     codec_errors='ignore',
        #     timeout=test.timeout
        #     )

        # # print('restart board')
        # # if sys.stdin in select.select([sys.stdin], [], [], 30)[0]:
        # #     sys.stdin.readline()
        # # else:
        # #     print('It took too long to wait for a key pressing')
        # # print('after')
        # # proc = pexpect.fdpexpect.fdspawn(self.serial, encoding='utf8', timeout=test.timeout)

        # # time.sleep(3)

        # # print('1')
        # # proc.expect('p')
        # # print('2')
        # # proc.expect_exact('(psh)% ')
        # # print('3')
        # if self.logpath:
        #     proc.logfile = open(self.logpath, "a")

        # # FIXME - race on start of Phoenix-RTOS between dummyfs and psh
        # # flushing the buffer and sending newline
        # # ensures that carret is in newline just after (psh)% prompt
        # # flushed = ""
        # # while not proc.expect([r'.+', TIMEOUT], timeout=0.1):
        # #     flushed += proc.match.group(0)
        # # flushed = None
        # # proc.send("\n")

        # try:
        #     test.handle(proc)
        # finally:
        #     self.serial.close()
