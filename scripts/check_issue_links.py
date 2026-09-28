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

# The closing fence must repeat the opening one exactly and carry nothing but
# whitespace, or a longer fence is closed early by a shorter line inside it and
# the rest of the block is read as prose.
FENCED = re.compile(
    r"^(?P<fence>`{3,}|~{3,})[^\n]*\n.*?^(?P=fence)[ \t]*$",
    re.DOTALL | re.MULTILINE,
)
# A code span runs to the next backtick and may wrap a line, which is how
# CLAUDE.md's own `found during S08.03\n(#290)` example is written.
CODE_SPAN = re.compile(r"`[^`]*?`")

LINKED = re.compile(rf"github\.com/{OWNER}/{REPO}/(?:issues|pull)/(\d+)")
# No ceiling - an issue number only grows. The floor of two digits is deliberate:
# a single-digit `#N` in this repository's prose is a project number, not an issue
# ("project board \"SolidSyslog\" (project #1)"), and #1 is a release pull request,
# so accepting one digit would fail this check on a correct sentence.
BARE = re.compile(r"(?<![\w/])#(\d{2,})\b")


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


def graphql(query):
    """(data, errors) from one `gh api graphql` call.

    `gh` exits non-zero whenever GraphQL reports any error, and still prints the
    whole body: `data` carries every field that resolved and `errors` says what did
    not. So read the body before trusting the exit code - otherwise a reference to
    a number that does not exist is reported as an authentication failure and the
    next reader goes hunting for a token.
    """
    done = subprocess.run(
        ["gh", "api", "graphql", "-f", f"query={query}"],
        capture_output=True,
        text=True,
        check=False,
    )
    try:
        body = json.loads(done.stdout)
    except ValueError:
        body = None
    if not isinstance(body, dict) or body.get("data") is None:
        sys.exit(
            "could not reach GitHub - this check needs `gh` authenticated, or "
            f"GH_TOKEN set in CI:\n{done.stderr.strip()}"
        )
    return body["data"], body.get("errors") or []


def confirm_readable():
    """Prove the token can read issue state even when no document links an issue.

    Without this the query below never runs on a clean tree, so a workflow token
    that cannot read issues passes every time until the day somebody links one -
    a gate whose first real execution is the one that matters. One query buys that
    away. It reads the issue connection rather than a fixed number, which no
    renumbering or deletion can invalidate.
    """
    query = (
        f'query {{ repository(owner: "{OWNER}", name: "{REPO}") '
        "{ issues(first: 1) { nodes { number state } } } }"
    )
    data, errors = graphql(query)
    readable = (data.get("repository") or {}).get("issues")
    if errors or readable is None:
        detail = "; ".join(error.get("message", "?") for error in errors) or "no issues returned"
        sys.exit(
            "the token cannot read issue state, so this check could not run: "
            f"{detail}\nIn CI the docs-build job needs `issues: read` and "
            "`pull-requests: read` alongside `contents: read`."
        )


def states(numbers):
    """{number: (state, reason)} in one GraphQL call rather than one per link."""
    if not numbers:
        confirm_readable()
        return {}
    fields = "\n".join(
        f'n{number}: issueOrPullRequest(number: {number}) {{'
        f" ... on Issue {{ state stateReason }}"
        f" ... on PullRequest {{ state }} }}"
        for number in sorted(numbers)
    )
    query = f'query {{ repository(owner: "{OWNER}", name: "{REPO}") {{\n{fields}\n}} }}'
    data, errors = graphql(query)
    repository = data.get("repository")
    if repository is None:
        sys.exit("GitHub returned no repository for the issue-state query")

    # A field may be null only because GraphQL confirmed NOT_FOUND. Any other error
    # left it null for a reason that is not "no such issue", and reporting it as one
    # would describe a token that cannot read issues as every reference having been
    # deleted - the wrong diagnosis, and the expensive kind.
    refused = [error for error in errors if error.get("type") != "NOT_FOUND"]
    if refused:
        detail = "\n".join(f"  {error.get('type', '?')}: {error.get('message', '?')}" for error in refused)
        sys.exit(f"GitHub rejected the issue-state query:\n{detail}")

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
