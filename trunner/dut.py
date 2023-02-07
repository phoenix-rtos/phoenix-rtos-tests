from typing import Any

import pexpect
import pexpect.fdpexpect
import serial


class PortError(Exception):
    pass


class PortNotFound(PortError):
    pass


class Dut:
    """
    This class wraps the pexpect object using the delegator pattern.

    With the delegator pattern, we can keep one object through the testing campaign
    and we are flexible with changing the pexpect object underneath. This is helpful
    when implementing "reboot" logic for devices that are emulated (e.g. using qemu).
    """

    def __init__(self):
        self.pexpect_proc = None

    def __getattr__(self, __name: str) -> Any:
        return getattr(self.pexpect_proc, __name)

    def __setattr__(self, __name: str, __value: Any) -> None:
        try:
            super().__setattr__(__name, __value)
        except AttributeError:
            setattr(self.pexpect_proc, __name, __value)

    def clear_buffer(self):
        """
        Clears the pexpect buffer.

        This function may be useful for clearing the buffer between turning on the device
        and the first reboot to avoid unwanted content. It can only be used if we know that
        device will not write to output.
        """

        if not self.pexpect_proc:
            return

        try:
            while True:
                self.pexpect_proc.expect(r".+", timeout=0)
        except (pexpect.TIMEOUT, pexpect.EOF):
            pass

    def open(self):
        raise NotImplementedError

    def close(self):
        raise NotImplementedError


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
        if self.pexpect_proc:
            self.pexpect_proc.close()

    def open(self):
        self.pexpect_proc = pexpect.spawn(*self.args, **self.kwargs)

    def set_args(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs


class QemuDut(ProcessDut):
    pass


class HostDut(ProcessDut):
    pass
