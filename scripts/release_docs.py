#!/usr/bin/env python3
"""Bring the documentation to a release from the example stacks' sweeps.

Run:  python3 scripts/release_docs.py <version> <cmake-sweep> <make-sweep>
          [--previous <sweep>] [--check]
      python3 scripts/release_docs.py --stages <sweep> --previous <sweep>

Each sweep is the output of an example stack's Sweep run, as its artifact holds
it. The cost figures in README.md and docs/hardening-path.md, the links to the
stacks' release branches and the GIT_TAG are rewritten for <version>. --check
rewrites nothing and fails where any of them would change.

The figures are the CMake stack's. The Make stack builds the same integration
from its own copy of the upstreams and measures a few bytes apart, so where it
rounds to another figure that is printed and the CMake stack's is still used.

The stage costs in docs/hardening-path.md are rounded and sit in prose, so they
are not rewritten. With --previous, the output of the sweep the last release was
measured by, each stage whose cost moved is printed for re-taking by hand.

--stages prints those stages for one sweep and reads no documentation, which is
how the stages to re-take are known between releases.
"""

import argparse
import csv
import os
import re
import sys

FIRST_RECORD = "udp"
README = "README.md"
HARDENING_PATH = "docs/hardening-path.md"
BUILD_INTEGRATION = "docs/build-integration.md"
STACKS = ("solid-syslog-example", "solid-syslog-example-make")

# Flash is the read-only image plus the initialisers copied out of it; RAM is
# .data plus .bss. The stacks' own figures.py derives them the same way.
FLASH = ("flash_text", "flash_data")
RAM = ("flash_data", "static_bss")

COSTS_HEADING = "## What it costs\n"
KILOBYTES = re.compile(r"\d+\.\d KB")
PATH_COST = re.compile(r"(costs about )\d+\.\d( KB of flash and\s+)\d+\.\d( KB of RAM)")
RELEASE_LINK = re.compile(r"(https://github\.com/cososo-ltd/(solid-syslog-example(?:-make)?)/tree/release/)\d+\.\d+\.\d+")
GIT_TAG = re.compile(r"(GIT_TAG\s+)v\d+\.\d+\.\d+")


class ReleaseDocsError(Exception):
    """The sweeps or the tree are not in a state the documentation can be taken from."""


class Stage:
    def __init__(self, slug, flash, ram):
        self.slug = slug
        self.flash = flash
        self.ram = ram


