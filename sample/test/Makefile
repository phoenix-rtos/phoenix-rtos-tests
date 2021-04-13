include sample/test/fails/Makefile

HARNESS_PROGS := test-echo

UNITY_PROGS := test-unity

CURR_DIR := sample/test/

INSTALL_PROGS += $(HARNESS_PROGS) $(UNITY_PROGS)

$(addprefix $(PREFIX_PROG), $(HARNESS_PROGS)): $(PREFIX_PROG)%: $(PREFIX_O)$(CURR_DIR)%.o
	$(LINK)

$(addprefix $(PREFIX_PROG), $(UNITY_PROGS)): $(PREFIX_PROG)%: $(PREFIX_O)$(CURR_DIR)%.o $(PREFIX_A)unity.a
	$(LINK)
