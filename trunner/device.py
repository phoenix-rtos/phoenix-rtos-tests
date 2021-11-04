# Imports of available runners
from .runners import HostRunner, QemuRunner, IMXRT106xRunner, Stm32l4Runner

from .config import PHRTOS_PROJECT_DIR, DEVICE_SERIAL, DEVICE_SERIAL_USB


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
    def create(target=None, serial=None):
        if target == 'ia32-generic':
            return QemuRunner(*QEMU_CMD[target])
        if target == 'host-pc':
            return HostRunner()
        if target == 'armv7m7-imxrt106x':
            return IMXRT106xRunner((serial, DEVICE_SERIAL_USB))
        if target == 'stm32l4':
            return Stm32l4Runner(serial)

        raise ValueError(f"Unknown Runner target: {target}")
