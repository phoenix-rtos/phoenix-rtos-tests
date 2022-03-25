# Imports of available runners
from .runners import HostRunner, QemuRunner, IMXRT106xRunner, IMXRT117xRunner, STM32L4Runner

from .config import PHRTOS_PROJECT_DIR, DEVICE_SERIAL_USB, PHOENIXD_PORT


QEMU_CMD = {
    'ia32-generic': f'{PHRTOS_PROJECT_DIR}/scripts/ia32-generic-test.sh'
}


class RunnerFactory:
    @staticmethod
    def create(target, serial, log=False):
        if target == 'ia32-generic':
            return QemuRunner(QEMU_CMD[target], log=log)
        if target == 'host-pc':
            return HostRunner(log=log)
        if target == 'armv7m7-imxrt106x':
            return IMXRT106xRunner((serial, DEVICE_SERIAL_USB), PHOENIXD_PORT, is_rpi_host=True, log=log)
        if target == 'armv7m7-imxrt117x':
            return IMXRT117xRunner(serial, PHOENIXD_PORT, is_rpi_host=True, log=log)
        if target == 'armv7m4-stm32l4':
            return STM32L4Runner(serial, log=log)

        raise ValueError(f"Unknown Runner target: {target}")
