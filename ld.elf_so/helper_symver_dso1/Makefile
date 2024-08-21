NAME := libh_helper_symver_dso1
LOCAL_SRCS := h_helper_symver_dso1.c

LOCAL_INSTALL_PATH := /bin/rtld/h_helper_symver_dso1

SONAME := libh_helper_symver_dso.so
LOCAL_VERSION_SCRIPT := h_helper_symver_dso1.map


# In phoenix building system it is impossible to create multiple libs with the same name
# This is a workaround manually moving the lib to the required path.
ALL_COMPONENTS += libh_helper_symver_dso_1

libh_helper_symver_dso_1: $(NAME)

libh_helper_symver_dso_1-install: NAME := $(NAME)
libh_helper_symver_dso_1-install: LOCAL_INSTALL_PATH := $(LOCAL_INSTALL_PATH)

libh_helper_symver_dso_1-clean: LOCAL_INSTALL_PATH := $(LOCAL_INSTALL_PATH)

libh_helper_symver_dso_1-install: $(NAME)-install
	$(SIL)ln -f $(PREFIX_ROOTFS)$(LOCAL_INSTALL_PATH)/$(NAME).so $(PREFIX_ROOTFS)$(LOCAL_INSTALL_PATH)/libh_helper_symver_dso.so

libh_helper_symver_dso_1-clean: $(NAME)-clean
	$(SIL)rm $(PREFIX_ROOTFS)$(LOCAL_INSTALL_PATH)/libh_helper_symver_dso.so


include $(shared-lib.mk)
