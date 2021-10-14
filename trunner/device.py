# Imports of available runners
from .runners.IMXRT106xRunner import IMXRT106xRunner
from .runners.QemuRunner import QemuRunner
from .runners.HostRunner import HostRunner

from .config import PHRTOS_PROJECT_DIR, DEVICE_SERIAL, DEVICE_SERIAL_USB, rootfs


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
    def create(target):
        if target == 'ia32-generic':
            return QemuRunner(*QEMU_CMD[target])
        if target == 'host-pc':
            return HostRunner()
        if target == 'armv7m7-imxrt106x':
            return IMXRT106xRunner(DEVICE_SERIAL)

        raise ValueError(f"Unknown Runner target: {target}")
