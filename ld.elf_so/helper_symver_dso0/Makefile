NAME := libh_helper_symver_dso0
LOCAL_SRCS := h_helper_symver_dso0.c

LOCAL_INSTALL_PATH := /bin/rtld/h_helper_symver_dso0

SONAME := libh_helper_symver_dso.so


# In phoenix building system it is impossible to create multiple libs with the same name
# This is a workaround manually moving the lib to the required path.
ALL_COMPONENTS += libh_helper_symver_dso_0

libh_helper_symver_dso_0: $(NAME)

libh_helper_symver_dso_0-install: NAME := $(NAME)
libh_helper_symver_dso_0-install: LOCAL_INSTALL_PATH := $(LOCAL_INSTALL_PATH)

libh_helper_symver_dso_0-clean: LOCAL_INSTALL_PATH := $(LOCAL_INSTALL_PATH)

libh_helper_symver_dso_0-install: $(NAME)-install
	$(SIL)ln -f $(PREFIX_ROOTFS)$(LOCAL_INSTALL_PATH)/$(NAME).so $(PREFIX_ROOTFS)$(LOCAL_INSTALL_PATH)/libh_helper_symver_dso.so

libh_helper_symver_dso_0-clean: $(NAME)-clean
	$(SIL)rm $(PREFIX_ROOTFS)$(LOCAL_INSTALL_PATH)/libh_helper_symver_dso.so


include $(shared-lib.mk)
