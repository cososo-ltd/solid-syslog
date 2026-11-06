#!/usr/bin/env python3
import argparse
import pathlib
import re
import sys

PROOF = "Bdd/Targets/VxWorks64/BddTargetVxWorks64Headers.c"
PUBLIC_HEADER_GLOBS = ("Core/Interface/*.h", "Platform/VxWorks64/Interface/*.h")
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)


def public_headers(root):
    found = []
    for glob in PUBLIC_HEADER_GLOBS:
        found.extend(sorted(root.glob(glob)))
    return found


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=pathlib.Path)
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    included = set(INCLUDE.findall((root / PROOF).read_text(encoding="utf-8")))
    omitted = [
        header.relative_to(root)
        for header in public_headers(root)
        if header.name not in included
    ]
    for header in omitted:
        print("FAIL %s is not included by %s" % (header.as_posix(), PROOF))
    return 1 if omitted else 0


if __name__ == "__main__":
    sys.exit(main())
