$(PREFIX_PROG)sample_echo: $(PREFIX_O)sample/test/test-echo.o
	$(LINK)

$(PREFIX_PROG)sample_unity: $(PREFIX_O)sample/test/test-unity.o $(PREFIX_A)unity.a
	$(LINK)

all: $(addprefix $(PREFIX_PROG_STRIPPED), sample_echo sample_unity)

