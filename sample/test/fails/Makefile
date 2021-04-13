HARNESS_PROGS := test-fail-output-1 test-fail-output-2 test-fail-exception

UNITY_PROGS := test-fail-unity test-fail-unity-exc

CURR_DIR := sample/test/fails/

INSTALL_PROGS += $(HARNESS_PROGS) $(UNITY_PROGS)

$(addprefix $(PREFIX_PROG), $(HARNESS_PROGS)): $(PREFIX_PROG)%: $(PREFIX_O)$(CURR_DIR)%.o
	$(LINK)

$(addprefix $(PREFIX_PROG), $(UNITY_PROGS)): $(PREFIX_PROG)%: $(PREFIX_O)$(CURR_DIR)%.o $(PREFIX_A)unity.a
	$(LINK)
