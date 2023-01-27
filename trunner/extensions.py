import importlib.util
import os

from pathlib import Path
from typing import Callable, List, Sequence, Tuple

from trunner.target import TargetBase
from trunner.host import Host


class ExtensionError(Exception):
    pass


def read_extensions_paths() -> List[Path]:
    paths = os.getenv("PHOENIX_TRUNNER_EXT")
    if not paths:
        return []

    result = []

    for p in paths.split(os.pathsep):
        p = Path(p)
        if not p.is_dir():
            raise ExtensionError(f"Extension path {p} must be a dir!")

        result.extend(p.glob("**/*_extension.py"))

    return result


def load_register_fn(path: Path):
    spec = importlib.util.spec_from_file_location(f"{path.name}_module", path.absolute())
    if not spec:
        raise ExtensionError(f"Failed to load spec from location {path}")

    extension_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extension_module)

    if not hasattr(extension_module, "register_extension"):
        raise ExtensionError(f"Extension at {path} doesn't define register_extension function!")

    return extension_module.register_extension


def register_extensions(extensions: Sequence[Callable[[], dict]]) -> Tuple[List[TargetBase], List[Host]]:
    targets, hosts = [], []

    for register_fn in extensions:
        d = register_fn()

        if "target" in d:
            targets.append(d["target"])

        if "host" in d:
            hosts.append(d["host"])

    return targets, hosts


def load_extensions() -> Tuple[List[TargetBase], List[Host]]:
    paths = read_extensions_paths()
    register_fns = []

    for path in sorted(paths, key=lambda p: p.name):
        fn = load_register_fn(path)
        if fn:
            register_fns.append(fn)

    return register_extensions(register_fns)
