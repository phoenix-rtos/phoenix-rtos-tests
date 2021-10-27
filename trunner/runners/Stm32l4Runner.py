#
# Phoenix-RTOS test runner
#
# Stm32l4 runner
#
# Copyright 2021 Phoenix SYstems
# Authors: Mateusz Niewiadomski
#
import subprocess
import serial
import time
import pexpect
import pexpect.fdpexpect

from .common import DeviceRunner


class Stm32l4Runner(DeviceRunner):

	class oneByOne_fdspawn(pexpect.fdpexpect.fdspawn):

		def send(self, s):
			for c in s:
				super().send(c)
				time.sleep(0.03)


		def sendline(self, s):
			self.send(s)
			super().send('\n')


	def __init__(self, port1, port2 = None):
		super().__init__(port1)
		self.port2 = port2
		self.image_path = '_boot/phoenix-armv7m4-stm32l4x6.bin'
		self.image_flashed = False


	def flash(self):
		cmd_program = '-c "program ' + self.image_path + ' 0x08000000 verify reset exit"'
		
		cmd = 'openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "reset_config srst_only srst_nogate connect_assert_srst" ' +  cmd_program 
		openocd_process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		stdout, stderr = openocd_process.communicate()
		openocd_process.wait()
		# TODO - check flashing status


	def run(self, test):
		if test.skipped():
			return

		if not test.is_type('harness'):
			test.skip()
			return

		try:
			self.serial = serial.Serial(self.port, baudrate=115200)
		except serial.SerialException:
			test.handle_exception()
			return

		proc = self.oneByOne_fdspawn(self.serial, encoding='utf-8', timeout=test.timeout)

		try:
			test.handle(proc)
		finally:
			self.serial.close()
			