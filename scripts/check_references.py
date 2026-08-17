#!/usr/bin/env python3
"""Assert that what a document or a build file names still exists.

Prose and build files are full of references into the repository, and almost
none of them are checked. `mkdocs build --strict` resolves a Markdown *link*
between two pages, and the docs-links workflow resolves an external URL, but a
path written in code font in a sentence is just text, and a path in a CMake
list, a Compose mount or a workflow's path filter is text nothing reads back
out. A rename leaves the reference behind, green, and pointing nowhere.

This is one pass over those files, pulling out every reference of a known kind
and asserting each resolves. Today there is one kind, the repo-relative path.
The second, deferred to #740, is the SolidSyslog symbol a page names: the same
files, the same extraction pass, and the same exception problem, which is why it
belongs here as another KINDS row rather than in a second script.

**The extraction is deliberately narrow**, because a heuristic loose enough to
need a long exception list is one that will be switched off. Three rules do the
work:

* A token is read as a path only when its first segment is something git tracks
  at the top of the repository, or `.` / `..`. That excludes an include of a
  third-party header, a URL, and every `and/or` in a sentence without naming any
  of them. What it costs is a top-level directory renamed wholesale.
* Anything git ignores is a build artefact rather than a reference — it does not
  exist in a fresh checkout and asserting it would be asserting the build ran.
* A YAML block scalar is a shell script, not repository text. Its paths are
  relative to a working directory this cannot know, and most of them name files
  a job creates. Nothing is lost by leaving them: a shell command naming a path
  that does not exist fails the job it is in, which is exactly what a nav entry
  or a path filter does not do.

**The exception list is the part to watch.** Each entry says why the reference
is meant not to resolve. All of them so far are one thing: a document quoting a
path as some *other* file would write it, to state a rule about how paths are
written. Past a handful of entries the extraction is wrong and should be
tightened rather than the list grown.

Run:  python3 scripts/check_references.py
"""

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Deliberate exceptions, each with the reason the reference is meant not to
# resolve. Matched on (file, token). Printed on every run: an exception nobody
# sees is an exception nobody revisits.
ALLOWED = [
    (
        "CLAUDE.md",
        "../LICENSE.md",
        "states the rule for how a page under docs/ links a root document, so the "
        "path is counted from that page rather than from here",
    ),
    (
        "CLAUDE.md",
        "../../LICENSE.md",
        "the same rule, counted from a page one level deeper",
    ),
    (
        "mkdocs.yml",
        "../SECURITY.md",
        "names a link as a docs page writes it, to say what the source-link hook rewrites",
    ),
    (
        ".github/workflows/ci.yml",
        "../SECURITY.md",
        "the same link, in the comment on the step that would fail if the hook "
        "stopped rewriting it",
    ),
    (
        "Tests/Lwip/CMakeLists.txt",
        "Tests/X/",
        "the shape the next platform's test directory would take, not one that exists",
    ),
]

# Where references are looked for. Build files are here because nothing else
# reads them back: a path in a CMake list or a path filter is checked by the job
# it belongs to failing, months later, on a branch that did not touch it.
SCANNED_NAMES = ("CMakeLists.txt",)
SCANNED_SUFFIXES = (".md", ".yaml", ".yml", ".cmake")

# A fence opens and closes a block whose whole content is candidate text rather
# than prose.
FENCE = re.compile(r"^\s*(?:```|~~~)")

# Markdown carries references in two shapes: a code span, and a link target.
CODE_SPAN = re.compile(r"`([^`\n]+)`")
LINK_TARGET = re.compile(r"\]\(([^)\s]+)\)")

# A YAML key introducing a block scalar, and the indent its body sits under.
BLOCK_SCALAR = re.compile(r"^(\s*)(?:-\s+)?[^:#]+:\s*[|>][-+0-9]*\s*(?:#.*)?$")

