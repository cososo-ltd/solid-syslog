#!/usr/bin/env python3
import argparse
import pathlib
import re
import sys

PROOF = "Bdd/Targets/VxWorks64/BddTargetVxWorks64Headers.c"
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=pathlib.Path)
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    included = set(INCLUDE.findall((root / PROOF).read_text(encoding="utf-8")))
    omitted = [
        header.relative_to(root)
        for header in sorted(root.glob("Core/Interface/*.h"))
        if header.name not in included
    ]
    for header in omitted:
        print("FAIL %s is not included by %s" % (header.as_posix(), PROOF))
    return 1 if omitted else 0


if __name__ == "__main__":
    sys.exit(main())
