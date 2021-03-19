#
# Makefile for meterfs test application
#
# Copyright 2021 Phoenix Systems
#
# %LICENSE%
#

LIBS := unity
OBJS_COMMON := common

ifeq ("$(TARGET)","host-pc")
LIBS += host-flash libmeterfs libphoenix
OBJS_COMMON += file_pc
else
OBJS_COMMON += file_phx
endif

OBJS_COMMON := $(patsubst %, $(PREFIX_O)meterfs/%.o, $(OBJS_COMMON))
LIBS := $(patsubst %, $(PREFIX_A)%.a, $(LIBS))

$(PREFIX_PROG)%: $(PREFIX_O)meterfs/%.o $(OBJS_COMMON) $(LIBS)
	$(LINK)

INSTALL_PROGS += test_meterfs_allocate test_meterfs_openclose test_meterfs_writeread test_meterfs_miscellaneous
