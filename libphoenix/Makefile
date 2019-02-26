#
# Makefile for libphoenix (test)
#
# Copyright 2017 Phoenix Systems
# Author: Pawel Pisarczyk
#

SIL ?= @
MAKEFLAGS += --no-print-directory --output-sync

TARGET ?= arm-imx6ull
#TARGET ?= armv7-stm32-tiramisu
#TARGET ?= ia32-qemu
#TARGET ?= riscv64

TOPDIR := $(CURDIR)
PREFIX_BUILD ?= ../_build/$(TARGET)
PREFIX_BUILD := $(abspath $(PREFIX_BUILD))
BUILD_DIR ?= $(PREFIX_BUILD)/$(notdir $(TOPDIR))
BUILD_DIR := $(abspath $(BUILD_DIR))

include Makefile.targets

# install tests in separate directory
PREFIX_PROG := $(PREFIX_BUILD)/prog/
PREFIX_PROG_STRIPPED := $(PREFIX_BUILD)/prog.stripped/

SYSROOT := $(shell $(CC) $(CFLAGS) -print-sysroot)
HEADERS_INSTALL_DIR := $(SYSROOT)/usr/include/

ifeq (/,$(SYSROOT))
$(error Sysroot is not supported by Your toolchain. Use cross-toolchain to compile)
endif

CFLAGS += -I"${HEADERS_INSTALL_DIR}"

PROGS := test_mmap test_malloc test_threads test_str2num test_scanf test_msg test_env test_signals test_condwait psh

# include after all dependencies are set
include $(TOPDIR)/Makefile.rules
