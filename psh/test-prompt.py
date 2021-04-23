def harness(p):
    # Expect erase in display ascii escape sequence and prompt sign
    expect = '\r\x1b[0J' + '(psh)% '
    got = p.read(len(expect))

    if got != expect:
        print(f'Expected:\n\t{expect}\nGot:\n\t{got}')
        return False

    return True
