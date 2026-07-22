#!/usr/bin/env python3
#
# Phoenix-RTOS
#
# Generate a C header with HAS_<FUNC> feature macros from a compiled libc.
#
# The script inspects compiled output (static archives *.a and object files
# *.o) with `nm`, collects the *defined* (exported) function symbols that match
# one or more name prefixes and emits, for every one of them, a line:
#
#     #define HAS_<UPPER_SYMBOL> 1
#
# Tests can then gate optional functionality on the presence of a symbol
# instead of hard-coding a per-platform `#ifdef __phoenix__`:
#
#     #include "libc_features.h"
#
#     #ifndef HAS_PTHREAD_SETCANCELTYPE
#         TEST_IGNORE_MESSAGE("pthread_setcanceltype is not implemented");
#     #else
#         ... real test body ...
#     #endif
#
# Because detection is based on the symbol table of the built library, a test
# is automatically enabled as soon as libc starts to provide the function - no
# manual editing of the test sources is required.
#
# NOTE: A symbol that is present but implemented as a stub returning ENOSYS /
# ENOTSUP (e.g. the current pthread_rwlock_* family) is reported as available,
# because from the linker's point of view it exists. Such runtime limitations
# have to be handled at runtime by the test itself.
#
# Copyright 2026 Phoenix Systems
# Author: Phoenix-RTOS test tooling
#
# This file is part of Phoenix-RTOS.
#
# SPDX-License-Identifier: BSD-3-Clause

import argparse
import os
import re
import subprocess
import sys

# nm symbol types that denote a defined (exported) function/text symbol.
# T/t - text (global/local), W/w - weak, V/v - weak object, i - indirect.
_DEFINED_TYPES = set("TtWwVvi")

# Matches an `nm` symbol line: optional value, single-letter type, name.
_NM_LINE = re.compile(r"^\s*(?:[0-9a-fA-F]+)?\s+([A-Za-z])\s+(\S+)\s*$")

# A valid C identifier - used to reject compiler-generated symbol clones such as
# `foo.part.0`, `foo.constprop.1` or `foo.isra.0` that cannot become macros.
_C_IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*\Z")


def collect_input_files(paths):
    """Expand the given paths into a sorted list of *.a / *.o files."""
    files = []
    for path in paths:
        if os.path.isdir(path):
            for root, _dirs, names in os.walk(path):
                for name in names:
                    if name.endswith((".a", ".o")):
                        files.append(os.path.join(root, name))
        elif os.path.isfile(path):
            files.append(path)
        else:
            print("warning: '%s' is not a file or directory" % path, file=sys.stderr)
    return sorted(set(files))


def defined_symbols(nm, files):
    """Return the set of defined symbol names found across all files."""
    symbols = set()
    for path in files:
        try:
            out = subprocess.run(
                [nm, "--defined-only", path],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
                universal_newlines=True,
            )
        except FileNotFoundError:
            sys.exit("error: '%s' not found - set --nm or the NM env variable" % nm)

        if out.returncode != 0:
            # Not fatal: some files may not be object files nm understands.
            print("warning: nm failed on '%s': %s" % (path, out.stderr.strip()),
                  file=sys.stderr)
            continue

        for line in out.stdout.splitlines():
            match = _NM_LINE.match(line)
            if match is None:
                continue
            sym_type, name = match.group(1), match.group(2)
            if sym_type in _DEFINED_TYPES and _C_IDENTIFIER.match(name):
                symbols.add(name)
    return symbols


def matching_symbols(symbols, prefixes):
    """Filter symbols to those starting with any of the given prefixes."""
    return sorted(s for s in symbols if any(s.startswith(p) for p in prefixes))


def render_header(macros, guard, prefixes, sources):
    lines = []
    lines.append("/*")
    lines.append(" * Phoenix-RTOS")
    lines.append(" *")
    lines.append(" * libc feature detection macros - GENERATED FILE, DO NOT EDIT.")
    lines.append(" *")
    lines.append(" * Regenerate with phoenix-rtos-tests/scripts/gen_libc_features.py")
    lines.append(" * Prefixes: %s" % ", ".join(prefixes))
    lines.append(" *")
    lines.append(" * A HAS_<FUNC> macro is defined for every matching function that is")
    lines.append(" * present in the scanned libc build. Missing functions leave the")
    lines.append(" * corresponding macro undefined.")
    lines.append(" *")
    lines.append(" * SPDX-License-Identifier: BSD-3-Clause")
    lines.append(" */")
    lines.append("")
    lines.append("#ifndef %s" % guard)
    lines.append("#define %s" % guard)
    lines.append("")
    lines.append("/* clang-format off */")
    lines.append("")
    if macros:
        width = max(len(m) for m in macros)
        for macro in macros:
            lines.append("#define %-*s 1" % (width, macro))
    else:
        lines.append("/* no matching symbols found */")
    lines.append("")
    lines.append("/* clang-format on */")
    lines.append("")
    lines.append("#endif /* %s */" % guard)
    lines.append("")
    return "\n".join(lines)


def default_guard(output):
    if output is None:
        return "LIBC_FEATURES_H"
    base = os.path.basename(output)
    return "_" + re.sub(r"[^A-Za-z0-9]", "_", base).upper() + "_"


def main(argv):
    parser = argparse.ArgumentParser(
        description="Generate HAS_<FUNC> feature macros from a compiled libc.")
    parser.add_argument(
        "inputs", nargs="+", metavar="PATH",
        help="archive (*.a), object (*.o) or directory to scan recursively "
             "(e.g. a libphoenix build directory)")
    parser.add_argument(
        "-p", "--prefix", action="append", default=[], metavar="PREFIX",
        help="symbol name prefix to keep (repeatable, default: pthread_)")
    parser.add_argument(
        "-o", "--output", metavar="FILE",
        help="header file to write (default: stdout)")
    parser.add_argument(
        "--nm", default=os.environ.get("NM", "nm"),
        help="nm binary to use (default: $NM or 'nm')")
    parser.add_argument(
        "--guard", help="include-guard macro name (default: derived from output)")
    args = parser.parse_args(argv)

    prefixes = args.prefix if args.prefix else ["pthread_"]

    files = collect_input_files(args.inputs)
    if not files:
        sys.exit("error: no *.a or *.o files found in the given paths")

    symbols = defined_symbols(args.nm, files)
    matched = matching_symbols(symbols, prefixes)
    macros = ["HAS_" + s.upper() for s in matched]

    guard = args.guard if args.guard else default_guard(args.output)
    header = render_header(macros, guard, prefixes, files)

    if args.output:
        os.makedirs(os.path.dirname(os.path.abspath(args.output)), exist_ok=True)
        with open(args.output, "w") as fp:
            fp.write(header)
        print("wrote %d macro(s) to %s" % (len(macros), args.output), file=sys.stderr)
    else:
        sys.stdout.write(header)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
