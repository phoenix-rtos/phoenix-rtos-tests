def harness(proc):
    proc.expect_exact("[Test fail-exception started]")
    # Will trigger timeout error
    proc.expect_exact("[Succeeded!]")
