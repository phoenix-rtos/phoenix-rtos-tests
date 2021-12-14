# Psh tools for writing harness tests

## The psh module description

The python module `psh` makes writing harness tests easier. It provides the following functions:

* `assert_cmd(pexpect_proc, cmd, expected='', msg='', is_regex=False)` - Sends a specified command and asserts that it's displayed correctly with optional expected output and next prompt. The function also checks if there are no unwanted pieces of information. It's possible to pass a regex in the `expected` argument, but then `is_regex` has to be set True. The regex should be passed as one raw string and match `EOL` after an expected output. Multi-line expected outputs have to be passed in a tuple, with lines in specific items. There is also a possibility to pass an additional message to print if the assertion fails. For a more readable assertion message in case that the test fails, the expected output is printed as regex, but in separate lines. So EOL, which is `r'(\r+)\n'` is replaced by `'\n'`.

* `assert_unprintable(pexpect_proc, cmd, msg='')` - Sends an unprintable command, so the sent message isn't asserted. Asserts only `EOL` and next prompt.

* `assert_only_prompt(pexpect_proc)` - Asserts psh prompt with the appropriate esc sequence.

* `assert_prompt(pexpect_proc, msg='', timeout=-1, catch_timeout=True)` - Asserts psh prompt by searching only for `'(psh)% '` in buffer (without checking an escape sequence). There is also a possibility to pass a message to print if the assertion fails and set timeout arguments.

* `assert_prompt_fail(pexpect_proc, msg='', timeout=-1)` - Assert, that there is no psh prompt in the read buffer.

* `assert_exec(pexpect_proc, prog, expected='', msg='')` - Same as `assert_cmd`, but input is selected appropriately for the current target platform (using sysexec or /bin/prog_name). So for example instead of using `assert_cmd('/bin/psh')` or `assert_cmd('sysexec psh')` use `assert_exec(prog='psh')`.

* `ls(pexpect_proc, dir='')` -  Returns the list with named tuples containing information about files present in the specified directory - `dir`. Named tuple consists of following elements:
  - `name` - name of the file in string format
  - `owner`- owner of the file in string format
  - `is_dir` - True, if the file is directory
  - `date` - Date of last file's edition in the following tuple format: `(month, mday, time)`, for example `('Jan', '01', '00:01')`

* `ls_simple(pexpect_proc, dir='')` - Returns list of file names from the specified directory - `dir`

* `uptime(pexpect_proc)` - Returns tuple with time since start of a system in format: `hh:mm:ss`, for example `['01', '01', '00']`

* `get_commands(pexpect_proc)` - Returns a list of available psh commands.

* `init(pexpect_proc)` - runs psh and next, asserts the first prompt.

The `pexpect_proc` argument is the [pexpect](https://pexpect.readthedocs.io/en/stable/api/index.html) process returned by `pexpect.spawn()` method. Each functional test has the spawned process passed in the argument, which is `p`, please see examples below.

## The usage examples

* The error statement assertion
  
  ```python
  import psh.tools.psh as psh

  def harness(p):
      psh.init(p)
      fname = 'nonexistentFile'

      psh.assert_cmd(p, cmd=f'cat {fname}', expected=f'cat: {fname} no such file')
  ```


* The multi-line Hello World assertion (assuming that, `helloworld` binary, which prints 'Hello\nWorld\n' is provided)

  ```python
  import psh.tools.psh as psh

  def harness(p):
      psh.init(p)
      expected = ('Hello', 'World')
      psh.assert_exec(p, 'helloworld', expected, "Hello World hasn't been displayed correctly")
  ```
