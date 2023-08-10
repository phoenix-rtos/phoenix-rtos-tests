# custom name as LOCAL_DIR is auto-reset by binary.mk
MY_LOCAL_DIR := $(call my-dir)
COMMON_SRCS := $(wildcard $(MY_LOCAL_DIR)*.c)

LIBC_UNIT_TESTS :=

# quick template for adding libc tests from subdirs (makes test-libc-xxx binary from xxx/*.c)
# $(eval $(call add_libc_test,NAME[,LOCAL_LDFLAGS]))
define add_test_libc
NAME := test-libc-$(1)
LIBC_UNIT_TESTS += test-libc-$(1)
SRCS := $(wildcard $(MY_LOCAL_DIR)$(1)/*.c) $(COMMON_SRCS)
DEP_LIBS := unity
LOCAL_CFLAGS := -I$(MY_LOCAL_DIR) $(3)
LOCAL_LDFLAGS := -z stack-size=12288 $(2)

include $(binary.mk)
endef

$(eval $(call add_test_libc,printf))
$(eval $(call add_test_libc,scanf))
$(eval $(call add_test_libc,pthread, -lpthread))
$(eval $(call add_test_libc,misc))
$(eval $(call add_test_libc,stdio))
$(eval $(call add_test_libc,stdlib))
$(eval $(call add_test_libc,string))
$(eval $(call add_test_libc,exit, -lpthread))
$(eval $(call add_test_libc,time))
# -fno-bultin - prevent compiler to replace math funcs with compiler builtins
# -ffloat-store - prevent extended precision on ia32
$(eval $(call add_test_libc,math, -lm, -fno-builtin -ffloat-store))
