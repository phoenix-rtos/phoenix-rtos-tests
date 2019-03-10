#
# Makefile for libphoenix tests
#
# Copyright 2019 Phoenix Systems
#
# %LICENSE%
#

$(PREFIX_PROG)test_threads: $(PREFIX_O)proc/test_threads.o
	$(LINK)

$(PREFIX_PROG)test_condwait: $(PREFIX_O)proc/test_condwait.o
	$(LINK)
	
all: $(addprefix $(PREFIX_PROG_STRIPPED), test_threads test_condwait)
