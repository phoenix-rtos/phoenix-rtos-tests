from typing import Callable, Optional

from pexpect import TIMEOUT, EOF

from trunner.dut import Dut
from trunner.types import Status, TestResult
from .base import TerminalHarness, HarnessError


class PyHarness(TerminalHarness):
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
        result = TestResult(status=Status.FAIL)
        test_result = None

        try:
            # TODO maybe we should catch the output from the test, for example based on test configuration
            test_result = self.pyharness(self.dut)

            if test_result is None:
                test_result = TestResult(status=Status.OK)

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

        return result
