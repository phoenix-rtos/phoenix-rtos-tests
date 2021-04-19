#
# Makefile for phoenix-rtos-devices
#
# Copyright 2018, 2019 Phoenix Systems
#
# %LICENSE%
#

SIL ?= @
MAKEFLAGS += --no-print-directory

TARGET ?= ia32-generic

include ../phoenix-rtos-build/Makefile.common
include ../phoenix-rtos-build/Makefile.$(TARGET_SUFF)

INSTALL_PROGS :=

.PHONY: clean
clean:
	@echo "rm -rf $(BUILD_DIR)"

ifneq ($(filter clean,$(MAKECMDGOALS)),)
	$(shell rm -rf $(BUILD_DIR))
endif

T1 := $(filter-out clean all install,$(MAKECMDGOALS))
ifneq ($(T1),)
	include $(T1)/Makefile
.PHONY: $(T1)
$(T1): all
else
	include Makefile.$(TARGET_SUFF)
endif

PREFIX_INSTALL = $(PREFIX_ROOTFS)bin/

$(addprefix $(PREFIX_INSTALL), $(INSTALL_PROGS)): $(PREFIX_INSTALL)% : $(PREFIX_PROG_STRIPPED)%
	$(INSTALL_FS)

all: $(addprefix $(PREFIX_PROG), $(INSTALL_PROGS))

install: $(addprefix $(PREFIX_INSTALL), $(INSTALL_PROGS))
