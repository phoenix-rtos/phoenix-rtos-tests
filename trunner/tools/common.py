from typing import Type
from functools import wraps
from pathlib import Path

from trunner.harness import HarnessError


def add_output_to_exception(*excludes: Type[HarnessError]):
    """Decorator to add the output of process to the exception.

    It should be nested in the contextmanager decorator"""

    def decorator(generator):
        @wraps(generator)
        def wrapper(self, *args, **kwargs):
            try:
                yield from generator(self, *args, **kwargs)
            except HarnessError as e:
                if not isinstance(e, excludes) and self.proc:
                    command = Path(self.proc.command).name
                    e.add_additional_info(command + " output", self.output)

                raise

        return wrapper

    return decorator
