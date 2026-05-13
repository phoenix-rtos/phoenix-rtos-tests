MY_LOCAL_DIR := $(call my-dir)
COMMON_SRCS := $(wildcard $(MY_LOCAL_DIR)*.c)

POSIX_UNIT_TESTS :=

define add_test_posix
NAME := test-posix-$(1)
POSIX_UNIT_TESTS += test-posix-$(1)
SRCS := $(wildcard $(MY_LOCAL_DIR)$(1)/*.c) $(COMMON_SRCS)
DEP_LIBS := unity
LOCAL_CFLAGS := -I$(MY_LOCAL_DIR) -fno-builtin $(3)
LOCAL_LDFLAGS := -z stack-size=12288 $(2)

include $(binary.mk)
endef

$(eval $(call add_test_posix,sem))
