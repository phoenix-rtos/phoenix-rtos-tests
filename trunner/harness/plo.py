from dataclasses import dataclass
from typing import Callable, Optional, Sequence

import pexpect

from trunner.dut import Dut
from trunner.text import bold
from trunner.tools import Phoenixd
from trunner.types import AppOptions, TestResult
from .base import HarnessBase, HarnessError, Rebooter


class PloError(HarnessError):
    """Exception thrown by PloInterface class."""

    def __init__(
        self,
        msg: Optional[str] = None,
        output: Optional[str] = None,
        expected: Optional[str] = None,
        cmd: Optional[str] = None,
    ):
        self.msg = msg
        self.output = output
        self.cmd = cmd
        self.expected = expected
        super().__init__(self)

    def __str__(self):
        err = bold("PLO ERROR: ") + (self.msg if self.msg else "") + "\n"
        if self.cmd is not None:
            err += bold("CMD PASSED: ") + self.cmd + "\n"

        if self.expected is not None:
            err += bold("EXPECTED: ") + self.expected + "\n"

        if self.output is not None:
            err += bold("OUTPUT:") + "\n" + self.output + "\n"

        return err


class PloInterface:
    """
    Interface to communicate with plo.

    It implements useful methods to communicate with the plo bootloader and standardize the way
    to handle the errors.
    """

    def __init__(self, dut: Dut):
        self.dut = dut

    def enter_bootloader(self):
        """Interrupts timer counting to enter plo."""

        try:
            self.dut.expect_exact("Waiting for input", timeout=3)
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise PloError("Failed to go into bootloader mode", output=self.dut.before) from e

        self.dut.send("\n")

    def _assert_prompt(self, timeout: Optional[int] = None, error_check=True):
        """Asserts that plo prompt was outputted by dut.

        This method can be combined with sending command to wait for the command to end
        and to check if error occurred.

        Args:
            timeout: Timeout to wait for a prompt, same as in pexpect module.
            error_check: Check if there is a red color (\x1b[31m code) in pexpect buffer before the prompt.
        """

        try:
            self.dut.expect_exact("(plo)%", timeout=timeout)
        except pexpect.TIMEOUT as e:
            raise PloError(output=self.dut.before, expected="(plo)% ") from e

        if error_check:
            # red color means that, there was some error in plo
            if "\x1b[31m" in self.dut.before:
                raise PloError(output=self.dut.before, expected="(plo)% ")

    def wait_prompt(self, timeout: Optional[int] = 8):
        """Asserts that plo prompt was outputted by dut."""
        self._assert_prompt(timeout=timeout, error_check=False)

    def send_cmd(self, cmd: str):
        """Sends command and read echo of it."""

        try:
            # When sending the plo command, it only requires the CR character
            self.dut.send(cmd + "\r")
            self.dut.expect_exact(cmd)
        except pexpect.TIMEOUT as e:
            raise PloError("failed to read echoed command", cmd=cmd, output=self.dut.before) from e

    def cmd(self, cmd: str, timeout: Optional[int] = None):
        """Sends command and waits for it to finish.

        In case of error throws PloError exception.
        """

        self.send_cmd(cmd)
        try:
            self._assert_prompt(timeout=timeout)
        except PloError as e:
            e.cmd = cmd
            raise e

    def erase(self, device: str, offset: int, size: int, timeout: Optional[int] = None):
        """Performs erase command."""

        cmd = f"erase {device} {offset} {size}"
        self.send_cmd(cmd)

        try:
            self.dut.expect_exact("Serious risk of data loss, type YES! to proceed.")
            self.send_cmd("YES!")
            self.dut.expect_exact("Erased", timeout=timeout)
        except pexpect.TIMEOUT as e:
            raise PloError("Wrong erase command output!", cmd=cmd, output=self.dut.before) from e

        try:
            self._assert_prompt()
        except PloError as e:
            e.cmd = cmd
            raise e

    def app(
        self, device: str, file: str, imap: str, dmap: str, exec: bool = False
    ):  # pylint: disable=redefined-builtin
        """Performs app command."""
        x = "-x" if exec else ""
        self.cmd(f"app {device} {x} {file} {imap} {dmap}", timeout=30)

    def copy(
        self,
        src: str,
        src_obj: str,
        dst: str,
        dst_obj: str,
        src_size: Optional[int] = None,
        dst_size: Optional[int] = None,
        timeout: Optional[int] = -1,
    ):
        """Performs copy command."""
        src_sz = "" if src_size is None else str(src_size)
        dst_sz = "" if dst_size is None else str(dst_size)
        self.cmd(f"copy {src} {src_obj} {src_sz} {dst} {dst_obj} {dst_sz}", timeout=timeout)

    def copy_file2mem(
        self,
        src: str,
        file: str,
        dst: str = "flash1",
        off: int = 0,
        size: int = 0,
        timeout: Optional[int] = -1,
    ):
        """Copies file to memory."""
        self.copy(
            src=src,
            src_obj=file,
            dst=dst,
            dst_obj=str(off),
            dst_size=size,
            timeout=timeout,
        )

    def alias(self, name: str, offset: int, size: int):
        """Sets alias for the memory region."""
        self.cmd(f"alias {name} {offset:#x} {size:#x}", timeout=4)

    def go(self):
        """Sends go command to jump out from plo."""
        self.dut.send("go!\r")


