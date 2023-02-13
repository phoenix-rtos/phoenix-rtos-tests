from __future__ import annotations
import importlib.util
import shlex
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Set, TYPE_CHECKING

import yaml

from trunner.harness import PyHarness, unity_harness
from trunner.host import Host
from trunner.types import AppOptions, BootloaderOptions, TestOptions, ShellOptions

if TYPE_CHECKING:
    # TargetBase uses TestContext, fix circular import
    from trunner.target import TargetBase


class ParserError(Exception):
    pass


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
    """

    target: Optional[TargetBase]
    host: Host
    port: Optional[str]
    baudrate: int
    project_path: Path
    nightly: bool
    should_flash: bool
    should_test: bool
    verbosity: int


def array_value(array: Dict[str, List[str]]) -> List[str]:
    """Logic for setting parameters if there are multiple to choose from.

    `value` keyword is hard set of parameter
    `include` keyword appends parameter
    `excludes` keyword removes parameter
    """

    value = set(array.get("value", []))
    value |= set(array.get("include", []))
    value -= set(array.get("exclude", []))

    return list(value)


def is_array(array: dict) -> bool:
    array_keys = {"value", "include", "exclude"}
    is_arr = bool(array.keys() & array_keys)
    unknown_keys = array.keys() - array_keys
    return is_arr and len(unknown_keys) == 0


class ConfigParser:
    @dataclass
    class MainConfig:
        targets: Optional[Set] = None
        type: Optional[str] = None
        ignore: bool = False
        nightly: bool = False

    def __init__(self, ctx: TestContext):
        self.ctx = ctx
        self.yaml_path = Path("/")
        self.main = ConfigParser.MainConfig()
        self.raw_main = {}
        self.test = TestOptions()

    def _parse_type(self, config: dict):
        test_type = config.get("type", self.main.type)
        if not test_type:
            test_type = "harness"

        if test_type == "harness":
            self._parse_pyharness(config)
        elif test_type == "unity":
            self._parse_unity()
        else:
            raise ParserError("unknown key!")

    def _parse_pyharness(self, config: dict):
        path = config.get("harness", self.raw_main.get("harness"))
        if path is None:
            raise ParserError("test is of type 'harness' but there is no \"harness\" keyword")

        path = Path(path)

        if not path.is_absolute():
            # If path is not absolute then it must be relative to the directory of yaml
            path = self.yaml_path.parent / path
            if not path.is_absolute():
                raise ParserError("yml path is not absolute!")

        spec = importlib.util.spec_from_file_location("harness", path.absolute())
        if not spec:
            raise ParserError("error during loading harness module")

        harness_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(harness_module)

        if hasattr(harness_module, "harness"):
            harness_fn = harness_module.harness
        else:
            raise ParserError(f"harness function has not been found in {path}")

        self.test.harness = PyHarness(self.ctx.target.dut, harness_fn)

    def _parse_unity(self):
        self.test.harness = PyHarness(self.ctx.target.dut, unity_harness)

    def _parse_load(self, config: dict):
        apps = config.get("load", [])
        apps_to_boot = []

        for app in apps:
            file = app.get("app", None)
            if not file:
                raise ParserError("generic error")

            opts = AppOptions(file=file)

            if "source" in app:
                opts.source = app["source"]
            if "imap" in app:
                opts.imap = app["imap"]
            if "dmap" in app:
                opts.dmap = app["dmap"]
            if "exec" in app:
                opts.exec = app["exec"]

            apps_to_boot.append(opts)

        if self.test.shell is not None and self.test.shell.binary:
            # If it is non rootfs target then we have to load test binary
            for app in apps_to_boot:
                if app.file == self.test.shell.binary:
                    break
            else:
                if not self.ctx.target.rootfs:
                    apps_to_boot.append(
                        AppOptions(
                            file=self.test.shell.binary,
                            exec=False,
                        )
                    )

        if apps_to_boot:
            self.test.bootloader = BootloaderOptions(apps=apps_to_boot)
        else:
            self.test.bootloader = None

    def _parse_shell_command(self, config: dict):
        execute_binary = True
        cmd = config.get("execute")
        if cmd is None:
            cmd = config.get("run")
            if cmd is None:
                # There is nothing to run, just parse prompt
                self.test.shell = ShellOptions()
                return

            execute_binary = False

        cmd = shlex.split(cmd)
        if not cmd:
            raise ParserError("execute/run attribute cannot be empty")

        if execute_binary:
            prefix = self.ctx.target.exec_dir() + "/" if self.ctx.target.rootfs else "sysexec "
            parsed_cmd = shlex.split(prefix + cmd[0]) + cmd[1:]
            binary = cmd[0]
        else:
            binary = None
            parsed_cmd = cmd

        self.test.shell = ShellOptions(
            binary=binary,
            cmd=parsed_cmd,
        )

    def _parse_ignore(self, config: dict) -> None:
        ignore = config.get("ignore", self.main.ignore)

        if not isinstance(ignore, bool):
            raise ParserError(f"ignore must be a boolean value (true/false) not {ignore}")

        self.test.ignore = ignore

    def _parse_nightly(self, config: dict) -> None:
        nightly = config.get("nightly", self.main.nightly)

        if not isinstance(nightly, bool):
            raise ParserError(f"nightly must be a boolean value (true/false) not {nightly}")

        self.test.nightly = nightly

    def _build_targets(self, targets: dict):
        default_target = [self.ctx.target.name] if not self.ctx.target.experimental else []
        values = targets.get("value", default_target)
        include = targets.get("include", [])
        exclude = targets.get("exclude", [])
        return (set(values) | set(include)) - set(exclude)

    def _parse_targets(self, config: dict) -> None:
        targets = config.get("targets")

        if targets and (not isinstance(targets, dict) or not is_array(targets)):
            raise ParserError('"targets" should be a dict with "value", "include", "exclude" keys.')

        if not targets and not self.main.targets:
            targets = set()
            if not self.ctx.target.experimental:
                targets.add(self.ctx.target.name)
        elif not targets:
            targets = self.main.targets
        elif not self.main.targets or "value" in targets:
            targets = self._build_targets(targets)
        else:
            targets["value"] = list(self.main.targets)
            targets = self._build_targets(targets)

        self.test.target = self.ctx.target.name if self.ctx.target.name in targets else None

    def _parse_name(self, config: dict) -> None:
        name = config.get("name")
        if not name:
            raise ParserError("Test name is missing!")

        test_dir = self.yaml_path.parent
        test_dir = test_dir.relative_to(self.ctx.project_path)

        self.test.name = f"{test_dir}/{name}"

    def _parse_config(self, config: dict):
        # Note, the order of parsing is important!
        self.test = TestOptions()

        self._parse_name(config)
        self._parse_targets(config)
        if self.test.target is None:
            self.test = None
            return

        self._parse_nightly(config)
        if self.test.nightly and not self.ctx.nightly:
            self.test = None
            return

        self._parse_ignore(config)
        if self.test.ignore:
            return

        self._parse_shell_command(config)
        self._parse_load(config)
        self._parse_type(config)

    def _load_yaml(self) -> dict:
        assert self.yaml_path is not None

        with open(self.yaml_path, "r", encoding="utf-8") as f_yaml:
            config = yaml.safe_load(f_yaml)

        return config

    def _split_test_config(self, config: dict) -> Tuple[dict, List[dict]]:
        try:
            main = config.pop("test")
            tests = main.pop("tests")
        except KeyError as e:
            raise ParserError(f"required keyword {e} not found in test config") from e

        return main, tests

    def _preprocess_main_config(self, config: dict):
        self.raw_main = config
        self.main = ConfigParser.MainConfig()
        self.main.type = config.get("type", None)

        if "targets" in config:
            self.main.targets = self._build_targets(config["targets"])

        self.main.ignore = config.get("ignore", False)
        self.main.nightly = config.get("nightly", False)

    def parse(self, path: Path) -> List[TestOptions]:
        self.yaml_path = path

        res = []
        config = self._load_yaml()
        main_cfg, test_cfgs = self._split_test_config(config)
        self._preprocess_main_config(main_cfg)

        for cfg in test_cfgs:
            self._parse_config(cfg)
            if self.test is not None:
                res.append(self.test)

        return res
