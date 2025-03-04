MY_LOCAL_DIR := $(call my-dir)

# libgraph in test version without real libvga, libvrtio dependencies
NAME := libgraph.test

LOCAL_HEADERS := ../../phoenix-rtos-corelibs/libgraph/graph.h
LOCAL_SRCS := ../../phoenix-rtos-corelibs/libgraph/graph.c

# mocked sys/threads.h, sys/types.h
LOCAL_CFLAGS := -I$(MY_LOCAL_DIR)

include $(static-lib.mk)

NAME := test-libgraph
LOCAL_SRCS := main.c mock_threads.c
DEP_LIBS := unity libgraph.test
LOCAL_CFLAGS := -I$(MY_LOCAL_DIR) -I$(MY_LOCAL_DIR)/../../phoenix-rtos-corelibs/libgraph/

include $(binary.mk)