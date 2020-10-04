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

$(PREFIX_PROG)test_msg: $(PREFIX_O)proc/test_msg.o
	$(LINK)

$(PREFIX_PROG)test_pthreads: $(PREFIX_O)proc/test_pthreads.o
	$(LINK)

$(PREFIX_PROG)test_env: $(PREFIX_O)proc/test_env.o
	$(LINK)
	
all: $(addprefix $(PREFIX_PROG_STRIPPED), test_threads test_condwait test_msg test_pthreads test_env)
