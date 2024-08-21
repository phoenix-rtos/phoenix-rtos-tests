NAME := libh_abuse_static
LOCAL_SRCS := h_abuse_static.c

LOCAL_INSTALL_PATH := /bin/rtld

LOCAL_LDFLAGS := -Wl,-rpath,${LOCAL_INSTALL_PATH}

DEP_LIBS_SHARED := libh_def_static

include $(shared-lib.mk)