# A step's script written inline rather than as a block. Same reasoning: it is
# shell, so its paths are the job's, not the repository's.
SHELL = re.compile(r"^\s*(?:-\s+)?(?:run|command|entrypoint):\s*\S")

# Punctuation a token collects from the sentence or the syntax around it. A
# trailing full stop goes; a trailing `.md` does not, since `d` is not stripped.
LEADING = "([{<\"'`,;:"
TRAILING = ".,;:!?)]}>\"'`"

# What disqualifies a word before its shape is considered: a URL or an address,
# a glob or a placeholder, a shell or CMake variable, or a character no path in
# this repository uses. Each names something other than one file here.
NOT_A_PATH = re.compile(r"://|[^A-Za-z0-9._/+:-]")


def read(relative):
    with open(os.path.join(ROOT, relative), encoding="utf-8") as handle:
        return handle.read()


def git(*arguments, stdin=None):
    """Git is the authority on what this repository holds and what it ignores;
    a hand-kept copy of either would be one more thing to keep in step."""
    return subprocess.run(
        ["git", "-C", ROOT, *arguments],
        input=stdin,
        capture_output=True,
        text=True,
        check=False,
    ).stdout.splitlines()


def tracked_roots():
    """What git tracks at the top of the repository — the first segment a path
    must have to be read as one. Untracked output (`build/`, `site/`) is not
    here, so nothing under it is ever mined."""
    return {name.split("/")[0] for name in git("ls-files")}


def ignored(paths):
    """The subset git ignores: build artefacts, which a fresh checkout lacks."""
    if not paths:
        return set()
    return set(git("check-ignore", "--stdin", stdin="\n".join(sorted(paths)) + "\n"))


def scanned():
    """Every file references are looked for in, repo-relative and sorted. Taken
    from git rather than from a walk, so generated trees never appear."""
    return sorted(
        name
        for name in git("ls-files")
        if os.path.basename(name) in SCANNED_NAMES or name.endswith(SCANNED_SUFFIXES)
    )


def candidates(relative, line, verbatim):
    """The text on one line that may hold a reference.

    Prose is mined only inside a code span or a link target — a path in a
    sentence is written in code font by convention, and one that is not is prose
    about a path rather than a reference to it. Verbatim text, a fenced block or
    a build file, is candidate text whole.
    """
    if relative.endswith(".md") and not verbatim:
        return [m.group(1) for m in CODE_SPAN.finditer(line)] + [
            m.group(1) for m in LINK_TARGET.finditer(line)
        ]
    return [line]


def words(text):
    """The path-shaped words of some candidate text, unpunctuated.

    A word is split on `:` after it has been judged, so a Compose mount
    (`../Bdd/output:/var/log`) yields both sides and a URL yields neither.
    """
    for word in text.split():
        word = word.lstrip(LEADING).rstrip(TRAILING)
        if word and not NOT_A_PATH.search(word):
            for part in word.split(":"):
                yield part


def names_a_file(token):
    """A path is written as one: it names a file, or it carries the trailing
    slash that says it is a directory.

    The repository is full of slash-joined names that are not paths — a branch
    (`ci/pin-action-shas`), a component (`Bdd/Targets/Common/BddTargetInteractive`),
    a test (`Tests/Lwip/SolidSyslogLwipRawDnsResolverTest`), a pair of
    directories written as one (`Core/Platform`). Requiring the shape separates
    them without naming any of them, at the cost of a directory referred to
    without its slash.
    """
    tail = token.rsplit("/", 1)[-1]
    return token.endswith("/") or ("." in tail and tail not in (".", ".."))


def paths_in(relative, line, verbatim, roots):
    """Every token on this line that is a reference to a path in this repository.

    An anchor or a query names a place within the target rather than a different
    target, so both are cut before the path is resolved.
    """
    for text in candidates(relative, line, verbatim):
        for word in words(text):
            token = word.split("#")[0].split("?")[0]
            if "/" not in token or not names_a_file(token):
                continue
            if token.split("/")[0] in roots or token.startswith((".", "..")):
                yield token


