NAME := libh_abuse_dynamic
LOCAL_SRCS := h_abuse_dynamic.c

LOCAL_INSTALL_PATH := /bin/rtld

LOCAL_LDFLAGS := -Wl,-rpath,${LOCAL_INSTALL_PATH}

DEP_LIBS_SHARED := libh_def_dynamic

include $(shared-lib.mk)
