TESTSDIR := /bin
SHARED_LIBS_DIR := /bin/rtld
CPPFLAGS += -D_RTLD_TEST_SRCDIR=\"$(TESTSDIR)\"
CPPFLAGS += -D_RTLD_TEST_SHARED_LIBS_DIR=\"$(SHARED_LIBS_DIR)\"


# t_df_1_noopen
NAME := h_df_1_noopen1
LOCAL_SRCS := h_df_1_noopen.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

NAME := h_df_1_noopen2
LOCAL_SRCS := h_df_1_noopen.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
DEP_LIBS_SHARED := libh_helper_ifunc_dso
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

NAME := t_df_1_noopen
LOCAL_SRCS := t_df_1_noopen.c
DEP_LIBS := unity libexecassert
DEPS := h_df_1_noopen1 h_df_1_noopen2
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

# t_dl_symver
NAME := h_dl_symver_v0
LOCAL_SRCS := h_dl_symver.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)/h_helper_symver_dso0
DEP_LIBS_SHARED := libh_helper_symver_dso0
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

NAME := h_dl_symver_v1
LOCAL_SRCS := h_dl_symver.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)/h_helper_symver_dso1
DEP_LIBS_SHARED := libh_helper_symver_dso1
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

NAME := h_dl_symver_v2
LOCAL_SRCS := h_dl_symver.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)/h_helper_symver_dso2
DEP_LIBS_SHARED := libh_helper_symver_dso2
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

NAME := t_dl_symver
LOCAL_SRCS := t_dl_symver.c
DEP_LIBS := unity libexecassert
DEPS := h_dl_symver_v0 h_dl_symver_v1 h_dl_symver_v2
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)


# t_dlerror-cleared
NAME := t_dlerror-cleared
LOCAL_SRCS := t_dlerror-cleared.c
DEP_LIBS := unity
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

# t_dlerror-false
NAME := t_dlerror-false
LOCAL_SRCS := t_dlerror-false.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,/var/nonexistent/lib
DEP_LIBS := unity
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

# t_dlinfo
NAME := t_dlinfo
LOCAL_SRCS := t_dlinfo.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
DEPS := libh_helper_ifunc_dso
DEP_LIBS := unity
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

# t_dlvsym
NAME := t_dlvsym
LOCAL_SRCS := t_dlvsym.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)/h_helper_symver_dso2
DEPS := libh_helper_symver_dso2
DEP_LIBS := unity
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

# t_hash
NAME := t_hash
LOCAL_SRCS := t_hash.c
LIBS_SHARED := librtld_stubs
DEP_LIBS := unity
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
# Link with hashing implementation from ld.elf_so
LOCAL_CFLAGS := -I../phoenix-rtos-utils/ld.elf_so
SRCS := ../phoenix-rtos-utils/ld.elf_so/hash.c
include $(binary-dyn.mk)

# t_ifunc
NAME := h_ifunc
LOCAL_SRCS := h_ifunc.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
DEP_LIBS_SHARED := libh_helper_ifunc_dso
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)


define ifunc_test
NAME := $(1)
LOCAL_SRCS := t_ifunc.c
DEP_LIBS := unity libexecassert
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
LOCAL_LDFLAGS += $(2)
DEPS := h_ifunc
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)
endef

# Workaround to pass a single argument.
T_IFUNC_LDFLAGS := $(LDFLAGS_PREFIX)-z,relro
T_IFUNC_NOW_LDFLAGS := $(LDFLAGS_PREFIX)-z,relro $(LDFLAGS_PREFIX)-z,now
T_IFUNC_NORELRO_LDFLAGS := $(LDFLAGS_PREFIX)-z,norelro
T_IFUNC_NORELRO_NOW_LDFLAGS :=  $(LDFLAGS_PREFIX)-z,norelro $(LDFLAGS_PREFIX)-z,now

$(eval $(call ifunc_test, t_ifunc, $(T_IFUNC_LDFLAGS)))
$(eval $(call ifunc_test, t_ifunc_now, $(T_IFUNC_NOW_LDFLAGS)))
$(eval $(call ifunc_test, t_ifunc_norelro, $(T_IFUNC_NORELRO_LDFLAGS)))
$(eval $(call ifunc_test, t_ifunc_norelro_now, $(T_IFUNC_NORELRO_NOW_LDFLAGS)))

# t_rtld_r_debug
NAME := t_rtld_r_debug
LOCAL_SRCS := t_rtld_r_debug.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
LIBS_SHARED := librtld_stubs
DEPS := libh_helper_ifunc_dso
DEP_LIBS := unity
include $(binary-dyn.mk)

# t_thread_local_dtor
NAME := h_thread_local_dtor
LOCAL_SRCS := h_thread_local_dtor.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
LIBS_SHARED := libpthread librtld_stubs
DEPS := libh_helper_dso3
include $(binary-dyn.mk)

NAME := t_thread_local_dtor
LOCAL_SRCS := t_thread_local_dtor.c
DEP_LIBS := unity libexecassert
DEPS := h_thread_local_dtor
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

# t_tls_extern
NAME := t_tls_extern
LOCAL_SRCS := t_tls_extern.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
DEP_LIBS := unity
DEPS := libh_def_dynamic libh_use_dynamic libh_onlyuse_static libh_onlyuse_dynamic libh_onlydef libh_def_static libh_use_static libh_abuse_static libh_abuse_dynamic
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)

# t_tls_extern-nightly
NAME := t_tls_extern-nightly
LOCAL_SRCS := t_tls_extern-nightly.c
LOCAL_LDFLAGS := $(LDFLAGS_PREFIX)-rpath,$(SHARED_LIBS_DIR)
DEP_LIBS := unity
DEPS := libh_def_dynamic libh_use_dynamic
LIBS_SHARED := librtld_stubs
include $(binary-dyn.mk)
