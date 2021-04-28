def harness(p):
    scenario = [
        'Constructor 0',
        'Constructor 1',
        'Main function',
        'Destructor 1',
        'Destructor 0'
    ]

    for expect in scenario:
        line = p.readline().rstrip()
        assert expect == line, f'Expected: {expect}, got: {line}'
