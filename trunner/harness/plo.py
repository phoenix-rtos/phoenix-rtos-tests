from abc import abstractmethod
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Optional, Sequence
import time

import pexpect

from trunner.dut import Dut
from trunner.text import bold
from trunner.tools import Phoenixd
from trunner.types import AppOptions, FileOptions, TestResult, TestStage
from .base import HarnessError, IntermediateHarness, Rebooter, TerminalHarness


@dataclass(frozen=True)
class PloJffs2CleanmarkerSpec:
    """Represents the JFFS2 cleanmarkers image specs to be used for formatting"""

    start_block: int
    number_of_blocks: int
    block_size: int
    cleanmarker_size: int

    def __str__(self):
        """Generate spec as plo jffs2 command argument"""
        return f"0x{self.start_block:x}:0x{self.number_of_blocks:x}:0x{self.block_size:x}:0x{self.cleanmarker_size:x}"


class PloError(HarnessError):
    """Exception thrown by PloInterface class."""

    def __init__(
        self,
        msg: str = "",
        output: Optional[str] = None,
        expected: Optional[str] = None,
        cmd: Optional[str] = None,
    ):
        super().__init__()
        self.msg = msg
        self.output = output
        self.cmd = cmd
        self.expected = expected

    def __str__(self):
        err = [bold("PLO ERROR: ") + self.msg]

        if self.cmd is not None:
            err.extend([bold("CMD PASSED:"), self.cmd])

        if self.expected is not None:
            err.extend([bold("EXPECTED:"), self.expected])

        if self.output is not None:
            err.extend([bold("OUTPUT:"), self.output])

        err.extend(self._format_additional_info())

        err.append("")
        return "\n".join(err)


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

        # Temporary fix for [https://github.com/phoenix-rtos/phoenix-rtos-project/issues/1040]
        time.sleep(0.1)

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

        cmd = f"erase {device} 0x{offset:x} 0x{size:x}"
        self.send_cmd(cmd)

        try:
            if self.dut.expect_exact(["Serious risk of data loss, type YES! to proceed.", "Erased"]) == 0:
                self.send_cmd("YES!")
                self.dut.expect_exact("Erased", timeout=timeout)
        except pexpect.TIMEOUT as e:
            raise PloError("Wrong erase command output!", cmd=cmd, output=self.dut.before) from e

        try:
            self._assert_prompt()
        except PloError as e:
            e.cmd = cmd
            raise e

    def jffs2(self, device: str, erase: bool, cleanmarkers: PloJffs2CleanmarkerSpec, block_timeout: int):
        """Performs jffs2 command."""

        block_count = cleanmarkers.number_of_blocks

        cmd = f"jffs2 -d {device} -c {cleanmarkers}"
        if erase:
            cmd += " -e"

        self.send_cmd(cmd)

        for i in range(0, block_count):
            try:
                self.dut.expect_exact(f"jffs2: block {i}/{block_count}", timeout=block_timeout)
            except pexpect.TIMEOUT as e:
                raise PloError("Wrong jffs2 command output!", cmd=cmd, output=self.dut.before) from e

        self._assert_prompt()

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

    def blob(self, device: str, file: str, map: str):
        """Loads a raw binary blob from device into the syspage."""
        self.cmd(f"blob {device} {file} {map}", timeout=30)

    def go(self):
        """Sends go command to jump out from plo."""
        self.dut.send("go!\r")


@dataclass(frozen=True)
class PloImageProperty:
    """Class that represents the OS image loaded by plo."""

    file: str
    source: str
    memory_bank: str


@dataclass(frozen=True)
class PloJffsImageProperty(PloImageProperty):
    """Class that represents the OS image for jffs2 file system, which needs erase before flashing."""

    flash_device_id: str
    cleanmarkers_args: PloJffs2CleanmarkerSpec
    # For new flash memories, block_timeout should be set with documentation.
    block_timeout: int


class PloImageLoader(TerminalHarness, PloInterface):
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
        plo_loader: TerminalHarness,
        phoenixd: Phoenixd,
    ):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.dut = dut
        self.rebooter = rebooter
        self.image = image
        self.plo_loader = plo_loader
        self.phoenixd = phoenixd

    def __call__(self):
        self.rebooter(flash=True, hard=True)
        self.plo_loader()

        if isinstance(self.image, PloJffsImageProperty):
            self.jffs2(self.image.flash_device_id, True, self.image.cleanmarkers_args, self.image.block_timeout)

        with self.phoenixd.run():
            self.copy_file2mem(
                src=self.image.source,
                file=self.image.file,
                dst=self.image.memory_bank,
                timeout=120,
            )

        self.rebooter(flash=False, hard=True)