def load(sweep):
    """The stages of a sweep in order, each with its cost above the Baseline."""
    with open(os.path.join(sweep, "stages.csv"), newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))
    above = {}
    with open(os.path.join(sweep, "figures.csv"), newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            above.setdefault(int(row["index"]), {})[row["key"]] = int(row["above_baseline"])
    stages = []
    for row in rows:
        index = int(row["index"])
        if not row["verdict"].startswith("PASS"):
            raise ReleaseDocsError(f"{sweep}: stage {index} {row['slug']} did not pass")
        figures = above.get(index, {})
        missing = [key for key in FLASH + RAM if key not in figures]
        if missing:
            raise ReleaseDocsError(f"{sweep}: stage {index} {row['slug']} has no {missing[0]}")
        stage = Stage(row["slug"], sum(figures[k] for k in FLASH), sum(figures[k] for k in RAM))
        # Every cost is counted from the Baseline's frozen figures, so a Baseline
        # that has since grown would be charged to every stage after it.
        if not stages and any(figures[k] for k in FLASH + RAM):
            raise ReleaseDocsError(
                f"{sweep}: the Baseline measures {stage.flash:+,} B flash and {stage.ram:+,} B RAM above its own "
                "measurements/Baseline.csv, which was captured from another Baseline"
            )
        stages.append(stage)
    if not stages:
        raise ReleaseDocsError(f"{sweep}: no stages")
    return stages


def kilobytes(octets):
    return f"{octets / 1000:.1f}"


def headline(stages, sweep):
    """The four figures the README states: the first record, then the whole path."""
    first = next((s for s in stages if s.slug == FIRST_RECORD), None)
    if first is None:
        raise ReleaseDocsError(f"{sweep}: no stage '{FIRST_RECORD}'")
    whole = stages[-1]
    return tuple(kilobytes(n) for n in (first.flash, first.ram, whole.flash, whole.ram))


def rewrite(root, version, figures):
    """The files that differ once brought to <version>, as {path: new text}."""
    texts = {relative: _read(root, relative) for relative in _pages(root)}
    brought = dict(texts)
    brought[README] = _readme_costs(brought[README], figures)
    brought[HARDENING_PATH] = _path_cost(brought[HARDENING_PATH], figures)
    brought[BUILD_INTEGRATION] = _git_tag(brought[BUILD_INTEGRATION], version)
    linked = set()
    for relative, text in brought.items():
        linked.update(match.group(2) for match in RELEASE_LINK.finditer(text))
        brought[relative] = RELEASE_LINK.sub(lambda match: match.group(1) + version, text)
    unlinked = [stack for stack in STACKS if stack not in linked]
    if unlinked:
        raise ReleaseDocsError(f"nothing links a release branch of {unlinked[0]}")
    return {relative: text for relative, text in brought.items() if text != texts[relative]}


def moved(now, previous):
    """One line per stage whose own cost differs from the last release's."""
    was = {stage.slug: step for stage, step in _steps(previous)}
    lines = []
    for number, (stage, step) in enumerate(_steps(now)):
        if number == 0:
            continue
        before = was.pop(stage.slug, None)
        if before is None:
            lines.append(f"Stage {number} {stage.slug}: new, flash {step[0]:+,} B, RAM {step[1]:+,} B")
        elif before != step:
            lines.append(
                f"Stage {number} {stage.slug}: flash {before[0]:+,} -> {step[0]:+,} B, RAM {before[1]:+,} -> {step[1]:+,} B"
            )
    was.pop(previous[0].slug, None)
    lines.extend(f"{slug}: no longer a stage" for slug in was)
    return lines


def _steps(stages):
    steps = []
    for index, stage in enumerate(stages):
        earlier = stages[index - 1] if index else Stage("", 0, 0)
        steps.append((stage, (stage.flash - earlier.flash, stage.ram - earlier.ram)))
    return steps


def _readme_costs(text, figures):
    start = text.find(COSTS_HEADING)
    if start < 0:
        raise ReleaseDocsError(f"{README} has no '{COSTS_HEADING.strip()}' section")
    start += len(COSTS_HEADING)
    end = text.find("\n\n", start + 1)
    paragraph = text[start:end]
    if len(KILOBYTES.findall(paragraph)) != len(figures):
        raise ReleaseDocsError(f"{README} states {len(KILOBYTES.findall(paragraph))} figures where {len(figures)} are measured")
    pending = iter(figures)
    return text[:start] + KILOBYTES.sub(lambda match: next(pending) + " KB", paragraph) + text[end:]


def _path_cost(text, figures):
    brought, count = PATH_COST.subn(lambda m: m.group(1) + figures[2] + m.group(2) + figures[3] + m.group(3), text)
    if count != 1:
        raise ReleaseDocsError(f"{HARDENING_PATH} states the cost of the path {count} times")
    return brought


def _git_tag(text, version):
    brought, count = GIT_TAG.subn(lambda match: match.group(1) + "v" + version, text)
    if count != 1:
        raise ReleaseDocsError(f"{BUILD_INTEGRATION} names {count} GIT_TAGs")
    return brought


def _pages(root):
    pages = [README]
    for directory, _, names in os.walk(os.path.join(root, "docs")):
        for name in sorted(names):
            if name.endswith(".md"):
                pages.append(os.path.relpath(os.path.join(directory, name), root).replace(os.sep, "/"))
    return sorted(pages)


def _read(root, relative):
    try:
        with open(os.path.join(root, relative), encoding="utf-8") as f:
            return f.read()
    except FileNotFoundError:
        raise ReleaseDocsError(f"{relative} is missing") from None


def _stages(args):
    try:
        if not args.previous:
            raise ReleaseDocsError("--stages needs --previous")
        lines = moved(load(args.stages), load(args.previous))
    except ReleaseDocsError as error:
        print(f"release_docs: {error}", file=sys.stderr)
        return 1
    print("\n".join(lines) if lines else "no stage's cost has moved")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("version", nargs="?")
    parser.add_argument("cmake", nargs="?", help="the output of solid-syslog-example's Sweep run")
    parser.add_argument("make", nargs="?", help="the output of solid-syslog-example-make's Sweep run")
    parser.add_argument("--stages", help="a sweep whose stages are compared with --previous, and nothing else done")
    parser.add_argument("--previous", help="the output of the sweep the last release was measured by")
    parser.add_argument("--check", action="store_true", help="rewrite nothing; fail where anything would change")
    parser.add_argument("--root", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    args = parser.parse_args(argv)
    if args.stages:
        return _stages(args)
    try:
        if args.make is None:
            raise ReleaseDocsError("a version and both stacks' sweeps are needed")
        if not re.fullmatch(r"\d+\.\d+\.\d+", args.version):
            raise ReleaseDocsError(f"'{args.version}' is not a version")
        figures = headline(load(args.cmake), args.cmake)
        make_figures = headline(load(args.make), args.make)
        changed = rewrite(args.root, args.version, figures)
        stages = moved(load(args.cmake), load(args.previous)) if args.previous else []
    except ReleaseDocsError as error:
        print(f"release_docs: {error}", file=sys.stderr)
        return 1
    print(f"first record {figures[0]} KB flash, {figures[1]} KB RAM; whole path {figures[2]} KB flash, {figures[3]} KB RAM")
    if make_figures != figures:
        print(f"the Make stack rounds to {' / '.join(make_figures)} KB")
    for relative in sorted(changed):
        print(f"{'differs' if args.check else 'rewritten'}: {relative}")
        if not args.check:
            with open(os.path.join(args.root, relative), "w", encoding="utf-8", newline="") as f:
                f.write(changed[relative])
    if stages:
        print(f"stages of {HARDENING_PATH} to re-take:")
        for line in stages:
            print(f"  {line}")
    return 1 if args.check and changed else 0


if __name__ == "__main__":
    sys.exit(main())
