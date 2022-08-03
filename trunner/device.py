# Imports of available runners
from .runners import HostRunner, QemuRunner, IMXRT106xRunner, IMXRT117xRunner, STM32L4Runner, ZYNQ7000ZedboardRunner

from .config import DEVICE_SERIAL_USB, PHOENIXD_PORT, QEMU_TARGETS


class RunnerFactory:
    @staticmethod
    def create(target, serial, log=False):
        if target in QEMU_TARGETS:
            return QemuRunner(target, log=log)
        if target == 'host-generic-pc':
            return HostRunner(target, log=log)
        if target == 'armv7m7-imxrt106x-evk':
            return IMXRT106xRunner(target, (serial, DEVICE_SERIAL_USB), PHOENIXD_PORT, is_rpi_host=True, log=log)
        if target == 'armv7m7-imxrt117x-evk':
            return IMXRT117xRunner(target, serial, PHOENIXD_PORT, is_rpi_host=True, log=log)
        if target == 'armv7a9-zynq7000-zedboard':
            return ZYNQ7000ZedboardRunner(target, serial, PHOENIXD_PORT, is_rpi_host=True, log=log)
        if target == 'armv7m4-stm32l4x6-nucleo':
            return STM32L4Runner(target, serial, log=log)

        raise ValueError(f"Unknown Runner target: {target}")
