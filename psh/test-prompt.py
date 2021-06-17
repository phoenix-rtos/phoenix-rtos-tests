from psh.tools import run_psh, assert_only_prompt


def harness(p):
    run_psh(p)
    assert_only_prompt(p)
