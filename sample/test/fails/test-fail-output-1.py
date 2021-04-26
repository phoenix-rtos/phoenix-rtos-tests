def harness(p):
    p.expect_exact("[Test FAIL TIMEOUT started]")
    # We expect "1", should trigger timeout
    p.expect_exact("2")
