from trunner.dut import Dut

# The "main" function for test is the harness. Every harness must define a function with this name.
def harness(dut: Dut): # Dut wraps pexpect object, but you should use it as a normal pexpect spawn object.
    expected = "Hello, world!"
    line = dut.readline().rstrip() # Read the line that has been printed by a device.
    assert line == expected, f"Line mismatch! Expected: {expected}, got: {line}" # Assure that the read line is `Hello, world!` message.
