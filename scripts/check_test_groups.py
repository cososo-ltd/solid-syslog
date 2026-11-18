#!/usr/bin/env python3
"""Assert each CppUTest group's tests sit together in their file.

CppUTest's JUnit output writes one file per group, `cpputest_<package>_<group>.xml`,
each time a run of tests in that group ends. Where a group's tests are
interrupted by another group's and then resume, the group ends twice, and the
second write replaces the first. Every test still runs and a failure still
fails its executable, but the tests in the overwritten part are missing from
the XML, so the test report understates the suite and would not show one of
them failing.

So a group, once another group's test has followed it, may not resume:

    TEST(Stream, Opens)
    TEST(StreamPool, Exhausts)
    TEST(Stream, Closes)        <- resumes Stream; reported, Opens is lost

The check is per file. A group shared between files of one executable would
lose tests the same way, and is not looked for here.

Run:  python3 scripts/check_test_groups.py
      python3 scripts/check_test_groups.py --root /path/to/checkout
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

SCOPE = ("Tests",)
SUFFIXES = (".cpp",)

# A test's opening macro at the start of a line, its group and name possibly on
# the lines below it. TEST_GROUP and TEST_GROUP_BASE do not match: the macro
# name must be followed directly by its bracket.
TEST = re.compile(r"^[ \t]*(?:IGNORE_TEST|TEST_F|TEST)\s*\(\s*(\w+)\s*,\s*(\w+)", re.MULTILINE)


def sources(root):
    for directory in SCOPE:
        for path in sorted((root / directory).rglob("*")):
            if path.suffix in SUFFIXES and path.is_file():
                yield path


def tests(text):
    """(line number, group, name) for each test, in file order."""
    return [(text.count("\n", 0, match.start()) + 1, match.group(1), match.group(2)) for match in TEST.finditer(text)]


def resumed_groups(text):
    """(line number, group, name) for the first test of each resumption."""
    resumptions, finished, current = [], set(), None
    for line, group, name in tests(text):
        if group != current:
            if group in finished:
                resumptions.append((line, group, name))
            if current is not None:
                finished.add(current)
            current = group
    return resumptions


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=pathlib.Path, default=ROOT)
    arguments = parser.parse_args(argv)
    root = arguments.root.resolve()

    faults, checked = [], 0
    for path in sources(root):
        checked += 1
        relative = path.relative_to(root).as_posix()
        for line, group, name in resumed_groups(path.read_text(encoding="utf-8")):
            faults.append((relative, line, f"TEST({group}, {name}) resumes {group} after another group's tests"))

    if not faults:
        print(f"Test groups: {checked} files, each group's tests together.")
        return 0

    print(f"Test groups resumed after another group's tests: {len(faults)}")
    print(
        "CppUTest's JUnit output keeps only one run of each group, so the rest are\n"
        "missing from the test report. Move each test below up to join the rest of\n"
        "its group."
    )
    for path, line, message in faults[:MAX_LISTED]:
        print(f"  {path}:{line}: {message}")
    if len(faults) > MAX_LISTED:
        print(f"  ... and {len(faults) - MAX_LISTED} more")
    if ON_ACTIONS:
        for path, line, message in faults[:MAX_LISTED]:
            print(f"::error file={path},line={line}::{message}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
