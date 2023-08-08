# Psh tools for writing harness tests

## The psh module description

The python module `psh` makes writing harness tests easier. It provides the following functions:

* `assert_cmd(pexpect_proc, cmd, expected='', result='success', msg='', is_regex=False)` - Sends a specified command and asserts that it's displayed correctly with optional expected output and next prompt. The function also checks if there are no unwanted pieces of information. It's possible to pass a regex in the `expected` argument, but then `is_regex` has to be set True. The regex should be passed as one raw string and match `EOL` after an expected output. Multi-line expected outputs have to be passed in a tuple, with lines in specific items. There is also a possibility to pass a bonus message to print if the assertion fails. In addition the exit code of the passed command is asserted. When result is set to 'success' (default) - expected exit code is 0, otherwise the exit code is not checked (result == 'dont-check') or the function expects exit code different from 0 (result == 'fail'). For a more readable assertion message in case that the test fails, the expected output is printed as regex, but in separate lines. So EOL, which is `r'(\r+)\n'` is replaced by `'\n'`.

* `assert_prompt_after_cmd(pexpect_proc, cmd, result='success', msg=None)` - Sends a command and asserts only `EOL` and next prompt. This method can be useful, when expecting command's output is not needed. Sent command isn't also asserted, so this function can be used for sending some unprintable characters. If it does not fail, the output caught before prompt will be returned. If the `msg` argument is not passed, the default error message will be used. The function also checks the exit code of a passed command in the same way as `assert_cmd()`.

* `assert_prompt(pexpect_proc, msg='', timeout=-1, catch_timeout=True)` - Asserts psh prompt by searching only for `'(psh)% '` in buffer (without checking an escape sequence). There is also a possibility to pass a message to print if the assertion fails and set timeout arguments.

* `assert_prompt_fail(pexpect_proc, msg='', timeout=-1)` - Asserts that there is no psh prompt in the read buffer.

* `assert_exec(pexpect_proc, prog, expected='', msg='')` - Same as `assert_cmd`, but input is selected appropriately for the current target platform (using sysexec or /bin/prog_name). So for example instead of using `assert_cmd('/bin/psh')` or `assert_cmd('sysexec psh')` use `assert_exec(prog='psh')`.

* `get_exit_code(pexpect_proc)` - Returns the exit code of previously sent command.

* `assert_cmd_failed(pexpect_proc)` - Asserts that the exit code of previously sent command is not equal 0.

* `assert_cmd_passed(pexpect_proc)` - Asserts that the exit code of previously sent command is equal 0.

* `init(pexpect_proc)` - Runs psh and next, asserts its first prompt.

* `deinit(pexpect_proc)` - Closes spawned psh and asserts its exit.

The `pexpect_proc` argument is the [pexpect](https://pexpect.readthedocs.io/en/stable/api/index.html) process returned by `pexpect.spawn()` method. Each functional test has the spawned process passed in the argument, which is `p`, please see examples below.

There are also other utilities helpful when writing psh tests, for example `ls` or `date`. Please go to `psh.py` and check them, descriptions are provided in function's doctrings.

## The usage examples

* The error statement assertion
  
  ```python
  import psh.tools.psh as psh

  def harness(p):
      psh.init(p)
      fname = 'nonexistentFile'
      cmd = f'cat {fname}'
      statement = f'cat: {fname} no such file'

      psh.assert_cmd(p, cmd, expected=statement)
  ```


* The multi-line Hello World assertion (assuming that, `helloworld` binary, which prints 'Hello\nWorld\n' is provided)

  ```python
  import psh.tools.psh as psh

  def harness(p):
      psh.init(p)
      expected = ('Hello', 'World')
      psh.assert_exec(p, 'helloworld', expected, "Hello World hasn't been displayed correctly")
  ```