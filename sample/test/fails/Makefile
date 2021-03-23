$(PREFIX_PROG)fail_output_1: $(PREFIX_O)sample/test/fails/test-fail-output-1.o
	$(LINK)

$(PREFIX_PROG)fail_output_2: $(PREFIX_O)sample/test/fails/test-fail-output-2.o
	$(LINK)

$(PREFIX_PROG)fail_exception: $(PREFIX_O)sample/test/fails/test-fail-exception.o
	$(LINK)

$(PREFIX_PROG)fail_unity: $(PREFIX_O)sample/test/fails/test-fail-unity.o $(PREFIX_A)unity.a
	$(LINK)

$(PREFIX_PROG)fail_unity_exception: $(PREFIX_O)sample/test/fails/test-fail-unity-exc.o $(PREFIX_A)unity.a
	$(LINK)

all: $(addprefix $(PREFIX_PROG_STRIPPED), fail_output_1 fail_output_2 fail_unity fail_exception \
					fail_unity_exception)

