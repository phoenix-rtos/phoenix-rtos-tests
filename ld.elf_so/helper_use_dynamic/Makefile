NAME := libh_use_dynamic
LOCAL_SRCS := h_use_dynamic.c

LOCAL_INSTALL_PATH := /bin/rtld

LOCAL_LDFLAGS := -Wl,-rpath,${LOCAL_INSTALL_PATH}

DEP_LIBS_SHARED := libh_def_dynamic

include $(shared-lib.mk)
