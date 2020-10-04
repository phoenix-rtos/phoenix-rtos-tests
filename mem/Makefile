#
# Makefile for libphoenix tests
#
# Copyright 2019 Phoenix Systems
#
# %LICENSE%
#

$(PREFIX_PROG)test_mmap: $(PREFIX_O)mem/test_mmap.o
	$(LINK)

$(PREFIX_PROG)test_malloc: $(PREFIX_O)mem/test_malloc.o
	$(LINK)

$(PREFIX_PROG)test_memmove: $(PREFIX_O)mem/test_memmove.o
	$(LINK)
	
all: $(addprefix $(PREFIX_PROG_STRIPPED), test_mmap test_malloc test_memmove)

