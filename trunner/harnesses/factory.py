#
# Phoenix-RTOS test runner
#
# Harnesses for test frameworks supported in the Test Runner - factory
#
# Copyright 2022 Phoenix Systems
# Authors: Damian Loewnau, Jakub Sarzyński, Piotr Nieciecki
#

from .unity import UnitTestHarness
from .busybox import BusyboxTestHarness
from .mbedtls import MbedtlsTestHarness
from .micropython import MicropythonStandardHarness
from .micropython import MicropythonReplHarness
from .lsb_vsx_posix import LsbVsxPosixTestHarness


class TestHarnessFactory:
    @staticmethod
    def create(test_type):
        if test_type == 'unit':
            return UnitTestHarness()
        if test_type == 'busybox':
            return BusyboxTestHarness()
        if test_type == 'mbedtls':
            return MbedtlsTestHarness()
        if test_type == 'micropython_std':
            return MicropythonStandardHarness()
        if test_type == 'micropython_repl':
            return MicropythonReplHarness()
        if test_type == 'lsb_vsx_posix':
            return LsbVsxPosixTestHarness()
        else:
            raise ValueError(f"Unknown test type: {test_type}")