@dataclass(frozen=True)
class PloImageProperty:
    """Class that represents the OS image loaded by plo."""

    file: str
    source: str
    memory_bank: str


class PloImageLoader(HarnessBase, PloInterface):
    """Harness to load the image to the memory using plo bootloader and phoenixd program.

    It loads plo bootloader using plo_loader callback and then, with the help of a phoenixd program,
    loads the main OS image described by the PloImageProperty class.

    Attributes:
        dut: Device on which harness will be run.
        rebooter: Rebooter object needed to do reboot of the dut.
        plo_loader: A callback that performs plo loading to the RAM memory.
        phoenixd: Phoenixd object that wraps phoenixd program.

    """

    def __init__(
        self,
        dut: Dut,
        rebooter: Rebooter,
        image: PloImageProperty,
        plo_loader: Callable[[], None],
        phoenixd: Phoenixd,
    ):
        self.dut = dut
        self.rebooter = rebooter
        self.image = image
        self.plo_loader = plo_loader
        self.phoenixd = phoenixd
        super().__init__()

    def __call__(self):
        self.rebooter(flash=True, hard=True)
        self.plo_loader()

        with self.phoenixd.run():
            self.copy_file2mem(
                src=self.image.source,
                file=self.image.file,
                dst=self.image.memory_bank,
                timeout=120,
            )

        self.rebooter(flash=False, hard=True)


class PloPhoenixdAppLoader(HarnessBase, PloInterface):
    """Harness to load the binaries to syspage using plo bootloader.

    It loads the binariers of applications using plo bootloader with the help of a phoenixd program.

    Attributes:
        dut: Device on which harness will be run.
        apps: Sequence of applications that will be loaded.
        phoenixd: Phoenixd object that wraps phoenixd program.

    """

    def __init__(self, dut: Dut, apps: Sequence[AppOptions], phoenixd: Phoenixd):
        self.dut = dut
        self.apps = apps if apps else []
        self.phoenixd = phoenixd
        super().__init__()

    def __call__(self):
        with self.phoenixd.run():
            for app in self.apps:
                self.app(
                    device=app.source,
                    file=app.file,
                    imap=app.imap,
                    dmap=app.dmap,
                    exec=app.exec,
                )


class PloHarness(HarnessBase, PloInterface):
    """Basic harness for the plo bootloader that initialize the device.

    This harness enters the plo bootloader and loads binaries to syspage if it's necessary.

    Attributes:
        dut: Device on which harness will be run.
        app_loader: Optional app loader harness to copy the binaries to the syspage.

    """

    def __init__(self, dut: Dut, app_loader: Optional[Callable[[], None]] = None):
        self.dut = dut
        self.app_loader = app_loader
        super().__init__()

    def __call__(self) -> Optional[TestResult]:
        self.enter_bootloader()
        self.wait_prompt()

        if self.app_loader:
            self.app_loader()

        self.go()
        return self.harness()
