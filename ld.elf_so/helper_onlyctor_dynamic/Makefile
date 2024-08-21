NAME := libh_onlyctor_dynamic
LOCAL_SRCS := h_onlyctor_dynamic.c

LOCAL_INSTALL_PATH := /bin/rtld

LOCAL_LDFLAGS := -Wl,-rpath,${LOCAL_INSTALL_PATH}

DEP_LIBS_SHARED := libh_onlydef

include $(shared-lib.mk)
