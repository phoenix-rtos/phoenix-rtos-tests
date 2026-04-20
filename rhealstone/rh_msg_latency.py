from trunner import ctx

def harness(p):
    benchmark_name = "rh_msg_latency"
    if ctx.target.rootfs:
        p.sendline(f"/bin/{benchmark_name}")
    else:
        p.sendline(f"sysexec {benchmark_name}")
    p.expect_exact("(psh)% ")
    p.sendline("")
