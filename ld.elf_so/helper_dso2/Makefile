NAME := libh_helper_dso2
LOCAL_SRCS := h_helper_dso2.c

LOCAL_INSTALL_PATH := /bin/rtld

LOCAL_LDFLAGS := -Wl,-rpath,${LOCAL_INSTALL_PATH}

DEP_LIBS_SHARED := libh_helper_dso1

include $(shared-lib.mk)
