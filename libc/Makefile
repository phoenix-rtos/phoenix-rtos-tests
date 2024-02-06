# Custom name as LOCAL_DIR is auto-reset by binary.mk
MY_LOCAL_DIR := $(call my-dir)
COMMON_SRCS := $(wildcard $(MY_LOCAL_DIR)*.c)

LIBC_UNIT_TESTS :=

# Quick template for adding libc tests from subdir (makes test-libc-xxx binary from xxx/*.c)
# $(eval $(call add_libc_test,NAME[,LOCAL_LDFLAGS][,LOCAL_CFLAGS]))
define add_test_libc
NAME := test-libc-$(1)
LIBC_UNIT_TESTS += test-libc-$(1)
SRCS := $(wildcard $(MY_LOCAL_DIR)$(1)/*.c) $(COMMON_SRCS)
DEP_LIBS := unity
LOCAL_CFLAGS := -I$(MY_LOCAL_DIR) -fno-builtin $(3)
LOCAL_LDFLAGS := -z stack-size=12288 $(2)

include $(binary.mk)
endef

# Add libc test using custom sources from subdir
# $(eval $(call add_libc_test,SUBDIR_NAME,NAME[,LOCAL_LDFLAGS][,LOCAL_CFLAGS], CUSTOM_SRCS))
define add_test_libc_custom
NAME := test-libc-$(2)
LIBC_UNIT_TESTS += test-libc-$(2)
SRCS := $(addprefix $(MY_LOCAL_DIR)$(1)/, $(5)) $(COMMON_SRCS)
DEP_LIBS := unity
LOCAL_CFLAGS := -I$(MY_LOCAL_DIR) $(4)
LOCAL_LDFLAGS := -z stack-size=12288 $(3)

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
$(eval $(call add_test_libc,math, -lm, -ffloat-store)) # -ffloat-store - prevent extended precision on ia32
# -Wno-attribute-warning - ignore warning when using sendmsg/recvmsg, we know it's not fully supported
$(eval $(call add_test_libc_custom,socket,inet-socket,, -Wno-attribute-warning, inet-socket.c common.c))
$(eval $(call add_test_libc_custom,socket,unix-socket,, -Wno-attribute-warning, unix-socket.c common.c))
