from abc import ABC, abstractmethod
from pathlib import Path
from typing import Callable

from serial.tools import list_ports

from trunner.dut import Dut
from trunner.tools import Psu
from trunner.harness import TerminalHarness, PloInterface
from trunner.dut import PortNotFound
from trunner.types import TestOptions, TestResult


def find_port(port_hint: str) -> str:
    port = None

    for p in list_ports.grep(port_hint):
        if port is not None:
            raise PortNotFound(
                "More than one port was found! Maybe more than one device is connected? Hint used to find port:"
                f" {port_hint}"
            )

        port = p

    if port is None:
        raise PortNotFound(
            "Couldn't find port to communicate with device! Make sure device is connected. Hint used to find port:"
            f" {port_hint}"
        )

    return port.device


class PsuPloLoader(TerminalHarness, PloInterface):
    def __init__(self, dut: Dut, psu: Psu):
        TerminalHarness.__init__(self)
        PloInterface.__init__(self, dut)
        self.psu = psu

    def __call__(self):
        """Loads plo image to RAM using psu tool and erases an area intended for rootfs."""
        self.psu.run()
        self.wait_prompt()


class TargetBase(ABC):
    """Base class to represent the target device.

    Attributes:
        name: A string literal that is used to represent the target.
        shell_prompt: Prompt that target shell uses.
        rootfs: Flag that tells if target uses real filesystem.
        experimental: If set, target must be explcitly specified in yaml config.
    """

    name = "base"
    shell_prompt = "(psh)% "
    rootfs = True
    experimental = False

    def __init__(self):
        self.project_dir = self._project_dir()
        self.prompt_timeout = -1
        self.dut: Dut

    @classmethod
    @abstractmethod
    def from_context(cls, _):
        pass

    def _project_dir(self) -> str:
        """Returns host directory with phoenix-rtos-project repository."""
        file_path = Path(__file__).resolve()
        for path in file_path.parents:
            if path.name == "phoenix-rtos-tests":
                return str(path.parent)

        raise FileNotFoundError("phoenix-rtos-tests directory is missing")

    def exec_dir(self) -> Path:
        """Returns a directory where test binaries are located in rootfs."""
        return Path("/bin")

    def root_dir(self) -> Path:
        """Returns a host path to root directory for the specified target"""
        return Path(f"{self.project_dir}/_fs/{self.name}/root")

    def boot_dir(self) -> str:
        """Returns a directory with system and bootloader images."""
        return f"{self.project_dir}/_boot/{self.name}"

    @abstractmethod
    def flash_dut(self):
        """Flashes the system image into target device."""

    @abstractmethod
    def build_test(self, test: TestOptions) -> Callable[[TestResult], TestResult]:
        """Returns the complete harness to run the test secified in `test` argument"""
