def harness(p):
    p.expect_exact("Unique number sequence")

    unique_numbers = []
    while p.expect(["(\d+) ", "That's all what I got!"]) != 1:
        val = int(p.match.group(0))
        unique_numbers.append(val)

    if len(unique_numbers) != len(set(unique_numbers)):
        return False

    return True
