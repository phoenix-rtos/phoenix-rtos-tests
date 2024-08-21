NAME := libh_helper_dso3
LOCAL_SRCS := h_helper_dso3.cpp

LOCAL_INSTALL_PATH := /bin/rtld
# TODO: why is it needed?
LIBS_SHARED := libstdc++
LOCAL_LDFLAGS := -static-libgcc

include $(shared-lib.mk)

# $(error $(RESOLVED_LIBS_SHARED))