def path_targets(relative, token):
    """What a path token could mean, as repo-relative paths.

    Both spellings are used and both are correct: from the repository root, and
    from the directory of the file that names it. Anything that normalises to
    outside the repository is not a reference into it and drops out here.
    """
    directory = os.path.dirname(relative)
    spellings = {os.path.normpath(token), os.path.normpath(os.path.join(directory, token))}
    return {p for p in spellings if not p.startswith("..") and not os.path.isabs(p)}


def path_resolves(relative, token):
    return any(os.path.exists(os.path.join(ROOT, p)) for p in path_targets(relative, token))


def path_artefacts(found):
    """The references naming something the build produces rather than something
    the repository holds. Asked of git in one call, since a reference costs
    nothing to extract and a process costs a great deal to start."""
    artefacts = ignored({target for reference in found for target in path_targets(*reference)})
    return {
        reference
        for reference in found
        if any(target in artefacts for target in path_targets(*reference))
    }


class Kind:
    """One class of reference: how to find it, how to resolve it, what to say.

    `unassertable` is given every reference of this kind at once and returns
    those that cannot be asserted at all, as against those that fail — a batch
    so that a check needing to ask git something asks it once.
    """

    def __init__(self, extract, resolves, complaint, unassertable):
        self.extract = extract
        self.resolves = resolves
        self.complaint = complaint
        self.unassertable = unassertable


# The kinds of reference asserted. #740 adds the SolidSyslog symbol a page names
# as a second row here, reusing the pass above and the exception list below.
KINDS = (
    Kind(
        extract=paths_in,
        resolves=path_resolves,
        complaint="names a path that does not exist",
        unassertable=path_artefacts,
    ),
)


def references(kind):
    """Every (file, token) of one kind, with the line each was found on."""
    roots = tracked_roots()
    found = {}
    for relative in scanned():
        verbatim = not relative.endswith(".md")
        block = None
        for number, line in enumerate(read(relative).splitlines(), 1):
            if relative.endswith(".md"):
                if FENCE.match(line):
                    verbatim = not verbatim
                    continue
            else:
                block = inside_block(line, block)
                if block is not None or SHELL.match(line):
                    continue
            for token in kind.extract(relative, line, verbatim, roots):
                found.setdefault((relative, token), number)
    return found


def inside_block(line, block):
    """The indent of the block scalar this line sits in, or None. A block ends
    at the first non-blank line indented no further than the key that opened
    it."""
    if block is not None:
        if not line.strip() or len(line) - len(line.lstrip()) > block:
            return block
    opened = BLOCK_SCALAR.match(line)
    return len(opened.group(1)) if opened else None


def check():
    # Everything below is asked of git, so a run outside a working tree would
    # find no files, assert nothing, and pass. Fail loudly instead.
    if not scanned():
        sys.exit(f"no documents or build files found under {ROOT} — is this a git checkout?")

    exempt = {(path, token) for path, token, _ in ALLOWED}
    faults = []
    for kind in KINDS:
        found = {r: n for r, n in references(kind).items() if r not in exempt}
        assertable = set(found) - kind.unassertable(set(found))
        faults += [
            f"{relative}:{found[(relative, token)]} {kind.complaint}: {token}"
            for relative, token in sorted(assertable)
            if not kind.resolves(relative, token)
        ]
    return sorted(faults)


if __name__ == "__main__":
    problems = check()
    for problem in problems:
        print(f"error: {problem}", file=sys.stderr)
    if problems:
        print(
            f"\n{len(problems)} problem(s). Fix the reference, or — if it is "
            "deliberately unresolvable — add it to ALLOWED with the reason.",
            file=sys.stderr,
        )
        sys.exit(1)
    for path, token, reason in ALLOWED:
        print(f"allowed: {path} may name {token} — {reason}")
    print(f"every path named by {len(scanned())} documents and build files exists")
