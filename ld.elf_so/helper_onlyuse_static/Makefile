NAME := libh_onlyuse_static
LOCAL_SRCS := h_onlyuse_static.c

LOCAL_INSTALL_PATH := /bin/rtld

LOCAL_LDFLAGS := -Wl,-rpath,${LOCAL_INSTALL_PATH}

DEP_LIBS_SHARED := libh_onlydef

include $(shared-lib.mk)
