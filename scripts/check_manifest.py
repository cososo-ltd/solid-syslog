#!/usr/bin/env python3
"""Assert the generated integration manifest lists every source it should.

The manifest is what a non-CMake (Path B) integrator compiles. It is generated
from the build targets, so it cannot drift from what a target *carries* — but it
can still drift from what is *on disk*, because a source no target lists is
invisible to both the manifest and the drift check.

The check is a set comparison, per section, against the filesystem:

    manifest's "# Core (always required):" list  ==  Core/Source/*.c
    manifest's "# <Platform>:" list              ==  <directory>/Source/*.c

Token to directory comes from SOLIDSYSLOG_PLATFORM_REGISTRY in the top-level
CMakeLists.txt, so a platform added there is checked here without touching this
script. Platforms are independent, so a per-platform check needs no combinations
enumerated.

Usage:
    scripts/check_manifest.py <manifest> [--root <repo root>]

Exit code:
    0  every section matches the tree
    1  a section is missing, or its file list differs from the directory
"""

import argparse
import pathlib
import re
import sys

# `token|option|default|kind|directory|roles` — see CMakeLists.txt.
REGISTRY_BLOCK = re.compile(
    r"set\(SOLIDSYSLOG_PLATFORM_REGISTRY(.*?)^\)", re.DOTALL | re.MULTILINE
)
REGISTRY_ROW = re.compile(r'"([^"|]+)\|[^"|]*\|[^"|]*\|[^"|]*\|([^"|]+)\|[^"]*"')

# A section header is exactly `# <Token>:` — the prose lines the generator emits
# around the probe-platform block are comments too, and must not open a section.
SECTION_HEADER = re.compile(r"^# ([A-Za-z0-9]+):$")
CORE_HEADER = "# Core (always required):"
SELECTED_HEADER = re.compile(r"^# Selected platforms: (.*)$")
SCOPE_HEADER = re.compile(r"^# Scope: (\w+)$")

CORE_SECTION = "Core"
CORE_SOURCE_DIR = "Core/Source"


def read_registry(root):
    """Map each platform token to its directory, from the CMake registry."""
    text = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    block = REGISTRY_BLOCK.search(text)
    if not block:
        sys.exit("SOLIDSYSLOG_PLATFORM_REGISTRY not found in CMakeLists.txt")
    return {
        token: directory for token, directory in REGISTRY_ROW.findall(block.group(1))
    }


def read_manifest(path):
    """Return (scope, selected tokens, {section: .c paths}, orphaned .c paths).

    A source line under no section header is orphaned. Reporting those rather
    than dropping them is what stops a malformed manifest passing by having
    nothing left to compare.
    """
    scope = "all"
    selected = []
    sections = {}
    orphans = set()
    current = None
    in_sources = False

    for line in path.read_text(encoding="utf-8").splitlines():
        header = SCOPE_HEADER.match(line)
        if header:
            scope = header.group(1)
        header = SELECTED_HEADER.match(line)
        if header:
            selected = [
                token.strip()
                for token in header.group(1).split(",")
                if token.strip() and not token.startswith("(")
            ]
        if line.startswith("## "):
            in_sources = line.startswith("## Source files")
            current = None
        elif not in_sources:
            continue
        elif line == CORE_HEADER:
            current = CORE_SECTION
            sections.setdefault(current, set())
        elif SECTION_HEADER.match(line):
            current = SECTION_HEADER.match(line).group(1)
            sections.setdefault(current, set())
        elif line.startswith("#"):
            current = None
        elif line.endswith(".c"):
            if current:
                sections[current].add(line)
            else:
                orphans.add(line)

    return scope, selected, sections, orphans


def sources_on_disk(root, directory):
    return {
        str(path.relative_to(root)).replace("\\", "/")
        for path in sorted((root / directory).glob("*.c"))
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=pathlib.Path, nargs="?")
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument(
        "--list-platforms",
        action="store_true",
        help="print the registry's platform tokens, one per line, and exit",
    )
    arguments = parser.parse_args()

    root = arguments.root.resolve()
    registry = read_registry(root)

    if arguments.list_platforms:
        print("\n".join(sorted(registry)))
        return 0
    if arguments.manifest is None:
        parser.error("a manifest path is required unless --list-platforms is given")
    scope, selected, sections, orphans = read_manifest(arguments.manifest)

    problems = []

    if scope == "platform":
        if CORE_SECTION in sections:
            problems.append(
                f"{CORE_SECTION}: a platform-scoped manifest must not carry the "
                f"Core sources — they belong in the core manifest"
            )
        if not sections:
            problems.append("no platform section — this manifest describes nothing")
    elif CORE_SECTION not in sections:
        problems.append(
            f"{CORE_SECTION}: no '{CORE_HEADER}' section — a {scope}-scoped "
            f"manifest carries the Core sources"
        )

    for path in sorted(orphans):
        problems.append(f"{path} is listed under no section header")

    for token in selected:
        if token not in sections:
            problems.append(
                f"{token}: named in 'Selected platforms' but has no section — "
                f"the whole platform is missing from the manifest"
            )

    for section, listed in sorted(sections.items()):
        if section == CORE_SECTION:
            directory = CORE_SOURCE_DIR
        elif section in registry:
            directory = f"{registry[section]}/Source"
        else:
            problems.append(
                f"{section}: not a token in SOLIDSYSLOG_PLATFORM_REGISTRY "
                f"(valid: {', '.join(sorted(registry))})"
            )
            continue

        on_disk = sources_on_disk(root, directory)
        for path in sorted(on_disk - listed):
            problems.append(f"{section}: {path} is on disk but not in the manifest")
        for path in sorted(listed - on_disk):
            problems.append(f"{section}: {path} is in the manifest but not on disk")

    if problems:
        print(f"{arguments.manifest} does not match the tree:", file=sys.stderr)
        for problem in problems:
            print(f"  {problem}", file=sys.stderr)
        return 1

    checked = ", ".join(sorted(sections))
    plural = "section" if len(sections) == 1 else "sections"
    print(
        f"{arguments.manifest}: {len(sections)} {plural} match the tree ({checked})"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
