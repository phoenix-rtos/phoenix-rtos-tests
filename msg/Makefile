NAME := msg-server
LOCAL_SRCS := msg_server.c
LIBS := libphoenix
DEP_LIBS := unity

include $(binary.mk)

NAME := test-msg
LOCAL_SRCS := test_msg.c
LIBS := libphoenix
DEP_LIBS := unity
DEPS := msg-server

include $(binary.mk)
