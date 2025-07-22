# Custom name as LOCAL_DIR is auto-reset by binary.mk
MY_LOCAL_DIR := $(call my-dir)

SYS_UNIT_TESTS :=

# Quick template for adding sys tests from subdir (makes test-sys-xxx binary from xxx/*.c)
# $(eval $(call add_sys_test,NAME[,LOCAL_LDFLAGS][,LOCAL_CFLAGS]))
define add_test_sys
NAME := test-sys-$(1)
SYS_UNIT_TESTS += test-sys-$(1)
SRCS := $(wildcard $(MY_LOCAL_DIR)$(1)/*.c)
DEP_LIBS := unity
LOCAL_CFLAGS := -fno-builtin $(3)
LOCAL_LDFLAGS := $(2)

include $(binary.mk)
endef

$(eval $(call add_test_sys,cond))
$(eval $(call add_test_sys,mutex))
$(eval $(call add_test_sys,perf))
