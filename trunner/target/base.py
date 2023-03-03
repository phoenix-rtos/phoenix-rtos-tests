from abc import ABC, abstractmethod
from pathlib import Path
from typing import Callable, Optional

from serial.tools import list_ports

from trunner.dut import PortNotFound
from trunner.types import TestOptions, TestResult


def find_port(port_hint: str) -> str:
    port = None

    for p in list_ports.grep(port_hint):
        if port is not None:
            raise PortNotFound(f'More than one port can be found using hint "{port_hint}"')

        port = p

    if port is None:
        raise PortNotFound(f'There is not port with that can be found using hint "{port_hint}"')

    return port.device


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
        else:
            raise FileNotFoundError("phoenix-rtos-tests directory is missing")

    def exec_dir(self) -> str:
        """Returns a directory where test binaries are located in rootfs."""
        return "/bin"

    def bin_dir(self) -> str:
        """Returns a directory with binaries that will be loaded by bootloader."""
        return f"{self.project_dir}/_fs/{self.name}/root/bin"

    def boot_dir(self) -> str:
        """Returns a directory with system and bootloader images."""
        return f"{self.project_dir}/_boot/{self.name}"

    @abstractmethod
    def flash_dut(self):
        """Flashes the system image into target device."""

    @abstractmethod
    def build_test(self, test: TestOptions) -> Callable[[], Optional[TestResult]]:
        """Returns the complete harness to run the test secified in `test` argument"""
