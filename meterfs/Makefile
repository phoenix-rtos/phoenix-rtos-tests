#
# Makefile for meterfs test application
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#


NAME := test_meterfs_common
LOCAL_SRCS := common.c
DEPS := unity

TEST_LIBS :=
ifeq ("$(TARGET_FAMILY)-$(TARGET_SUBFAMILY)","host-generic")
  TEST_LIBS += host-flash libmeterfs libphoenix
  LOCAL_SRCS += file_pc.c
else
  LOCAL_SRCS += file_phx.c
endif

include $(static-lib.mk)

define add_meterfs_test
$(call add_test,$(1),$$(TEST_LIBS),unity test_meterfs_common)
endef

$(eval $(call add_meterfs_test, test_meterfs_allocate))
$(eval $(call add_meterfs_test, test_meterfs_openclose))
$(eval $(call add_meterfs_test, test_meterfs_writeread))
$(eval $(call add_meterfs_test, test_meterfs_miscellaneous))
