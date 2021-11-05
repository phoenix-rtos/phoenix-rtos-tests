# Imports of available runners
from .runners import HostRunner, QemuRunner, IMXRT106xRunner, STM32L4Runner

from .config import PHRTOS_PROJECT_DIR, DEVICE_SERIAL_USB


QEMU_CMD = {
    'ia32-generic': (
        'qemu-system-i386',
        [
            '-hda', f'{PHRTOS_PROJECT_DIR}/_boot/phoenix-ia32-generic.disk',
            '-nographic',
            '-monitor', 'none'
        ]
    )
}


class RunnerFactory:
    @staticmethod
    def create(target, serial):
        if target == 'ia32-generic':
            return QemuRunner(*QEMU_CMD[target])
        if target == 'host-pc':
            return HostRunner()
        if target == 'armv7m7-imxrt106x':
            return IMXRT106xRunner((serial, DEVICE_SERIAL_USB))
        if target == 'armv7m4-stm32l4':
            return STM32L4Runner(serial)

        raise ValueError(f"Unknown Runner target: {target}")
