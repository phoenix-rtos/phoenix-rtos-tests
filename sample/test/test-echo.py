def harness(p):
    p.expect_exact("[Start test-1 example]")

    p.send("Hello world!")
    p.expect_exact("Hello world!")

    p.send("E")
    p.send("C")
    p.send("H")
    p.send("O")

    p.expect_exact("ECHO")

    p.sendline("")
    p.expect_exact("[Success!]")
