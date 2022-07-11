#
# Phoenix-RTOS test runner
#
# Harnesses for test frameworks supported in the Test Runner - factory
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau, Jakub Sarzyński
#

from .unity import UnitTestHarness
from .busybox import BusyboxTestHarness
from .mbedtls import MbedtlsTestHarness


class TestHarnessFactory:
    @staticmethod
    def create(test_type):
        if test_type == 'unit':
            return UnitTestHarness.harness
        if test_type == 'busybox':
            return BusyboxTestHarness.harness
        if test_type == 'mbedtls':
            return MbedtlsTestHarness.harness
        else:
            raise ValueError(f"Unknown test type: {test_type}")
