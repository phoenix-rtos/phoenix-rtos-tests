#
# Makefile for POSIX tests
#
# Copyright 2026 Phoenix Systems
#
# %LICENSE%
#
MY_LOCAL_DIR := $(call my-dir)

POSIX_UNIT_TESTS :=

define add_test_posix
NAME := test-posix-$(1)
POSIX_UNIT_TESTS += test-posix-$(1)
SRCS := $(wildcard $(MY_LOCAL_DIR)$(1)/*.c)
DEP_LIBS := unity
LOCAL_CFLAGS := -I$(MY_LOCAL_DIR)

include $(binary.mk)
endef

$(eval $(call add_test_posix,dirent))
