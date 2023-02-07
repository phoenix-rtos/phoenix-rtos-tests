LOCAL_DIR := $(call my-dir)
NAME := test-libc
SRCS := $(wildcard $(LOCAL_DIR)*.c)
DEP_LIBS := unity

LOCAL_LDFLAGS := -lpthread
LOCAL_LDFLAGS += -z stack-size=12288

include $(binary.mk)
