import io
import socket
import subprocess
import time
from contextlib import contextmanager
from pathlib import Path
from typing import Callable, Optional, Sequence, TextIO

import pexpect

from trunner.ctx import TestContext
from trunner.dut import Dut, SerialDut
from trunner.harness import (
    FlashError,
    HarnessBuilder,
    PloHarness,
    PloInterface,
    PloRamSyspageLoader,
    Rebooter,
    RebooterHarness,
    ShellHarness,
    TestStartRunningHarness,
)
from trunner.host import Host
from trunner.tools import GdbInteractive
from trunner.types import AppOptions, FileOptions, TestOptions, TestResult
from .base import TargetBase


class ARMV7R5FSomRebooter(Rebooter):
    """Rebooter for armv7r5f-zynqmp-som."""

    def __init__(self, host: Host, dut: Dut, openocd_config: str):
        super().__init__(host, dut)
        self.openocd_config = openocd_config

    def __call__(self, flash: bool = False, hard: bool = False):
        """Resets the board. Uses debugger for soft reboot, prompts user on non-GPIO hosts."""
        if flash or hard:
            self._set_flash_mode(flash)

        if hard:
            if self.host.has_gpio():
                self.host.set_power(False)
                time.sleep(1)
                self.dut.clear_buffer()
                self.host.set_power(True)
                time.sleep(1)
            else:
                self._reboot_dut_text(hard=hard)
        else:
            self._reboot_by_debugger()

    def _set_flash_mode(self, flash: bool):
        self.host.set_flash_mode(not flash)

    def _reboot_by_debugger(self):
        """Reset the board via SRST through OpenOCD JLink."""
        args = [
            "openocd",
            "-c adapter driver jlink",
            "-c transport select jtag",
            "-c adapter speed 12000",
            "-c reset_config srst_only",
            f"-f{self.openocd_config}",
            "-c init",
            "-c reset run",
            "-c sleep 500",
            "-c exit",
        ]

        try:
            subprocess.run(args, capture_output=True, text=True, timeout=30, check=True)
        except subprocess.TimeoutExpired as e:
            raise FlashError(msg="OpenOCD reboot timed out") from e
        except subprocess.CalledProcessError as e:
            raise FlashError(msg=f"OpenOCD reboot failed with code {e.returncode}", output=e.stderr) from e
        except FileNotFoundError as e:
            raise FlashError(msg=str(e)) from e

        self.dut.clear_buffer()


class ARMV7R5FSomSyspageLoader(PloRamSyspageLoader):
    """Loads app binaries and file blobs into RAM via JLink OpenOCD GDB server
    and registers them with PLO on armv7r5f-zynqmp-som."""

    source_device = "ramdisk"
    destination_map = "ddr"
    ramdisk_addr = 0x8000000
    page_sz = 0x200
    max_load_attempts = 3

    def __init__(
        self,
        dut: Dut,
        apps: Sequence[AppOptions],
        gdb: GdbInteractive,
        openocd_config: str,
        files: Sequence[FileOptions] = (),
        root_dir: Optional[Path] = None,
    ):
        super().__init__(dut, apps, files, root_dir)
        self.gdb = gdb
        self.openocd_config = openocd_config

    @contextmanager
    def _openocd_gdb_server(self):
        """Starts OpenOCD as a GDB server with JLink adapter, halting the target."""

        args = [
            "-c adapter driver jlink",
            "-c transport select jtag",
            "-c adapter speed 12000",
            f"-f{self.openocd_config}",
            "-c targets uscale.r5.0",
            "-c init",
            "-c halt",
        ]
        logfile = io.StringIO()
        proc = pexpect.spawn("openocd", args, encoding="ascii", logfile=logfile, timeout=30)
        try:
            # 'init' opens GDB server ports and prints "Listening on port" before halt runs.
            # Wait for the R5.0 port specifically, then wait for 'halt' to actually complete.
            proc.expect_exact(f"Listening on port {self.gdb.port}", timeout=20)
            idx = proc.expect(["halted", "Error: Error waiting for halt", pexpect.TIMEOUT], timeout=15)
            if idx != 0:
                raise FlashError(msg="OpenOCD failed to halt uscale.r5.0", output=logfile.getvalue())
            yield
        finally:
            try:
                with socket.create_connection(("localhost", 4444), timeout=2.0) as sock:
                    sock.sendall(b"shutdown\n")
                proc.expect(pexpect.EOF, timeout=15)
                proc.close()
            except Exception:
                proc.close(force=True)

    def _gdb_detach_and_resume(self):
        """Resume target via OpenOCD monitor command and disconnect GDB cleanly."""

        self.gdb.proc.sendline("monitor resume")
        self.gdb.expect_prompt()
        self.gdb.proc.sendline("disconnect")
        self.gdb.expect_prompt()
        self.gdb.proc.sendline("quit")
        self.gdb.proc.expect(pexpect.EOF, timeout=5)

    def _load_via_gdb(self) -> None:
        """Starts OpenOCD GDB server, loads all apps/blobs via GDB and resumes the target."""
        with self._openocd_gdb_server():
            with self.gdb.run():
                offset = 0
                apps_offset = offset
                self.gdb.connect()

                for app in self.apps:
                    path = self.gdb.cwd / Path(app.file)
                    aligned_sz = self._aligned_size(path)
                    self.gdb.load(path, self.ramdisk_addr + offset)
                    offset += aligned_sz

                files_offset = offset
                for f in self.files:
                    path = self._root_dir / Path(f.file).relative_to("/")
                    aligned_sz = self._aligned_size(path)
                    self.gdb.load(path, self.ramdisk_addr + offset)
                    offset += aligned_sz

                self._gdb_detach_and_resume()

        self._register_apps_in_plo(apps_offset, app_host_dir=self.gdb.cwd)
        self._register_files_in_plo(files_offset)

    def __call__(self) -> None:
        last_exc: FlashError
        for attempt in range(1, self.max_load_attempts + 1):
            try:
                self._load_via_gdb()
                return
            except FlashError as exc:
                last_exc = exc
                if attempt < self.max_load_attempts:
                    # JTAG-DP STICKY ERROR / halt timeout are transient after a previous
                    # GDB session — give the adapter a moment to recover before retrying.
                    time.sleep(2)
        raise last_exc


