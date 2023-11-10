#
# Makefile for mem tests
#
# Copyright 2019 Phoenix Systems
#
# %LICENSE%
#

# LEGACY 
$(eval $(call add_test, test_mmap))
$(eval $(call add_test, test_malloc))

# UNITY

NAME := test-mprotect
LOCAL_SRCS := test_mprotect.c
DEP_LIBS := unity

include $(binary.mk)

NAME := test-mprotect-fault
LOCAL_SRCS := test_mprotect_fault.c
DEP_LIBS := unity

include $(binary.mk)
