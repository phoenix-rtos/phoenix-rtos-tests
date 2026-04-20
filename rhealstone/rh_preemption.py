from trunner import ctx

def harness(p):
    benchmark_name = "rh_preemption"
    if ctx.target.rootfs:
        p.sendline(f"/bin/{benchmark_name}")
    else:
        p.sendline(f"sysexec {benchmark_name}")
    p.expect_exact("(psh)% ", timeout=60)
    p.sendline("")
