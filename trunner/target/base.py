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


class TargetBase:
    name = "base"
    shell_prompt = "(psh)% "
    rootfs = True
    experimental = False

    def __init__(self):
        self.project_dir = self._project_dir()

    def _project_dir(self) -> str:
        file_path = Path(__file__).resolve()
        # file_path is phoenix-rtos-project/phoenix-rtos-tests/trunner/target/base.py
        project_dir = file_path.parent.parent.parent.parent
        return str(project_dir)

    def exec_dir(self) -> str:
        return "/bin"

    def bin_dir(self) -> str:
        return self.project_dir + f"/_fs/{self.name}/root/bin"

    def boot_dir(self) -> str:
        return self.project_dir + f"/_boot/{self.name}"

    def flash_dut(self):
        raise NotImplementedError

    def build_test(self, test: TestOptions) -> Callable[[], Optional[TestResult]]:
        raise NotImplementedError
