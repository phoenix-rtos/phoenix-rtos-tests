def harness(p):
    # Expect erase in display ascii escape sequence and prompt sign
    prompt = '\r\x1b[0J' + '(psh)% '
    got = p.read(len(prompt))
    assert got == prompt, f'Expected:\n{prompt}\nGot:\n{got}'
