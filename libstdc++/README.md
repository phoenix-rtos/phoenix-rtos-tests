# Tests for libstdc++

## Prerequisites

Tests demand `msgfmt` and `dejagnu` to be installed. You can do it by typing:

```bash
sudo apt-get install gettext dejagnu
```

in your console.

## The way of usage

To execute tests,  use the command below. Remember that you need to have built the desired target first.

```bash
./phoenix-rtos-tests/libstdc++/runtests.sh <target> <path> [test_name] [verbosity]
```

## Description

To run the test suite for libstdc++, stay in `phoenix-rtos-project` directory and run the script
with at least mandatory arguments (those in angle brackets). The `<target>` parameter is the same as
a build target for the build script. Currently, we can choose between:

- `ia32-generic-qemu`
- (more targets in the future ...)

The `<path>` indicates the toolchain install absolute path. (e.g. `~/toolchains/i386-pc-phoenix/_build`
the same path when building toolchain) Typing only those two arguments provides running all tests with
no verbosity (you only get to know which tests passed or not). In order to run tests you need, to
pass `[test_name]` argument which can be:

<details>
<summary> Available tests </summary>

- 17_intro,
- 18_support,
- 19_diagnostics,
- 20_util,
- 21_strings,
- 22_locale,
- 23_containers,
- 24_iterators,
- 25_algorithms,
- 26_numerics,
- 27_io,
- 28_regex,
- 29_atomics,
- 30_threads

</details>

The last option is verbosity, responsible for the amount of output that is returned by tests. It has a form e.g. `"-v -v -v"` or `-v`.
The more `-v` is in the last argument, the more verbose the output is.

## Log and summary

There is `libstdc++.log` with logs that come from tests and `libstdc++.sum` contains of summary of tests in

```bash
<path>/gcc-9.5.0/build/<toolchain_target>/libstdc++-v3/testsuite
```

e.g.

```bash
~/toolchains/i386-pc-phoenix/gcc-9.5.0/build/i386-pc-phoenix/libstdc++-v3/testsuite
```

`<path>` is the same as the argument for this script.`<toolchain_target>` is the appropriate toolchain target
for the passed build target.

## Example

The example for toolchain-target: `i386-pc-phoenix` and build-target `ia32-generic-qemu`

```bash
./phoenix-rtos-tests/libstdc++/runtests.sh ia32-generic-qemu ~/toolchains/i386-pc-phoenix/_build
```
