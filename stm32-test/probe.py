from psh.tools.basic import run_psh, assert_only_prompt, assert_prompt

def harness(p):
	p.sendline('ls')
	p.expect_exact("syspage")
	assert_prompt(p)
