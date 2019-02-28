#
# Makefile for libphoenix tests
#
# Copyright 2019 Phoenix Systems
#
# %LICENSE%
#

TEST_PROGS := test_condwait test_env test_malloc test_mmap test_msg test_scanf test_signals test_str2num test_threads

$(PREFIX_PROG)%: $(PREFIX_O)%.o
	$(LINK)

all: $(addprefix $(PREFIX_PROG_STRIPPED), $(TEST_PROGS))

# Prevent removing intermediate files
.PRECIOUS: $(PREFIX_O)%.o $(PREFIX_PROG)%