class ARMV7R5FSomTarget(TargetBase):
    """Target for armv7r5f-zynqmp-som (Zynq UltraScale+ RPU R5 on SOM board)."""

    name = "armv7r5f-zynqmp-som"
    rootfs = False
    gdb_port = 3334
    openocd_adapter_speed = 12000
    openocd_config_file = "scripts/openocd/zynqmp/xilinx_zynqmp.cfg"
    plo_ram_image = "plo-ram.img"
    plo_ram_addr = 0xFFFC0000
    flash_image = "flash0.disk"
    ramdisk_addr = 0x08000000
    flash_copy_size = 0x4000000

    def __init__(self, host: Host, port: Optional[str] = None, baudrate: int = 115200):
        if port is None:
            port = "/dev/ttyUSB0"

        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        self.openocd_config = f"{self._project_dir()}/{self.openocd_config_file}"
        self.rebooter = ARMV7R5FSomRebooter(host, self.dut, self.openocd_config)
        super().__init__()

    @classmethod
    def from_context(cls, ctx: TestContext):
        return cls(ctx.host, ctx.port, ctx.baudrate)

    def _run_openocd_jlink(self, commands: list, cwd: Optional[str] = None, timeout: int = 120):
        """Run a one-shot OpenOCD command with JLink adapter."""
        args = [
            "openocd",
            "-c adapter driver jlink",
            "-c transport select jtag",
            f"-c adapter speed {self.openocd_adapter_speed}",
            f"-f{str(self.openocd_config)}",
        ]
        args.extend(commands)

        try:
            subprocess.run(args, capture_output=True, text=True, timeout=timeout, cwd=cwd, check=True)
        except subprocess.TimeoutExpired as e:
            raise FlashError(msg=f"OpenOCD timed out after {timeout}s") from e
        except subprocess.CalledProcessError as e:
            raise FlashError(
                msg=f"OpenOCD exited with code {e.returncode}",
                output=e.stderr,
            ) from e
        except FileNotFoundError as e:
            raise FlashError(msg=str(e)) from e

    def flash_dut(self, host_log: TextIO):
        """Flashes the system image via JTAG + PLO."""

        plo = PloInterface(self.dut)

        self.rebooter(flash=True, hard=True)

        self._run_openocd_jlink(
            commands=[
                "-c targets uscale.r5.0",
                "-c init",
                "-c start_rpu 0",
                "-c halt",
                f"-c load_image {self.plo_ram_image} {self.plo_ram_addr:#x} bin",
                "-c resume",
                "-c exit",
            ],
            cwd=self.boot_dir(),
        )
        plo.wait_prompt()

        self._run_openocd_jlink(
            commands=[
                "-c targets uscale.r5.0",
                "-c init",
                "-c halt",
                f"-c load_image {self.flash_image} {self.ramdisk_addr:#x} bin",
                "-c resume",
                "-c exit",
            ],
            cwd=self.boot_dir(),
            timeout=300,
        )

        plo.cmd(f"copy ramdisk 0x0 0x{self.flash_copy_size:x} flash0 0x0 0x{self.flash_copy_size:x}", timeout=600)

        self.rebooter(flash=False, hard=True)

    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:
        builder = HarnessBuilder()

        if test.should_reboot:
            builder.add(RebooterHarness(self.rebooter))

        if test.bootloader is not None:
            syspage_loader = None

            if test.bootloader.apps or test.bootloader.files:
                syspage_loader = ARMV7R5FSomSyspageLoader(
                    dut=self.dut,
                    apps=test.bootloader.apps,
                    gdb=GdbInteractive(
                        port=self.gdb_port,
                        cwd=self.root_dir() / test.shell.path,
                    ),
                    openocd_config=self.openocd_config,
                    files=test.bootloader.files,
                    root_dir=self.root_dir(),
                )

            builder.add(PloHarness(self.dut, syspage_loader=syspage_loader))

        if test.shell is not None:
            builder.add(
                ShellHarness(
                    self.dut,
                    self.shell_prompt,
                    test.shell.cmd,
                    # /dev/klogctl not supported on this target
                    suppress_dmesg=False,
                )
            )
        else:
            builder.add(TestStartRunningHarness())

        builder.add(test.harness)

        return builder.get_harness()
