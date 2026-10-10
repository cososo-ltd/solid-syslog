#!/usr/bin/env python3
"""Move the line numbers in misra_suppressions.txt with the code they annotate.

An edit that adds or removes lines above a suppressed line leaves its entry in
misra_suppressions.txt pointing at the wrong line, and the cppcheck-misra gate
then fails on a finding it has always passed. This script reads the branch's
diff and moves each such entry by the lines added and removed above it. It
runs no analysis, so it takes seconds and needs no vendor headers.

Usage:
    scripts/misra_renumber.py                # show proposed updates
    scripts/misra_renumber.py --apply        # write them back
    scripts/misra_renumber.py --base <ref>   # diff against <ref>, not the
                                             # merge base with origin/main

The diff runs from the base to the working tree, so uncommitted edits count:
run it after the clang-format reflow, which moves lines too.

Exit code:
    0  nothing to update (or, with --apply, everything updated)
    1  updates proposed - re-run with --apply
    2  entries needing attention, listed; the others are still proposed or
       applied

Only entries that already existed at the base are moved. An entry added on the
branch already names its new line, and an entry moved by an earlier run no
longer matches the base, so a second run changes nothing.

An entry needs attention when its suppressed line was itself edited or removed,
or when the text at the computed line differs from the text at the old one.
The script never guesses those: CI's MISRA lane reports the finding at its new
line, and the suppression is updated from that.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SUPPRESSIONS = "misra_suppressions.txt"
ENTRY = re.compile(r"^(?P<rule>[^:#\s]+):(?P<path>[^:]+):(?P<line>\d+)\s*$")
HUNK = re.compile(r"^@@ -(?P<old>\d+)(?:,(?P<oldcount>\d+))? \+(?P<new>\d+)(?:,(?P<newcount>\d+))? @@")


def git(*args):
    return subprocess.run(["git", *args], cwd=REPO_ROOT, check=True, capture_output=True, text=True).stdout


def default_base():
    return git("merge-base", "HEAD", "origin/main").strip()


# Each hunk as (first old line, old line count, new line count). A hunk that
# only adds lines has an old count of 0 and its first old line is the line the
# additions follow.
def parse_hunks(diff_text):
    hunks = []
    for line in diff_text.splitlines():
        match = HUNK.match(line)
        if match:
            old_count = int(match["oldcount"]) if match["oldcount"] is not None else 1
            new_count = int(match["newcount"]) if match["newcount"] is not None else 1
            hunks.append((int(match["old"]), old_count, new_count))
    return hunks


# The line an unchanged old line moves to, or None if the diff edited or
# removed it.
def map_line(hunks, old_line):
    shift = 0
    for start, old_count, new_count in hunks:
        if old_count == 0:
            if start < old_line:
                shift += new_count
        elif old_line < start:
            break
        elif old_line < start + old_count:
            return None
        else:
            shift += new_count - old_count
    return old_line + shift


def entries(text):
    result = []
    for line in text.splitlines():
        match = ENTRY.match(line)
        if match:
            result.append((match["rule"], match["path"], int(match["line"])))
    return result


def line_at(text, number):
    lines = text.splitlines()
    return lines[number - 1] if 0 < number <= len(lines) else None


# Returns (updates, attention): updates maps an entry to its new line;
# attention lists entries that cannot be moved safely, with the reason.
def plan(base, current_suppressions):
    base_entries = set(entries(git("show", f"{base}:{SUPPRESSIONS}")))
    updates = {}
    attention = []
    diffs = {}
    for entry in entries(current_suppressions):
        rule, path, line = entry
        if entry not in base_entries:
            continue
        if path not in diffs:
            diffs[path] = git("diff", "-U0", base, "--", path)
        if not diffs[path]:
            continue
        new_line = map_line(parse_hunks(diffs[path]), line)
        if new_line is None:
            attention.append((entry, "the suppressed line was edited or removed"))
            continue
        working = REPO_ROOT / path
        if not working.exists():
            attention.append((entry, "the file no longer exists"))
            continue
        old_text = line_at(git("show", f"{base}:{path}"), line)
        new_text = line_at(working.read_text(encoding="utf-8"), new_line)
        if old_text is None or new_text is None or old_text.strip() != new_text.strip():
            attention.append((entry, f"line {new_line} does not hold the line that was suppressed"))
            continue
        if new_line != line:
            updates[entry] = new_line
    return updates, attention


def rewrite(text, updates):
    out = []
    for line in text.splitlines(keepends=True):
        match = ENTRY.match(line.rstrip("\r\n"))
        key = (match["rule"], match["path"], int(match["line"])) if match else None
        if key in updates:
            ending = line[len(line.rstrip("\r\n")):]
            line = f"{key[0]}:{key[1]}:{updates[key]}{ending}"
        out.append(line)
    return "".join(out)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--apply", action="store_true", help="write the updates back")
    parser.add_argument("--base", help="the ref to diff against (default: merge base with origin/main)")
    options = parser.parse_args(argv)

    base = options.base or default_base()
    suppressions_path = REPO_ROOT / SUPPRESSIONS
    current = suppressions_path.read_text(encoding="utf-8")
    updates, attention = plan(base, current)

    for (rule, path, line), new_line in sorted(updates.items()):
        print(f"  {rule}:{path}:{line} -> {new_line}")
    for (rule, path, line), reason in attention:
        print(f"ATTENTION {rule}:{path}:{line} - {reason}")

    if options.apply and updates:
        with open(suppressions_path, "w", encoding="utf-8", newline="") as out:
            out.write(rewrite(current, updates))
        print(f"applied {len(updates)} update(s)")

    if attention:
        return 2
    if updates and not options.apply:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
