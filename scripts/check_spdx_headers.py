#!/usr/bin/env python3
"""Assert every shipped source file carries our licence header, and only ours.

Two separate claims, checked over the same files.

**Every file says how it is licensed.** The library is consumed by copying
sources out of the tree as well as by building it in place -- `solidsyslog.mk`
and the generated manifest both hand an integrator a list of `.c` files to bring
into their own build. Once copied, `LICENSE.md` is left behind, and a per-file
header is the only statement of terms that travels with the code. So each file
under `Core/` and `Platform/` opens with:

    /* SPDX-FileCopyrightText: <the Required Notice from LICENSE.md>
     * SPDX-License-Identifier: <the SPDX expression from LICENSE.md>
     */

Neither value is written here. Both are read out of `LICENSE.md` at run time,
so the headers cannot drift from the licence they claim: change the model in
`LICENSE.md` and this fails until the tree agrees again.

**No file claims anyone else's copyright.** `Core/` and `Platform/` contain no
third-party code, and that is what makes it safe to stamp our copyright across
all of them. If third-party source is ever dropped in because it was
convenient, the header sweep would assert our copyright over someone else's
work in a machine-readable field that scanners believe -- and risk breaching
that code's own terms. So any attribution naming someone other than us is an
error here.

There is deliberately **no allowlist** for that second check. A third-party
notice inside `Core/` or `Platform/` means either the file does not belong in
the shipped library or the scope of this check is wrong, and both are decisions
for a person. Vendored code lives outside these trees -- see the Arm driver
under `Bdd/Targets/FreeRtosLwip/netif/smsc9220/`, which keeps its own Apache-2.0
headers and is out of scope precisely so it can.

Run:  python3 scripts/check_spdx_headers.py
      python3 scripts/check_spdx_headers.py --root /path/to/checkout
"""

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# The shipped library: Tier 1 and Tier 2. Tests/, Bdd/ and ci/ are out of scope
# -- they are not shipped, and Bdd/Targets/ legally consumes other projects'
# code, so stamping our copyright there would be a false claim.
SCOPE = ("Core", "Platform")
SUFFIXES = (".c", ".h")

# Where the two values come from, so there is one authority for both.
NOTICE = re.compile(r"^Required Notice:\s*(.+?)\s*$", re.MULTILINE)
EXPRESSION = re.compile(r"^```text\n(.+?)\n```", re.MULTILINE | re.DOTALL)

# An attribution, as opposed to the word "licence" appearing in prose. Core's
# Datagram headers say a SENT result "licenses the sender to drop the record",
# which a looser pattern would flag; these match a copyright claim only.
ATTRIBUTION = re.compile(
    r"©|copyright|all rights reserved|SPDX-FileCopyrightText|@author|\(c\)\s*\d{4}",
    re.IGNORECASE,
)


def licence_facts(root):
    """The copyright notice and SPDX expression LICENSE.md declares."""
    text = (root / "LICENSE.md").read_text(encoding="utf-8")
    notice = NOTICE.search(text)
    expression = EXPRESSION.search(text)
    if notice is None:
        sys.exit("LICENSE.md has no 'Required Notice:' line to take the copyright from")
    if expression is None:
        sys.exit("LICENSE.md has no ```text block to take the SPDX expression from")
    return notice.group(1), expression.group(1).strip()


def expected_header(notice, expression):
    """The exact lines every in-scope file must open with."""
    return [
        f"/* SPDX-FileCopyrightText: {notice}",
        f" * SPDX-License-Identifier: {expression}",
        " */",
    ]


def sources(root):
    for directory in SCOPE:
        for path in sorted((root / directory).rglob("*")):
            if path.suffix in SUFFIXES and path.is_file():
                yield path


def header_faults(lines, header):
    """Why this file's opening lines are not the header, or None if they are."""
    if lines[: len(header)] != header:
        for index, want in enumerate(header):
            got = lines[index] if index < len(lines) else "<end of file>"
            if got != want:
                return f"line {index + 1} is {got!r}, expected {want!r}"
    if len(lines) > len(header) and lines[len(header)].strip():
        return "no blank line after the header block"
    return None


def foreign_attributions(lines, header):
    """Attribution lines below the header, which is where ours is allowed."""
    return [
        f"line {number}: {line.strip()!r}"
        for number, line in enumerate(lines[len(header) :], start=len(header) + 1)
        if ATTRIBUTION.search(line)
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=ROOT)
    arguments = parser.parse_args()
    root = arguments.root.resolve()

    notice, expression = licence_facts(root)
    header = expected_header(notice, expression)

    missing, foreign, checked = [], [], 0
    for path in sources(root):
        checked += 1
        lines = path.read_text(encoding="utf-8").splitlines()
        relative = path.relative_to(root)
        fault = header_faults(lines, header)
        if fault is not None:
            missing.append(f"{relative}: {fault}")
        for hit in foreign_attributions(lines, header):
            foreign.append(f"{relative}: {hit}")

    if missing:
        print(f"{len(missing)} file(s) without the licence header:", file=sys.stderr)
        for fault in missing:
            print(f"  {fault}", file=sys.stderr)
        print(
            "\nEvery file under Core/ and Platform/ must open with:\n"
            + "\n".join(f"  {line}" for line in header)
            + "\nfollowed by a blank line. Both values come from LICENSE.md.",
            file=sys.stderr,
        )
    if foreign:
        print(
            f"\n{len(foreign)} copyright attribution(s) below the header:",
            file=sys.stderr,
        )
        for hit in foreign:
            print(f"  {hit}", file=sys.stderr)
        print(
            "\nCore/ and Platform/ carry no third-party code -- that invariant is\n"
            "what makes it safe to stamp our copyright across every file in them.\n"
            "Third-party source belongs outside these trees, keeping its own\n"
            "notices. There is no allowlist here on purpose.",
            file=sys.stderr,
        )
    if missing or foreign:
        return 1

    print(
        f"licence headers: {checked} files under {'/, '.join(SCOPE)}/ "
        f"carry the header LICENSE.md declares, and no other attribution"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
