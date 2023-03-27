TEST_FILES := $(wildcard ${PREFIX_PROJECT}/phoenix-rtos-tests/simsensor/files/*)
FILES_DIR := ${PREFIX_ROOTFS}usr/test/simsensor

${shell mkdir -p ${FILES_DIR}}
${shell cp -r ${TEST_FILES} ${FILES_DIR}}

NAME := simsensor_tests
LOCAL_SRCS := main.c queue.c reader.c

LIBS := libsimsensor
DEP_LIBS := unity

LOCAL_CFLAGS := -g
LOCAL_LDFLAGS := -g

include $(binary.mk)
