import time

from abc import ABC, abstractmethod
from io import StringIO
from pty import STDOUT_FILENO
from typing import Any, Optional, Tuple

import termios
import pexpect
from pexpect.exceptions import EOF
import pexpect.fdpexpect
import serial


class PortError(Exception):
    pass


class PortNotFound(PortError):
    pass


def clear_pexpect_buffer(pexpect_obj):
    """Clears the pexpect buffer.

    This function should be used when pexpect obj doesn't transmit new bytes."""

    try:
        while True:
            pexpect_obj.expect(r".+", timeout=0)
    except (pexpect.TIMEOUT, pexpect.EOF):
        pass


class Dut(ABC):
    """
    This class wraps the pexpect object using the delegator pattern.

    With the delegator pattern, we can keep one object through the testing campaign
    and we are flexible with changing the pexpect object underneath. This is helpful
    when implementing "reboot" logic for devices that are emulated (e.g. using qemu).
    """

    def __init__(self):
        self.pexpect_proc = None
        self._logfiles: Optional[Tuple[StringIO, StringIO, StringIO]] = None

    def __getattr__(self, __name: str) -> Any:
        return getattr(self.pexpect_proc, __name)

    def __setattr__(self, __name: str, __value: Any) -> None:
        try:
            super().__setattr__(__name, __value)
        except AttributeError:
            setattr(self.pexpect_proc, __name, __value)

    def set_logfiles(self, rd: StringIO, wr: StringIO, all: StringIO):
        self._logfiles = rd, wr, all
        self._set_logfiles()

    def get_logfiles(self) -> Tuple[StringIO, StringIO, StringIO]:
        if not self._logfiles:
            raise ValueError("logs were never configured")

        return self._logfiles

    def _set_logfiles(self):
        if self.pexpect_proc and self._logfiles is not None:
            self.pexpect_proc.logfile_read = self._logfiles[0]
            self.pexpect_proc.logfile_send = self._logfiles[1]
            self.pexpect_proc.logfile = self._logfiles[2]

    def read(self, size: int = 512, timeout: float = 0.1) -> str:
        """read out RAW output from the DUT with configurable timeout"""
        if not self.pexpect_proc:
            return ""

        ret = ""
        abs_timeout = time.time() + timeout
        remaining_size = size
        remaining_time = abs_timeout - time.time()

        while remaining_time > 0 and remaining_size > 0:
            try:
                ret += self.pexpect_proc.read_nonblocking(size=remaining_size, timeout=remaining_time)
                remaining_size = size - len(ret)
            except pexpect.TIMEOUT:
                pass
            except EOF:
                break

            remaining_time = abs_timeout - time.time()

        return ret

    def clear_buffer(self):
        """
        Clears the pexpect buffer.

        This function may be useful for clearing the buffer between turning on the device
        and the first reboot to avoid unwanted content. It can only be used if we know that
        device will not write to output.
        """

        if not self.pexpect_proc:
            return

        clear_pexpect_buffer(self.pexpect_proc)

    @abstractmethod
    def open(self):
        pass

    @abstractmethod
    def close(self):
        pass


class SerialDut(Dut):
    """DUT class for targets that communicates with host using serial port"""

    def __init__(self, port, baudrate, *args, **kwargs):
        super().__init__()
        self.port = port
        self.baudrate = baudrate
        self.args = args
        self.kwargs = kwargs
        self.open()

    def close(self):
        if not self.serial.is_open:
            return

        self.pexpect_proc.flush()
        self.serial.close()

    def open(self):
        try:
            self.serial = serial.Serial(self.port, self.baudrate)
        except serial.SerialException as e:
            raise PortError(e) from e

        self.pexpect_proc = pexpect.fdpexpect.fdspawn(self.serial, *self.args, **self.kwargs)


class ProcessDut(Dut):
    """DUT class for targets that are emulated by spawning process on host, like emulators"""

    def __init__(self, *args, **kwargs):
        super().__init__()
        self.args = args
        self.kwargs = kwargs

    def close(self):
        """Forcefully close the child process"""
        if self.pexpect_proc:
            self.pexpect_proc.close()

    def open(self):
        self.pexpect_proc = pexpect.spawn(*self.args, **self.kwargs)
        self._set_logfiles()

    def set_args(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs

    def wait(self, timeout=5):
        """Wait with timeout for process end. Read out all data in the mean time (push it to logs)"""

        wait_start = time.time()
        while True:
            if wait_start + timeout < time.time():
                raise TimeoutError("Timed out waiting for EOF")
            try:
                # read out all remaining output from the program
                self.pexpect_proc.read_nonblocking(size=512)
            except EOF:
                break

        while self.pexpect_proc.isalive():
            time.sleep(0.1)

            if wait_start + timeout < time.time():
                raise TimeoutError("Timed out waiting for process end")

        return self.pexpect_proc.exitstatus


class QemuDut(ProcessDut):
    @staticmethod
    def _set_termios_raw():
        """set current tty not to process newlines"""
        attr = termios.tcgetattr(STDOUT_FILENO)

        # disable any newline manilpulations in c_oflag
        attr[1] &= ~(termios.ONLCR | termios.OCRNL | termios.ONOCR | termios.ONLRET)

        termios.tcsetattr(STDOUT_FILENO, termios.TCSANOW, attr)

    def open(self):
        # qemu when using stdio serial enforces ONLCR termios flag (converting `\n` to `\r\n`)
        # on proper guest OSes this results in `\r\r\n` which might be incorrectly interpreted by CI log viewers
        # use custom preexec_fn to setup termios of child PTY
        self.pexpect_proc = pexpect.spawn(*self.args, preexec_fn=self._set_termios_raw, **self.kwargs)
        self._set_logfiles()


class HostDut(ProcessDut):
    pass
