import contextlib
from typing import Optional
from ptyprocess.ptyprocess import io

from pexpect import TIMEOUT, EOF

from trunner.types import TestResult
from .base import HarnessBase, HarnessError


class PyHarness(HarnessBase):
    def __init__(self, dut, pyharness_fn):
        self.dut = dut
        self.pyharness = pyharness_fn
        super().__init__()

    def chain(self):
        raise NotImplementedError(f"{self.__class__.__name__} is a terminal harness and it can't be chained")

    def __call__(self) -> Optional[TestResult]:
        result = TestResult(status=TestResult.FAIL)
        test_result = None
        output = None

        try:
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
