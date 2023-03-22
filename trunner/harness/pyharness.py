from __future__ import annotations
import inspect
from typing import Callable, Optional, Tuple

from pexpect import TIMEOUT, EOF

from trunner.ctx import TestContext
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

    def __init__(
        self,
        dut: Dut,
        ctx: TestContext,
        pyharness_fn: Callable[[Dut], Optional[TestResult]] | Callable[[Dut, TestContext], Optional[TestResult]],
    ):
        super().__init__()
        self.dut: Dut = dut
        self.ctx: TestContext = ctx
        self.pyharness = pyharness_fn

    def resolve_pyharness_args(self) -> Tuple[Dut] | Tuple[Dut, TestContext]:
        parameters = inspect.signature(self.pyharness).parameters

        if len(parameters) == 0 or len(parameters) > 2:
            raise HarnessError("harness can be defined only with one or two parameters! (dut and ctx)")
        elif len(parameters) == 2:
            return (self.dut, self.ctx)
        else:
            return (self.dut,)

    def __call__(self) -> Optional[TestResult]:
        result = TestResult(status=Status.FAIL)
        test_result = None

        args = self.resolve_pyharness_args()

        try:
            # TODO maybe we should catch the output from the test, for example based on test configuration
            test_result = self.pyharness(*args)

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
