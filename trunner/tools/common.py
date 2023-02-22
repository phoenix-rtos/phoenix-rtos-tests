from typing import Optional, Type
from functools import wraps
from pathlib import Path

from trunner.harness import HarnessError


def add_output_to_exception(exclude: Optional[Type[HarnessError]] = None):
    """Decorator to add the output of process to the exception.

    It should be nested in the contextmanager decorator"""

    def decorator(generator):
        @wraps(generator)
        def wrapper(self, *args, **kwargs):
            try:
                yield from generator(self, *args, **kwargs)
            except exclude:
                # We are not interested in `exclude` type of exception
                raise
            except HarnessError as e:
                if not self.proc:
                    raise

                command = Path(self.proc.command).name
                e.add_additional_info(command, self.output)
                raise

        return wrapper

    return decorator
