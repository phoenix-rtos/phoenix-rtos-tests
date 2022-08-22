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
from .wrappers import Psu, Gdb, Phoenixd, PhoenixdError, GdbError, append_output


class Flasher(ABC):
    """Common interface for flashing the system image"""

    def __init__(self, runner):
        self.runner = runner

    def flash(self):
        phd = None
        try:
            self.reboot(flashing_mode=True, cut_power=self.runner.is_cut_power_used)
            with PloTalker(self.runner.serial_port) as plo:
                if self.runner.logpath:
                    plo.plo.logfile = open(self.runner.logpath, "a")

                self.upload_plo(plo)

                with Phoenixd(self.runner.target, self.runner.phoenixd_port, long_flashing=self.long_flashing) as phd:
                    plo.copy_file2mem(
                        src='usb0',
                        file=self.runner.IMAGE,
                        dst=f'flash{self.runner.flash_memory}',
                    )

            self.reboot(flashing_mode=False, cut_power=self.runner.is_cut_power_used)
        except (TIMEOUT, EOF, PloError, PhoenixdError, GdbError, RebootError) as exc:
            exception = f'{exc}\n'

            if phd and not isinstance(exc, RebootError):
                exception = append_output('phoenixd', exception, phd.output())

            logging.info(exception)
            sys.exit(1)

    @abstractmethod
    def reboot(self):
        """Method used for setting the intended boot mode and restarting/resseting the board."""
        pass

    @abstractmethod
    def upload_plo(self):
        """Method used for flashing a device with the image containing tests."""
        pass


class ZYNQ7000JtagFlasher(Flasher):
    """ Class intended for flashing the system image to Zynq7000 platform using jtag and plo"""

    PLO_FILE = 'plo-gdb.elf'
    GDB_SCRIPT = '/opt/phoenix/scripts/gdb_load_file'

    def __init__(self, runner):
        self.long_flashing = True
        super().__init__(runner)

    def reboot(self, flashing_mode, cut_power):
        if flashing_mode:
            self.runner.reboot(jtag_mode=True)
        else:
            self.runner.reboot()

    def upload_plo(self, plo):
        Gdb(self.runner.target, self.PLO_FILE, script=self.GDB_SCRIPT).run()

        plo.interrupt_counting()
        plo.wait_prompt()


class NXPSerialFlasher(Flasher):
    """ Class intended for flashing the system image to NXP platforms using psu and plo"""

    SDP = 'plo-ram.sdp'

    def __init__(self, runner):
        self.long_flashing = False
        super().__init__(runner)

    def reboot(self, flashing_mode, cut_power):
        if flashing_mode:
            self.runner.reboot(serial_downloader=True, cut_power=cut_power)
        else:
            self.runner.reboot(cut_power=cut_power)

    def upload_plo(self, plo):
        Psu(self.runner.target, script=self.SDP).run()
        plo.wait_prompt()
