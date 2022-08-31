# Tests for libstdc++

## 
```THE WAY OF USAGE
./phoenix-rtos-tests/libstdc++/runtests.sh <target> <path> [test_name] [verbosity]
```

## DESCRIPTION:
To run the test suite for libstdc++, stay in `phoenix-rtos-project` directory and run the script
with at least mandatory arguments (those in angle brackets). The `<target>` parameter is the same as 
a build target for the build script. Currently we can choose between:

- `ia32-generic-qemu`
- (more targets in the future ...)

The `<path>` as to indicate to the toolchain install absolute path. (e.g. ~/toolchains/i386-pc-phoenix
the same path when building toolchain) Typing only those two arguments provide running all tests with 
no verbosity (you only get to know which tests passed or not). In order to run tests which 
you need, pass `[test_name]` argument which can be:

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

The last option is verbosity, responsible for the amount of output which is returned by tests. It has a form e.g. `"-v -v -v"` or `-v`. The more `-v` is in the last argument, the more verbose output is.

## PREREQUISITES:
Tests demand msgfmt and dejagnu to be installed. You can do it typing:
```
sudo apt-get install gettext \
sudo apt-get install dejangu
```
in your console.

## LOG AND SUMMARY:
There are `libstdc++.log` with logs comes from tests and `libstdc++.sum` contains of summary of tests in
```
<path>/gcc-9.3.0/build/<toolchain_target>/libstdc++-v3/testsuite
```
e.g.
```
~/toolchains/i386-pc-phoenix/gcc-9.3.0/build/i386-pc-phoenix/libstdc++-v3/testsuite
```
`<path>` is the same as the argument for this script.`<toolchain_target>` is the appropriate toolchain target
for the passed build target.

## EXAMPLE:
The example for toolchain-target: `i386-pc-phoenix` and build-target `ia32-generic-qemu`
```
./phoenix-rtos-tests/libstdc++/runtests.sh ia32-generic-qemu ~/toolchains/i386-pc-phoenix
```
