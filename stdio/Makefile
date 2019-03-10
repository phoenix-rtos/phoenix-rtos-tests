#
# Makefile for libphoenix tests
#
# Copyright 2019 Phoenix Systems
#
# %LICENSE%
#

$(PREFIX_PROG)test_scanf: $(PREFIX_O)stdio/test_scanf.o
	$(LINK)

$(PREFIX_PROG)test_str2num: $(PREFIX_O)stdio/test_str2num.o
	$(LINK)
	
all: $(addprefix $(PREFIX_PROG_STRIPPED), test_scanf test_str2num)
