def harness(p):
    if 'Constructor 0' not in p.readline():
        return False

    if 'Constructor 1' not in p.readline():
        return False

    if 'Main function' not in p.readline():
        return False

    if 'Destructor 1' not in p.readline():
        return False

    if 'Destructor 0' not in p.readline():
        return False

    return True
