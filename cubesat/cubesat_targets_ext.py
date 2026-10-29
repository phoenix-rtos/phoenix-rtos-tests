from collections.abc import Callable
from typing import TextIO

import pexpect


from cubesat.grmon import GrmonError, GrmonProcess, VibemonError, VibemonProcess
from trunner.ctx import TestContext
from trunner.dut import SerialDut
from trunner.harness import (
    HarnessBuilder,
    ShellHarness,
    TestStartRunningHarness,
)
from trunner.harness.base import FlashError, IntermediateHarness
from trunner.host import Host
from trunner.target import TargetBase
from trunner.target.base import find_port
from trunner.target.emulated import QemuTarget
from trunner.types import TestOptions, TestResult


class RISCV64GenericCubesatTarget(QemuTarget):
    name = "riscv64-generic-cubesat"

    def __init__(self):
        super().__init__("riscv64-generic-cubesat.sh")

    @classmethod
    def from_context(cls, _: TestContext):
        return cls()


class ResetHangHarness(IntermediateHarness):
    """Quick workaround harness for hanged tests"""

    def __init__(self, dut):
        super().__init__()
        self.dut = dut

    def __call__(self, result: TestResult) -> TestResult:
        self.dut.send("\003")
        return self.next_harness(result)


class RISCV64Gr765(TargetBase):
    rootfs = False
    sbi_file = "sbi-gr765.elf"
    part_kernel = "part_kernel.img"
    part_kernel_offset = 0x10040000
    rofs = True
    rofs_mnt_point = "/rofs"
    part_rofs = "part_rofs.img"
    part_rofs_offset = 0x40000000

    def __init__(self, host: Host, port: str | None = None, baudrate: int = 115200, vibemon: bool = False):
        if port is None:
            port = find_port("USB2")

        self.vibemon = vibemon
        self.host = host
        self.dut = SerialDut(port, baudrate, encoding="utf-8", codec_errors="ignore")
        super().__init__()

    @classmethod
    def from_context(cls, _: TestContext) -> "RISCV64Gr765CubesatTarget":  # type: ignore[override]
        assert _.host is not None

        vibemon = False
        if "vibemon" in _.kwargs and _.kwargs["vibemon"] == "True":
            vibemon = True

        return cls(_.host, _.port, _.baudrate, vibemon)

    def _mnt_rofs(self):
        mnt = self.rofs_mnt_point
        try:
            self.dut.expect_exact(self.shell_prompt)
            self.dut.sendline(f"sysexec -d rofs {self.part_rofs_offset}:{mnt}")
            self.dut.expect_exact(f"rofs: mounted at {mnt}")
            self.dut.sendline(f"mkdir var")
            self.dut.expect_exact(self.shell_prompt)
            self.dut.sendline(f"mkdir tmp")
            self.dut.expect_exact(self.shell_prompt)
            self.dut.sendline(f"mkdir bin")
            self.dut.expect_exact(self.shell_prompt)
            self.dut.sendline(f"mkdir etc")
            self.dut.expect_exact(self.shell_prompt)
            self.dut.sendline(f"ln -s {mnt}/root/etc/model_data etc/model_data")
            self.dut.expect_exact(self.shell_prompt)
            self.dut.sendline("dmesg")
        except (pexpect.TIMEOUT, pexpect.EOF) as e:
            raise FlashError(msg=str(e)) from e

    def flash_dut(self, host_log: TextIO):
        if self.vibemon:
            with VibemonProcess(host_log=host_log, cwd=self.boot_dir()).run() as vibemon:
                try:
                    vibemon.load(self.sbi_file)
                    vibemon.load(self.part_kernel, binary=True, offset=self.part_kernel_offset)
                    if self.rofs:
                        vibemon.load(self.part_rofs, binary=True, offset=self.part_rofs_offset)
                    vibemon.run_and_resume()
                except VibemonError as e:
                    raise FlashError(msg=str(e)) from e
        else:
            with GrmonProcess(host_log=host_log, cwd=self.boot_dir()).run() as grmon:
                try:
                    grmon.load(self.sbi_file)
                    grmon.load(self.part_kernel, binary=True, offset=self.part_kernel_offset)
                    if self.rofs:
                        grmon.load(self.part_rofs, binary=True, offset=self.part_rofs_offset)
                    grmon.detach()
                except GrmonError as e:
                    raise FlashError(msg=str(e)) from e

        if self.rofs:
            self._mnt_rofs()

    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:

        if self.rofs and test.shell.binary and test.shell.path:
            cmd = f"{self.rofs_mnt_point}/root/{test.shell.path}/{test.shell.binary}"
            test.shell.cmd = [cmd]
            test.cmd = cmd

        builder = HarnessBuilder()

        builder.add(ResetHangHarness(self.dut))

        if test.shell is not None:
            builder.add(ShellHarness(self.dut, self.shell_prompt, test.shell.cmd, suppress_dmesg=False))
        else:
            builder.add(TestStartRunningHarness())

        builder.add(test.harness)
        return builder.get_harness()


class RISCV64Gr765CubesatTarget(RISCV64Gr765):
    name = "riscv64-gr765-cubesat"


class RISCV64Gr765Vcu118Target(RISCV64Gr765):
    name = "riscv64-gr765-vcu118"


def register_extension():
    return {"target": [RISCV64Gr765CubesatTarget, RISCV64Gr765Vcu118Target, RISCV64GenericCubesatTarget]}
