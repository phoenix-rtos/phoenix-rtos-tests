import contextlib
from typing import Callable, Optional
from ptyprocess.ptyprocess import io

from pexpect import TIMEOUT, EOF

from trunner.dut import Dut
from trunner.types import TestResult
from .base import TermHarness, HarnessError


class PyHarness(TermHarness):
    """Harness that wraps basic user harness function.

    This class implements logic to run user defined harnesses and handles errors accordingly.

    Attributes:
        dut: Device on which harness will be run.
        pyharness: User defined harness function.

    """

    def __init__(self, dut: Dut, pyharness_fn: Callable[[Dut], Optional[TestResult]]):
        super().__init__()
        self.dut = dut
        self.pyharness = pyharness_fn

    def __call__(self) -> Optional[TestResult]:
        result = TestResult(status=TestResult.FAIL)
        test_result = None
        output = None

        try:
            # Catch the output printed to stdout
            with contextlib.redirect_stdout(io.StringIO()) as o:
                test_result = self.pyharness(self.dut)

            if test_result is None:
                test_result = TestResult(status=TestResult.OK)

            output = o.getvalue()
        except (TIMEOUT, EOF) as e:
            result.fail_pexpect(self.dut, e)
        except UnicodeDecodeError as e:
            result.fail_decode(self.dut, e)
        except AssertionError as e:
            result.fail_assertion(self.dut, e)
        except Exception:
            result.fail_unknown_exception()
        finally:
            # pyharness can return either TestResult or None. Check if type is correct.
            if test_result is not None:
                if not isinstance(test_result, TestResult):
                    raise HarnessError(
                        f"harness returned unknown type, expected {type(result).__name__}, got"
                        f" {type(test_result).__name__}"
                    )

                result = test_result

        if output is not None:
            result.output = output

        return result
