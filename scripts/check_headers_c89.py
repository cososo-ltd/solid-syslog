#!/usr/bin/env python3
"""Assert every public header compiles standalone as ISO C89.

The library is C99, but its headers are what an integrator's code includes, and
that code may be older than the library. A consumer on a C89 toolchain -- an
established embedded build that cannot be moved -- can link a C99 library, so
the headers should not be what stops them.

Each header is compiled on its own, so the check also proves self-containment:
a header that needs a companion included first fails here.

    cc -std=c89 -pedantic-errors -c <tu including the header>

`-pedantic-errors` is what makes this meaningful. Without it GCC accepts C99
constructs in C89 mode silently, and the check would pass on `//` comments,
declarations after statements and trailing enum commas alike.

The language is the whole claim. The headers include <stdint.h>, <stdbool.h> and
<stddef.h>, and the first two are C99 *library* headers that a strict C89
implementation need not provide -- so "C89 language, given those headers" is the
property, and a toolchain lacking them is out of scope here.

Usage:
    scripts/check_headers_c89.py [--cc <compiler>] [--root <repo root>]

Exit code:
    0  every public header compiles as C89
    1  at least one does not
"""

import argparse
import os
import pathlib
import subprocess
import sys
import tempfile

STANDARD = "c89"

# The public surface: what an integrator is entitled to include. Sources and
# private headers are excluded -- they are ours, and they are C99.
PUBLIC_HEADER_GLOBS = ("Core/Interface/*.h", "Platform/*/Interface/*.h")


def public_headers(root):
    found = []
    for glob in PUBLIC_HEADER_GLOBS:
        found.extend(sorted(root.glob(glob)))
    return found


def include_directories(root):
    directories = [root / "Core" / "Interface"]
    directories.extend(sorted(root.glob("Platform/*/Interface")))
    return directories


# ISO C forbids an empty translation unit, and a header that only opens an
# `extern "C"` block for C++ consumers leaves one. The typedef keeps the unit
# non-empty without declaring anything the header could collide with.
TRANSLATION_UNIT = '#include "%s"\ntypedef int SolidSyslogHeaderCheckNotEmpty;\n'


def compile_standalone(compiler, header, includes, workdir):
    """Compile a translation unit whose only content is this header."""
    translation_unit = workdir / "tu.c"
    translation_unit.write_text(
        TRANSLATION_UNIT % header.name, encoding="utf-8"
    )
    command = [compiler, "-std=%s" % STANDARD, "-pedantic-errors"]
    command.extend("-I%s" % directory for directory in includes)
    command.extend(["-c", str(translation_unit), "-o", os.devnull])
    completed = subprocess.run(
        command, capture_output=True, text=True, check=False
    )
    return completed.returncode, completed.stderr


def first_diagnostic(stderr):
    for line in stderr.splitlines():
        if "error:" in line:
            return line.strip()
    return stderr.strip().splitlines()[0] if stderr.strip() else "(no output)"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default=os.environ.get("CC", "cc"))
    parser.add_argument("--root", default=".", type=pathlib.Path)
    arguments = parser.parse_args()

    root = arguments.root.resolve()
    headers = public_headers(root)
    if not headers:
        print("no public headers found under %s" % root, file=sys.stderr)
        return 1

    includes = include_directories(root)
    failures = []
    with tempfile.TemporaryDirectory() as directory:
        workdir = pathlib.Path(directory)
        for header in headers:
            code, stderr = compile_standalone(
                arguments.cc, header, includes, workdir
            )
            if code != 0:
                failures.append((header.relative_to(root), first_diagnostic(stderr)))

    for header, diagnostic in failures:
        print("FAIL %s\n     %s" % (header, diagnostic))

    print(
        "\n%d public headers checked against ISO %s, %d failed"
        % (len(headers), STANDARD.upper(), len(failures))
    )
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
