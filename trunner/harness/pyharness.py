from __future__ import annotations
import inspect
import warnings
from typing import Dict, Tuple

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
        pyharness_fn,
        kwargs,
    ):
        super().__init__()
        self.dut: Dut = dut
        self.ctx: TestContext = ctx
        self.pyharness = pyharness_fn
        self.kwargs = kwargs

    def resolve_pyharness_args(self, result: TestResult) -> Tuple[Tuple, Dict]:
        parameters = inspect.signature(self.pyharness).parameters
        kwargs = {**self.kwargs, **self.ctx.kwargs}  # always prioritize kwargs from context

        # Given the parameter list of the `harness` function, attempt to match the arguments provided.
        # The `harness` function may be defined in one of the following ways:
        # Based on parameter list of harness function try to fit arguments.
        # - def harness(dut): (with only dut as a required argument)
        # - def harness(dut, ctx): (with dut and context as required arguments)
        # - def harness(dut, ctx, result): (with dut, context and result as required arguments)
        # - def harness(dut, **kwargs): (with dut as a required argument and additional keyword arguments)
        # - def harness(dut, ctx, **kwargs): (with both dut and context required arguments and additional keyword args)
        # - def harness(dut, ctx, result, **kwargs): (with both dut, context and result required arguments
        #   and additional keyword arguments)

        if len(parameters) == 0 or len(parameters) > 4:
            raise HarnessError(
                "the harness function can be defined only with one, two or three parameters! (dut, ctx and kwargs)"
            )

        have_kwargs = any(param.kind == param.VAR_KEYWORD for param in parameters.values())

        if len(parameters) != 4 or not have_kwargs:
            warnings.warn("Deprecated harness signature - use `harness(dut, ctx, result, **kwargs)`",
                          DeprecationWarning)

        target_positinal_args_cnt = len(parameters) - int(have_kwargs)
        positional_args = (self.dut, self.ctx, result)[:target_positinal_args_cnt]

        return positional_args, kwargs if have_kwargs else {}

    def __call__(self, result: TestResult) -> TestResult:
        test_result = None

        args, kwargs = self.resolve_pyharness_args(result)
        try:
            # TODO maybe we should catch the output from the test, for example based on test configuration
            test_result = self.pyharness(*args, **kwargs)

            if test_result is None:
                test_result = TestResult(status=Status.OK)

        except (TIMEOUT, EOF) as e:
            result.fail_pexpect(self.dut, e)
        except UnicodeDecodeError as e:
            result.fail_decode(self.dut, e)
        except AssertionError as e:
            result.fail_assertion(self.dut, e)
        except HarnessError as e:
            result.fail_harness_exception(e)
        except Exception:
            result.fail_unknown_exception()
        finally:
            # Ensure that the type of the return value from pyharness is correct, as it can be either TestResult or None
            if test_result is not None:
                if not isinstance(test_result, TestResult):
                    raise HarnessError(
                        f"harness returned unknown type, expected {type(result).__name__}, got"
                        f" {type(test_result).__name__}"
                    )

                result.overwrite(test_result)

        return result
