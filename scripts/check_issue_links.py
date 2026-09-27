#!/usr/bin/env python3
"""Assert every issue a document links is still open.

An issue or pull request is linked while its work is outstanding, and only then -
CLAUDE.md, *Link the record, not the source*. A reader told that a platform
diverges from a contract wants to know whether that is still true, so an open
issue earns its link; once it closes the link is evidence of nothing and the
prose around it has usually gone stale too.

That makes this mechanically checkable, and it is worth checking because the
failure is invisible. Nothing about a stale link looks wrong on the page: the
0.1.0 documentation audit found ten of them, every one pointing at work finished
months earlier.

**A fault here is not fixed by deleting the link.** Read the prose against the
code first, because the remedy depends on why the issue closed:

    completed    the divergence is gone - the prose and the link go together,
                 and the page says what the library does now
    not planned  the divergence still stands - only the link goes, and the prose
                 stays without promising a fix

The state is reported for exactly that reason.

Two documents are excluded, each because a reference in it is not a promise:

* ``docs/misra-deviations.md``. A deviation records when and under what review it
  was accepted, and that provenance is part of the record rather than history
  about how the code came to be. Agreed 2026-09-21; CLAUDE.md states the
  exception under *Link the record, not the source*.
* ``CHANGELOG.md``. A release record cites the work each release contained, so
  every reference in it is closed by construction.

A reference inside code font is quoted rather than made - CLAUDE.md names
``feedback from PR #407`` to prohibit that form, and must not trip over its own
example - so code spans and fenced blocks are stripped before scanning.

Run:  python3 scripts/check_issue_links.py
"""

import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OWNER = "cososo-ltd"
REPO = "solid-syslog"

SKIP = {
    os.path.join("docs", "misra-deviations.md"): "a deviation's provenance is part of the record",
    "CHANGELOG.md": "a release record cites the work that release contained",
}
SKIP_DIRS = (os.path.join("docs", "generated"),)

FENCED = re.compile(r"^(?P<f>```|~~~).*?^(?P=f)", re.DOTALL | re.MULTILINE)
# A code span runs to the next backtick and may wrap a line, which is how
# CLAUDE.md's own `found during S08.03\n(#290)` example is written.
CODE_SPAN = re.compile(r"`[^`]*?`")

LINKED = re.compile(rf"github\.com/{OWNER}/{REPO}/(?:issues|pull)/(\d+)")
BARE = re.compile(r"(?<![\w/])#(\d{2,5})\b")


def documents():
    """Every page the rule covers: docs/ and the root documents."""
    for name in sorted(os.listdir(ROOT)):
        if name.endswith(".md"):
            yield name
    for path, _, names in os.walk(os.path.join(ROOT, "docs")):
        for name in sorted(names):
            if not name.endswith(".md"):
                continue
            relative = os.path.relpath(os.path.join(path, name), ROOT)
            if not relative.startswith(SKIP_DIRS):
                yield relative


def prose(text):
    """The text with code removed, so a quoted reference is not read as a made one.

    Replaced with spaces rather than deleted, to keep every line number intact.
    """
    for pattern in (FENCED, CODE_SPAN):
        text = pattern.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)
    return text


def referenced():
    """{number: [(page, line)]} for every reference the rule covers."""
    found = {}
    for page in documents():
        if page in SKIP:
            continue
        with open(os.path.join(ROOT, page), encoding="utf-8") as handle:
            text = prose(handle.read())
        for number, line in enumerate(text.splitlines(), 1):
            for pattern in (LINKED, BARE):
                for hit in pattern.finditer(line):
                    # A markdown link matches twice - once in the URL, once in the
                    # `[#N]` text - and one reference is one fault, not two.
                    sites = found.setdefault(int(hit.group(1)), [])
                    if (page, number) not in sites:
                        sites.append((page, number))
    return found


def states(numbers):
    """{number: (state, reason)} in one GraphQL call rather than one per link."""
    if not numbers:
        return {}
    fields = "\n".join(
        f'n{number}: issueOrPullRequest(number: {number}) {{'
        f" ... on Issue {{ state stateReason }}"
        f" ... on PullRequest {{ state }} }}"
        for number in sorted(numbers)
    )
    query = f'query {{ repository(owner: "{OWNER}", name: "{REPO}") {{\n{fields}\n}} }}'
    done = subprocess.run(
        ["gh", "api", "graphql", "-f", f"query={query}"],
        capture_output=True,
        text=True,
        check=False,
    )
    # A reference to a number that does not exist makes `gh` exit non-zero, and it
    # still prints the whole body: `data` carries every number that did resolve and
    # `errors` a NOT_FOUND for the one that did not. So read the body before
    # trusting the exit code - otherwise a dangling reference is reported as an
    # authentication failure and the next reader goes hunting for a token.
    try:
        repository = json.loads(done.stdout)["data"]["repository"]
    except (ValueError, KeyError, TypeError):
        repository = None
    if repository is None:
        sys.exit(
            "could not read issue states from GitHub - this check needs `gh` "
            f"authenticated, or GH_TOKEN set in CI:\n{done.stderr.strip()}"
        )
    resolved = {}
    for key, value in repository.items():
        if value is not None:
            resolved[int(key[1:])] = (value["state"], value.get("stateReason"))
    return resolved


REMEDY = {
    "NOT_PLANNED": "the divergence still stands, so take the link out and leave the prose, no longer promising a fix",
    "COMPLETED": "read the prose against the code: a fixed divergence takes the prose and the link out together",
}


def check():
    faults = []
    found = referenced()
    resolved = states(set(found))
    for number, sites in sorted(found.items()):
        if number not in resolved:
            where = ", ".join(f"{page}:{line}" for page, line in sites)
            faults.append(f"{where} names #{number}, which does not exist in this repository")
            continue
        state, reason = resolved[number]
        if state == "OPEN":
            continue
        for page, line in sites:
            remedy = REMEDY.get(reason or "COMPLETED", REMEDY["COMPLETED"])
            faults.append(
                f"{page}:{line} links #{number}, which is {state.lower()}"
                f"{f' as {reason.lower().replace(chr(95), chr(32))}' if reason else ''} - {remedy}"
            )
    return faults, found, resolved


if __name__ == "__main__":
    problems, references, known = check()
    for problem in problems:
        print(f"error: {problem}", file=sys.stderr)
    if problems:
        print(
            f"\n{len(problems)} problem(s). An issue is linked while its work is "
            "outstanding, and only then.",
            file=sys.stderr,
        )
        sys.exit(1)
    for page, reason in sorted(SKIP.items()):
        print(f"skipped: {page} - {reason}")
    open_count = sum(1 for state, _ in known.values() if state == "OPEN")
    print(
        f"every issue a document links is open: {len(references)} reference(s) to "
        f"{open_count} issue(s)"
    )
