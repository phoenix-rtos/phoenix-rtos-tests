from trunner.ctx import TestContext
from trunner.dut import Dut
from trunner.types import TestResult, Status


def harness(dut: Dut, ctx: TestContext, result: TestResult):
    dut.expect("Exception", timeout=30)

    return TestResult(status=Status.OK)
