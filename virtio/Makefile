#
# Makefile for libphoenix tests
#
# Copyright 2020 Phoenix Systems
#
# %LICENSE%
#

$(PREFIX_PROG)test_virtio: $(PREFIX_O)virtio/test_virtio.o $(PREFIX_A)libvirtio.a
	$(LINK)

all: $(addprefix $(PREFIX_PROG_STRIPPED), test_virtio)
