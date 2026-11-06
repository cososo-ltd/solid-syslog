#!/usr/bin/env python3
"""Assert the VxWorks 6.4 C89 header proof includes every public header.

`Bdd/Targets/VxWorks64/BddTargetVxWorks64Headers.c` is compiled by the VIP at
C89, after `vxWorks.h`, to prove the public headers of Core and the VxWorks64
pack compile in an application built that way. It proves that only of the
headers it includes, and the list is kept by hand. No CI lane compiles the
file, since that needs the Wind River toolchain, so nothing else notices a
header left off it.

Two claims, checked from the file and the two directory listings:

- every header under `Core/Interface/` and `Platform/VxWorks64/Interface/` is
  included by the file;
- the file includes nothing else of ours. An include naming a header that has
  been renamed or removed would stop the VIP build, which CI cannot see either.

Run:  python3 scripts/check_vxworks64_header_proof.py
      python3 scripts/check_vxworks64_header_proof.py --root /path/to/checkout

Exit code:
    0  the file includes every public header, and only those
    1  it does not
"""

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

PROOF = "Bdd/Targets/VxWorks64/BddTargetVxWorks64Headers.c"
PUBLIC_HEADER_GLOBS = ("Core/Interface/*.h", "Platform/VxWorks64/Interface/*.h")

# The one include that is not ours: the file includes it first, as Wind River
# code does, so the public headers are compiled after it.
SYSTEM_HEADER = "vxWorks.h"

INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)


def public_headers(root):
    found = []
    for glob in PUBLIC_HEADER_GLOBS:
        found.extend(sorted(root.glob(glob)))
    return found


def included_headers(root):
    text = (root / PROOF).read_text(encoding="utf-8")
    return [name for name in INCLUDE.findall(text) if name != SYSTEM_HEADER]


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=ROOT)
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    headers = public_headers(root)
    included = included_headers(root)
    public_names = {header.name for header in headers}

    omitted = [header for header in headers if header.name not in included]
    unknown = [name for name in included if name not in public_names]

    for header in omitted:
        print("FAIL %s is not included by %s" % (header.relative_to(root).as_posix(), PROOF))
    for name in unknown:
        print("FAIL %s includes %s, which is not a public header of Core or the VxWorks64 pack" % (PROOF, name))

    print(
        "\n%d public headers of Core and the VxWorks64 pack checked against %s, %d omitted, %d unknown"
        % (len(headers), PROOF, len(omitted), len(unknown))
    )
    return 1 if omitted or unknown else 0


if __name__ == "__main__":
    sys.exit(main())
