import importlib.util
import os

from pathlib import Path
from typing import Callable, List, Sequence, Tuple

from trunner.target import TargetBase
from trunner.host import Host


class ExtensionError(Exception):
    pass


def read_extensions_paths() -> List[Path]:
    """Returns the list of extension file paths found in directories specified in PHOENIX_TRUNNER_EXT env variable."""

    paths = os.getenv("PHOENIX_TRUNNER_EXT")
    if not paths:
        return []

    result = []

    for p in paths.split(os.pathsep):
        p = Path(p)
        if not p.is_dir():
            raise ExtensionError(f"Extension path {p} must be a dir!")

        result.extend(p.glob("**/*_ext.py"))

    return result


def load_register_fn(path: Path):
    """Loads and returns the register_extension function defined in python file in path argument."""

    spec = importlib.util.spec_from_file_location(path.name, path.absolute())
    if not spec:
        raise ExtensionError(f"Failed to load spec from location {path}")

    extension_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extension_module)

    if not hasattr(extension_module, "register_extension"):
        raise ExtensionError(f"Extension at {path} doesn't define register_extension function!")

    return extension_module.register_extension


def register_extensions(extensions: Sequence[Callable[[], dict]]) -> Tuple[List[TargetBase], List[Host]]:
    """Returns lists of targets and hosts returned from given extension functions."""

    targets, hosts = [], []

    for register_fn in extensions:
        extension = register_fn()

        if "target" in extension:
            targets.append(extension["target"])

        if "host" in extension:
            hosts.append(extension["host"])

    return targets, hosts


def load_extensions() -> Tuple[List[TargetBase], List[Host]]:
    """Returns the external targets and hosts defined by user.

    This function loads the external targets and hosts found in extensions specified
    by PHOENIX_TRUNNER_EXT environment variable. The path added to PHOENIX_TRUNNER_EXT
    should be a directory with a file or files that ends with *_ext.py suffix. To successfully
    load the extension, it must define a function register_extension() that returns dict with
    keywords "host" and "target" mapping new classes.

    Example of such extension:
        File phoenix-rtos-project/dummy_target_tests/dummy_ext.py

        from trunner.target import IA32GenericQemuTarget
        from trunner.host import EmulatorHost


        class DummyTarget(IA32GenericQemuTarget):
            name = "ia32-dummy-qemu"


        class DummyHost(EmulatorHost):
            name = "emu-dummy-host"


        def register_extension():
            return {
                "target": DummyTarget,
                "host": DummyHost,
            }
    """

    paths = read_extensions_paths()
    register_fns = []

    for path in sorted(paths, key=lambda p: p.name):
        fn = load_register_fn(path)
        if fn:
            register_fns.append(fn)

    return register_extensions(register_fns)