class PloPhoenixdSyspageLoader(TerminalHarness, PloInterface):
    """Harness to load app binaries and file blobs to syspage using plo bootloader and phoenixd.

    Apps are loaded from the directory configured in the phoenixd instance.
    File blobs are resolved against root_dir and loaded by restarting phoenixd
    once per unique parent directory, so that each blob is served from the
    correct host path.

    Attributes:
        dut: Device on which harness will be run.
        apps: Sequence of applications that will be loaded.
        files: Sequence of file blobs that will be loaded.
        phoenixd: Phoenixd object that wraps phoenixd program.
        root_dir: Host path to root directory for the target. Required when files is non-empty.

    """

    def __init__(
        self,
        dut: Dut,
        apps: Sequence[AppOptions],
        phoenixd: Phoenixd,
        files: Sequence[FileOptions] = (),
        root_dir: Optional[Path] = None,
    ):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.dut = dut
        self.apps = apps
        self.files = files
        self.phoenixd = phoenixd
        if files and root_dir is None:
            raise ValueError(f"{self.__class__.__name__}: root_dir is required when files is non-empty")
        self._root_dir = root_dir

    def __call__(self):
        if self.apps:
            with self.phoenixd.run():
                for app in self.apps:
                    self.app(
                        device=app.source,
                        file=app.file,
                        imap=app.imap,
                        dmap=app.dmap,
                        exec=app.exec,
                    )

        files_by_dir = {}
        for f in self.files:
            parent = self._root_dir / Path(f.file).relative_to("/").parent
            files_by_dir.setdefault(parent, []).append(f)

        for dir_path, dir_files in files_by_dir.items():
            with self.phoenixd.run(dir_path=dir_path):
                for f in dir_files:
                    self.blob(
                        device=f.source,
                        file=Path(f.file).name,
                        map=f.map,
                    )


class PloRamSyspageLoader(TerminalHarness, PloInterface):
    """Base class for harnesses that load app binaries and file blobs into RAM and register them with PLO.

    Subclasses must implement ``__call__`` to define the specific loading mechanism
    (e.g. GDB, pyocd) and PLO registration steps.

    Subclasses must define ``page_sz``, ``source_device``, and ``destination_map`` as class attributes.
    """

    page_sz: int
    source_device: str
    destination_map: str

    def __init__(
        self,
        dut: Dut,
        apps: Sequence[AppOptions],
        files: Sequence[FileOptions] = (),
        root_dir: Optional[Path] = None,
    ):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.apps = apps
        self.files = files
        if files and root_dir is None:
            raise ValueError(f"{self.__class__.__name__}: root_dir is required when files is non-empty")
        self._root_dir = root_dir

    def _aligned_size(self, path: Path) -> int:
        """Returns file size rounded up to the next page boundary."""
        sz = path.stat().st_size
        offset = (self.page_sz - sz) % self.page_sz
        return sz + offset

    def _register_apps_in_plo(self, offset: int, app_host_dir: Path) -> int:
        """Registers app binaries in syspage at the given memory offset. Returns updated offset."""
        for app in self.apps:
            path = app_host_dir / Path(app.file)
            sz = path.stat().st_size
            self.alias(app.file, offset=offset, size=sz)
            self.app(self.source_device, app.file, self.destination_map, self.destination_map)
            offset += self._aligned_size(path)
        return offset

    def _register_files_in_plo(self, offset: int) -> int:
        """Registers file blobs in syspage at the given memory offset. Returns updated offset."""
        for f in self.files:
            path = self._root_dir / Path(f.file).relative_to("/")
            basename = Path(f.file).name
            sz = path.stat().st_size
            self.alias(basename, offset=offset, size=sz)
            self.blob(self.source_device, basename, self.destination_map)
            offset += self._aligned_size(path)
        return offset

    @abstractmethod
    def __call__(self) -> None:
        """Loads apps and file blobs into memory and registers them in syspage."""


class PloHarness(IntermediateHarness, PloInterface):
    """Basic harness for the plo bootloader that initialize the device.

    This harness enters the plo bootloader and loads binaries to syspage if it's necessary.

    Attributes:
        dut: Device on which harness will be run.
        syspage_loader: Optional syspage loader harness to copy the files/binaries to the syspage.

    """

    def __init__(self, dut: Dut, syspage_loader: Optional[Callable[[], None]] = None):
        IntermediateHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.dut = dut
        self.syspage_loader = syspage_loader

    def __call__(self, result: TestResult) -> TestResult:
        result.set_stage(TestStage.FLASH)
        self.enter_bootloader()
        self.wait_prompt()
        if self.syspage_loader:
            self.syspage_loader()
        self.go()
        return self.next_harness(result)
