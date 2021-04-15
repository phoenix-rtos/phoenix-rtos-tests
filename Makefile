#
# Makefile for phoenix-rtos-devices
#
# Copyright 2018, 2019 Phoenix Systems
#
# %LICENSE%
#

SIL ?= @
MAKEFLAGS += --no-print-directory

#TARGET ?= ia32-generic
#TARGET ?= armv7m3-stm32l152xd
#TARGET ?= armv7m3-stm32l152xe
#TARGET ?= armv7m4-stm32l4x6
#TARGET ?= armv7m7-imxrt105x
#TARGET ?= armv7m7-imxrt106x
#TARGET ?= armv7m7-imxrt117x
#TARGET ?= armv7a7-imx6ull
TARGET ?= ia32-generic
#TARGET ?= riscv64-spike

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
$(T1):
	@echo >/dev/null
else
	include unity/Makefile
	include stdio/Makefile
	include proc/Makefile
	include mem/Makefile
	include fs/Makefile
	include disk/Makefile
	#include virtio/Makefile
	include sample/Makefile
	include setjmp/Makefile

endif

PREFIX_INSTALL = $(PREFIX_FS)bin/

$(addprefix $(PREFIX_INSTALL), $(INSTALL_PROGS)): $(PREFIX_INSTALL)% : $(PREFIX_PROG_STRIPPED)%
	$(INSTALL_FS)

all: $(addprefix $(PREFIX_PROG), $(INSTALL_PROGS))

install: $(addprefix $(PREFIX_INSTALL), $(INSTALL_PROGS))
