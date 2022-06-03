# Imports of available runners
from .runners import HostRunner, QemuRunner, IMXRT106xRunner, IMXRT117xRunner, STM32L4Runner

from .config import DEVICE_SERIAL_USB, PHOENIXD_PORT, QEMU_TARGETS


class RunnerFactory:
    @staticmethod
    def create(target, serial, log=False):
        if target in QEMU_TARGETS:
            return QemuRunner(target, log=log)
        if target == 'host-generic-pc':
            return HostRunner(log=log)
        if target == 'armv7m7-imxrt106x-evk':
            return IMXRT106xRunner((serial, DEVICE_SERIAL_USB), PHOENIXD_PORT, is_rpi_host=True, log=log)
        if target == 'armv7m7-imxrt117x-evk':
            return IMXRT117xRunner(serial, PHOENIXD_PORT, is_rpi_host=True, log=log)
        if target == 'armv7m4-stm32l4x6-nucleo':
            return STM32L4Runner(serial, log=log)

        raise ValueError(f"Unknown Runner target: {target}")
