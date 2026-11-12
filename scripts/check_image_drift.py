#!/usr/bin/env python3
"""Fail when an image pin lags its repository's main with no bump pull request to carry it.

Every push to an image repository's main publishes sha-<short> of that commit
and sends the event Image Bump answers with a pull request. A publish that
fails after a green pull request sends nothing, so the pin here stays behind
in silence; this is the check that notices. image-drift.yml runs it weekly.

Run:  python3 scripts/check_image_drift.py --repositories
      python3 scripts/check_image_drift.py --head <repository>=<sha>... [--open-branch <name>...]
"""

import argparse
import os
import re
import sys

CONTAINERS_DOC = "docs/containers.md"

# The images whose repositories send image-published, and those repositories.
SOURCES = {
    "ghcr.io/cososo-ltd/cpputest": "cososo-ltd/CppUTestDocker",
    "ghcr.io/cososo-ltd/cpputest-freertos": "cososo-ltd/CppUTestFreertosDocker",
    "ghcr.io/cososo-ltd/cpputest-freertos-cross": "cososo-ltd/CppUTestFreertosDocker",
}

BUMP_BRANCH_PREFIX = "chore/bump-image-"


def check(table_text, heads, open_branches):
    problems = []
    for image, repository in SOURCES.items():
        pinned = _pinned_tag(table_text, image)
        if pinned is None:
            problems.append(image + " has no row in " + CONTAINERS_DOC)
        elif repository not in heads:
            problems.append("no main head given for " + repository)
        else:
            published = "sha-" + heads[repository][:7]
            if pinned != published and not _is_carried(published, open_branches):
                problems.append(
                    image + " is pinned at " + pinned + ", but " + repository + " main publishes "
                    + published + " and no open bump pull request carries it"
                )
    return problems


def _pinned_tag(table_text, image):
    match = re.search(r"^\| `" + re.escape(image) + r"` \| `([^`]*)`", table_text, re.MULTILINE)
    return None if match is None else match.group(1)


def _is_carried(tag, open_branches):
    return any(branch.startswith(BUMP_BRANCH_PREFIX + tag + "-") for branch in open_branches)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--repositories", action="store_true", help="print the repositories whose main heads are needed, and stop")
    parser.add_argument("--head", action="append", default=[], metavar="REPOSITORY=SHA", help="a repository's main head commit")
    parser.add_argument("--open-branch", action="append", default=[], metavar="NAME", help="the head branch of an open pull request here")
    parser.add_argument("--root", default=".", help="the repository root (default: the current directory)")
    args = parser.parse_args(argv)
    status = 0
    if args.repositories:
        for repository in sorted(set(SOURCES.values())):
            print(repository)
    else:
        heads = dict(pair.split("=", 1) for pair in args.head)
        with open(os.path.join(args.root, CONTAINERS_DOC), encoding="utf-8") as f:
            table_text = f.read()
        problems = check(table_text, heads, args.open_branch)
        for problem in problems:
            print("check_image_drift: " + problem, file=sys.stderr)
        if problems:
            status = 1
        else:
            for image in SOURCES:
                print(image + " " + _pinned_tag(table_text, image) + ": current, or carried by an open bump pull request")
    return status


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
