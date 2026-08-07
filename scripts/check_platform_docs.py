#!/usr/bin/env python3
"""Assert every registered platform is documented, and nothing else claims to be.

A platform is declared once, in SOLIDSYSLOG_PLATFORM_REGISTRY in the top-level
CMakeLists.txt. Everything else follows from its token:

    token       LwipRaw
    directory   Platform/LwipRaw/
    docs        docs/platforms/lwipraw/{index,setup}.md
    group       @defgroup platform_lwipraw
    nav         an entry in mkdocs.yml
    description an entry in hooks/page_descriptions.py

This checks that each of those exists for each row, and the reverse — a docs
folder or a group with no row behind it. Registering a platform is then the one
edit that cannot be forgotten, because forgetting anything else fails the build.

Run:  python3 scripts/check_platform_docs.py
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

REGISTRY = re.compile(r"set\(SOLIDSYSLOG_PLATFORM_REGISTRY(.*?)^\)", re.DOTALL | re.MULTILINE)
ROW = re.compile(r'"([^"|]+)\|[^"|]*\|[^"|]*\|[^"|]*\|([^"|]+)\|[^"]*"')


def read(*parts):
    with open(os.path.join(ROOT, *parts), encoding="utf-8") as handle:
        return handle.read()


def registered():
    """[(token, directory)] from the registry, which is the single declaration."""
    found = REGISTRY.search(read("CMakeLists.txt"))
    if found is None:
        sys.exit("SOLIDSYSLOG_PLATFORM_REGISTRY not found in CMakeLists.txt")
    return ROW.findall(found.group(1))


def documented():
    """Slugs that have a docs folder, whether or not anything registered them."""
    platforms = os.path.join(ROOT, "docs", "platforms")
    return {
        name
        for name in os.listdir(platforms)
        if os.path.isdir(os.path.join(platforms, name))
    }


def check():
    faults = []
    nav = read("mkdocs.yml")
    descriptions = read("hooks", "page_descriptions.py")
    matrix = read("docs", "platforms", "index.md")
    slugs = set()

    for token, directory in registered():
        slug = token.lower()
        slugs.add(slug)
        docs = os.path.join("docs", "platforms", slug)

        for page in ("index.md", "setup.md"):
            if not os.path.isfile(os.path.join(ROOT, docs, page)):
                faults.append(f"{token}: {docs}/{page} is missing")

        group = os.path.join(ROOT, directory, f"SolidSyslog{token}Platform.dox")
        if not os.path.isfile(group):
            faults.append(f"{token}: no group file at {directory}/SolidSyslog{token}Platform.dox")
        elif f"@defgroup platform_{slug} " not in read(group):
            faults.append(f"{token}: its group file does not declare @defgroup platform_{slug}")

        if f"platforms/{slug}/index.md" not in nav:
            faults.append(f"{token}: no entry in the mkdocs.yml nav")
        for page in ("index.md", "setup.md"):
            if f'"platforms/{slug}/{page}"' not in descriptions:
                faults.append(f"{token}: no meta description for platforms/{slug}/{page}")

        if f"]({slug}/index.md)" not in matrix:
            faults.append(f"{token}: not a row in the docs/platforms/index.md matrix")

        interface = os.path.join(ROOT, directory, "Interface")
        for header in sorted(os.listdir(interface)) if os.path.isdir(interface) else []:
            if header.endswith(".h") and "@ingroup" not in read(directory, "Interface", header):
                faults.append(f"{token}: {header} carries no @ingroup")

    for slug in sorted(documented() - slugs):
        faults.append(f"docs/platforms/{slug}/ documents a platform that is not registered")

    return faults


if __name__ == "__main__":
    problems = check()
    for problem in problems:
        print(f"error: {problem}", file=sys.stderr)
    if problems:
        print(
            f"\n{len(problems)} problem(s). A platform is declared in "
            "SOLIDSYSLOG_PLATFORM_REGISTRY; everything above follows from its token.",
            file=sys.stderr,
        )
        sys.exit(1)
    print(f"platform docs: {len(registered())} platforms, all documented")
