from __future__ import annotations
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, TYPE_CHECKING

if TYPE_CHECKING:
    # TargetBase uses TestContext, fix circular import
    from trunner.target import TargetBase
    from trunner.host import Host


@dataclass(frozen=True)
class TestContext:
    """Global context of the runner.

    Attributes:
        target: Target on which the runner was launched.
        host: Host on which the runner was launched.
        port: Path to serial port if target uses it.
        baudrate: Baudrate for serial. It is used if port is set.
        project_path: Path to phoenix-rtos-project directory.
        nightly: Determine if it's nigthly run.
        should_flash: True if device should be flashed.
        should_test: True if tests should be run.
        verbosity: Verbose level of the output of tests.
        stream_output: Stream DUT output to stdout during test execution.
        output: If not None - file name stem to store the test results ([stem].csv, [stem].xml).
    """

    port: Optional[str]
    baudrate: int
    project_path: Path
    nightly: bool
    logdir: Optional[str]
    should_flash: bool
    should_test: bool
    verbosity: int
    stream_output: bool
    output: Optional[str]
    kwargs: dict = field(default_factory=dict)
    target: Optional[TargetBase] = None
    host: Optional[Host] = None
