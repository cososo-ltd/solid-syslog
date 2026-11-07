#!/usr/bin/env python3
"""Summarise an lcov tracefile per pack, and gate it.

Reads the filtered tracefile the `coverage` target writes and prints a Markdown
table with one row for Core, one per Platform/<Pack>, and a total. The gate is
applied to the total: lines and branches must each reach the threshold, and a
tracefile holding no branch data fails rather than passing by default, because
that is exactly how a wrong lcov switch went unnoticed.

Usage:
    scripts/coverage_summary.py <tracefile> [--threshold <percent>]

Exit code:
    0  both rates meet the threshold
    1  a rate falls short, or the tracefile has no line or no branch data
"""

import argparse
import re
import sys
from typing import Dict, List, NamedTuple, TextIO

PACK_PATTERN = re.compile(r"Platform/([A-Za-z0-9]+)/")


class Counts(NamedTuple):
    lines_found: int
    lines_hit: int
    branches_found: int
    branches_hit: int

    def add(self, other: "Counts") -> "Counts":
        return Counts(*(a + b for a, b in zip(self, other)))


def pack_of(source_path: str) -> str:
    match = PACK_PATTERN.search(source_path.replace("\\", "/"))
    return match.group(1) if match else "Core"


def aggregate(tracefile: TextIO) -> Dict[str, Counts]:
    packs: Dict[str, Counts] = {}
    pack = "Core"
    for line in tracefile:
        key, _, value = line.strip().partition(":")
        if key == "SF":
            pack = pack_of(value)
            packs.setdefault(pack, Counts(0, 0, 0, 0))
        elif key in ("LF", "LH", "BRF", "BRH"):
            field = {"LF": 0, "LH": 1, "BRF": 2, "BRH": 3}[key]
            delta = [0, 0, 0, 0]
            delta[field] = int(value)
            packs[pack] = packs[pack].add(Counts(*delta))
    return packs


def total(packs: Dict[str, Counts]) -> Counts:
    result = Counts(0, 0, 0, 0)
    for counts in packs.values():
        result = result.add(counts)
    return result


def rate(hit: int, found: int) -> float:
    return 100.0 * hit / found if found else 0.0


def failures(counts: Counts, threshold: float) -> List[str]:
    found = []
    if counts.lines_found == 0:
        found.append("no line data in the tracefile")
    elif rate(counts.lines_hit, counts.lines_found) < threshold:
        found.append(f"line coverage {rate(counts.lines_hit, counts.lines_found):.1f}% is below {threshold:g}%")
    if counts.branches_found == 0:
        found.append("no branch data in the tracefile")
    elif rate(counts.branches_hit, counts.branches_found) < threshold:
        found.append(
            f"branch coverage {rate(counts.branches_hit, counts.branches_found):.1f}% is below {threshold:g}%"
        )
    return found


def row(name: str, counts: Counts) -> str:
    return (
        f"| {name} | {counts.lines_hit} / {counts.lines_found} | {rate(counts.lines_hit, counts.lines_found):.1f}% "
        f"| {counts.branches_hit} / {counts.branches_found} | {rate(counts.branches_hit, counts.branches_found):.1f}% |"
    )


def markdown(packs: Dict[str, Counts]) -> str:
    lines = ["| Pack | Lines | | Branches | |", "|---|---:|---:|---:|---:|"]
    ordered = ["Core"] + sorted(name for name in packs if name != "Core")
    lines.extend(row(name, packs[name]) for name in ordered if name in packs)
    lines.append(row("**Total**", total(packs)))
    return "\n".join(lines)


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("tracefile")
    parser.add_argument("--threshold", type=float, default=90.0, help="minimum percent for lines and for branches")
    args = parser.parse_args(argv)

    with open(args.tracefile, encoding="utf-8") as handle:
        packs = aggregate(handle)

    print(markdown(packs))
    problems = failures(total(packs), args.threshold)
    for problem in problems:
        print(f"FAIL: {problem}", file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
