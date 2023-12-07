#
# Makefile for phoenix-rtos-tests
#
# Copyright 2018, 2019 Phoenix Systems
#
# %LICENSE%
#

include ../phoenix-rtos-build/Makefile.common

.DEFAULT_GOAL := all

# shortcut for unity dependence
define add_unity_test
$(call add_test,$(1),$(2),unity)
endef

# default path for the programs to be installed in rootfs
DEFAULT_INSTALL_PATH := /bin

# read out all components
ALL_MAKES := $(shell find . -mindepth 2 -name Makefile -not -path '*/.*')
include $(ALL_MAKES)

# by default compile all tests, but allow custom values on per-TARGET_FAMILY basys
# TODO: prepare tool which will read YAML files and return the components which should be compiled for a given platform
# for now getting all components and removing tests working only on certain targets
DEFAULT_COMPONENTS := $(filter-out test_meterfs_% test_virtio test-grlib-multi, $(ALL_COMPONENTS))
-include Makefile.$(TARGET_FAMILY)

# create generic targets
.PHONY: all install clean
all: $(DEFAULT_COMPONENTS)
install: $(patsubst %,%-install,$(DEFAULT_COMPONENTS))
clean: $(patsubst %,%-clean,$(ALL_COMPONENTS))
