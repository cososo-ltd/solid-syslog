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
import os
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# GitHub Actions drops annotations past ten per step, so listing more than
# that produces output nobody sees. The same cap keeps a local run readable.
MAX_LISTED = 10
ON_ACTIONS = os.environ.get("GITHUB_ACTIONS") == "true"

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


def licence_owner(notice):
    """The organisation the Required Notice names, without its year prefix."""
    return re.sub(r"^(?:copyright\s+)?(?:©\s*)?\d{4}(?:\s*[-\u2013]\s*\d{4})?\s*", "", notice, flags=re.IGNORECASE).strip()


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
    """(line number, why) this file does not open with the header, or None."""
    if lines[: len(header)] != header:
        for index, want in enumerate(header):
            got = lines[index] if index < len(lines) else "<end of file>"
            if got != want:
                return index + 1, f"line {index + 1} is {got!r}, expected {want!r}"
    if len(lines) > len(header) and lines[len(header)].strip():
        return len(header) + 1, "no blank line after the header block"
    return None


def describe_attribution(text, owner):
    """Say which kind of attribution this is, having compared it to the owner.

    All three fail. The classification is for the reader, not for the decision:
    an attribution below our header is anomalous whoever it names, and deciding
    it is harmless on the strength of a name match would be the one mistake a
    tripwire cannot afford. Holder strings vary far too much -- "(c) 2019-2024
    Foo Ltd and contributors", "\u00a9 Foo" -- for a parser to be trusted in the
    permissive direction, and a wrong "this one is ours" is a silent miss.
    """
    if "@author" in text.lower():
        return f"authorship tag below the header, which usually means pasted code: {text!r}"
    if owner.lower() in text.lower():
        return f"our own copyright repeated below the header block: {text!r}"
    return f"copyright attribution not naming {owner}: {text!r}"


def foreign_attributions(lines, header, owner):
    """(line number, description) for attribution lines below our own header."""
    return [
        (number, describe_attribution(line.strip(), owner))
        for number, line in enumerate(lines[len(header) :], start=len(header) + 1)
        if ATTRIBUTION.search(line)
    ]


def annotate(path, line, message):
    """A GitHub Actions error annotation, which lands on the file in the diff.

    Without one, a failing run shows only "Process completed with exit code 1"
    and the detail is left in the log for someone to go and find. Actions caps
    annotations at ten per step, which is why the listings below are capped to
    match rather than emitting hundreds that get silently dropped.
    """
    print(f"::error file={path},line={line}::{message}")


def report(stream, title, entries, remedy, header):
    """One failing check: a count, the remedy, then a capped listing.

    The remedy comes before the listing on purpose. It used to follow it, which
    put the one actionable paragraph 373 lines below the top of the log.
    """
    print(f"\n{title}: {len(entries)}", file=stream)
    print(remedy, file=stream)
    for path, line, message in entries[:MAX_LISTED]:
        print(f"  {path}:{line}: {message}", file=stream)
    if len(entries) > MAX_LISTED:
        print(f"  ... and {len(entries) - MAX_LISTED} more", file=stream)
    if ON_ACTIONS:
        for path, line, message in entries[:MAX_LISTED]:
            annotate(path, line, message)


def step_summary(text):
    """Append to the run's job summary, which is the page a reader lands on."""
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if path:
        with open(path, "a", encoding="utf-8") as handle:
            handle.write(text + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=ROOT)
    arguments = parser.parse_args()
    root = arguments.root.resolve()

    notice, expression = licence_facts(root)
    owner = licence_owner(notice)
    header = expected_header(notice, expression)
    block = "\n".join(f"  {line}" for line in header)

    missing, foreign, checked = [], [], 0
    for path in sources(root):
        checked += 1
        lines = path.read_text(encoding="utf-8").splitlines()
        relative = path.relative_to(root)
        fault = header_faults(lines, header)
        if fault is not None:
            missing.append((relative, fault[0], fault[1]))
        for line, description in foreign_attributions(lines, header, owner):
            foreign.append((relative, line, description))

    if missing:
        report(
            sys.stderr,
            "Files without the licence header",
            missing,
            "\nEvery file under "
            + "/ and ".join(SCOPE)
            + "/ must open with:\n"
            + block
            + "\nfollowed by a blank line. Both values are read from LICENSE.md,\n"
            "so fix them there if the licence itself has changed.\n",
            header,
        )
    if foreign:
        report(
            sys.stderr,
            "Attribution lines below the licence header",
            foreign,
            "\nCore/ and Platform/ carry no third-party code, and that invariant is\n"
            "what makes it safe to stamp our copyright across every file in them.\n"
            "Our own notice belongs in the header block and nowhere else, so any\n"
            "attribution below it is anomalous whoever it names. Third-party\n"
            "source belongs outside these trees, keeping its own notices. There\n"
            "is deliberately no allowlist: if this fires, the question is whether\n"
            "the file belongs in the shipped library at all.\n",
            header,
        )

    if missing or foreign:
        step_summary(
            f"### Licence headers: {len(missing) + len(foreign)} problem(s)\n\n"
            f"- {len(missing)} of {checked} file(s) missing the header\n"
            f"- {len(foreign)} attribution line(s) below the header\n\n"
            f"Expected header:\n\n```c\n" + "\n".join(header) + "\n```\n\n"
            "Both values come from `LICENSE.md`. Run `python3 "
            "scripts/check_spdx_headers.py` for the full list."
        )
        return 1

    print(
        f"licence headers: {checked} files under "
        + "/ and ".join(SCOPE)
        + "/ carry the header LICENSE.md declares, and no other attribution"
    )
    step_summary(
        f"### Licence headers\n\n{checked} files under "
        + "/ and ".join(SCOPE)
        + "/ carry the header `LICENSE.md` declares, and no other attribution."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
