NAME := libh_onlyuse_dynamic
LOCAL_SRCS := h_onlyuse_dynamic.c

LOCAL_INSTALL_PATH := /bin/rtld

LOCAL_LDFLAGS := -Wl,-rpath,${LOCAL_INSTALL_PATH}

DEP_LIBS_SHARED := libh_onlydef

include $(shared-lib.mk)
