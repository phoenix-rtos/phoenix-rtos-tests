#
# Phoenix-RTOS test runner
#
# Flasher classes for device runners
#
# Copyright 2021, 2022 Phoenix Systems
# Authors: Jakub Sarzyński, Damian Loewnau
#

from abc import ABC, abstractmethod
import logging
import sys

from pexpect.exceptions import TIMEOUT, EOF
from .common import PloError, PloTalker, RebootError
from .utils import Psu, Gdb, Phoenixd, PhoenixdError, GdbError, append_output
from trunner.config import PHRTOS_PROJECT_DIR


class Flasher(ABC):
    """Common interface for flashing the system image"""

    def __init__(self, target, serial_port, phoenixd_port, flash_bank, logpath, copy_timeout):
        self.target = target
        self.serial_port = serial_port
        self.phoenixd_port = phoenixd_port
        self.flash_bank = flash_bank
        self.logpath = logpath
        self.copy_timeout = copy_timeout

    def flash(self, runner_reboot_fn):
        phd = None
        try:
            runner_reboot_fn(flashing_mode=True)
            with PloTalker(self.serial_port) as plo:
                if self.logpath:
                    plo.plo.logfile = open(self.logpath, "a")

                self.upload_plo(plo)
                self.erase(plo)

                with Phoenixd(self.target, self.phoenixd_port) as phd:
                    plo.copy_file2mem(
                        src='usb0',
                        file='phoenix.disk',
                        dst=f'flash{self.flash_bank}',
                        timeout=self.copy_timeout
                    )

            runner_reboot_fn(flashing_mode=False)
        except (TIMEOUT, EOF, PloError, PhoenixdError, GdbError, RebootError) as exc:
            exception = f'{exc}\n'

            if phd and not isinstance(exc, RebootError):
                exception = append_output('phoenixd', exception, phd.output())

            logging.info(exception)
            sys.exit(1)

    @abstractmethod
    def erase(self):
        """Method used for erasing target flash storage to prepare for copying an image """
        pass

    @abstractmethod
    def upload_plo(self):
        """Method used for flashing a device with the image containing tests."""
        pass


class ZYNQ7000JtagFlasher(Flasher):
    """ Class intended for flashing the system image to Zynq7000 platform using jtag and plo"""

    PLO_FILE = 'plo-gdb.elf'
    GDB_SCRIPT = f'{PHRTOS_PROJECT_DIR}/phoenix-rtos-build/scripts/upload-zynq7000.gdb'

    def __init__(self, target, serial_port, phoenixd_port, flash_bank, logpath):
        super().__init__(target, serial_port, phoenixd_port, flash_bank, logpath, copy_timeout=140)

    def erase(self, plo):
        plo.erase(
            device=f'flash{self.flash_bank}',
            offset=0x800000,
            size=0x1000000  # The size of rootfs on zynq7000-zedboard target equals 16 MB
        )

    def upload_plo(self, plo):
        Gdb(self.target, self.PLO_FILE, script=self.GDB_SCRIPT).run()

        plo.interrupt_counting()
        plo.wait_prompt()


class NXPSerialFlasher(Flasher):
    """ Class intended for flashing the system image to NXP platforms using psu and plo"""

    SDP = 'plo-ram.sdp'

    def __init__(self, target, serial_port, phoenixd_port, flash_bank, logpath):
        super().__init__(target, serial_port, phoenixd_port, flash_bank, logpath, copy_timeout=60)

    def erase(self, plo):
        # do not erase until all the nxp targets supported by runner are non-rootfs
        pass

    def upload_plo(self, plo):
        Psu(self.target, script=self.SDP).run()
        plo.wait_prompt()
