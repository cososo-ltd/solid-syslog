#!/usr/bin/env python3
"""Summarise an lcov tracefile per pack, and gate it.

Reads the filtered tracefile the `coverage` target writes and prints a Markdown
table with one row for Core, one per Platform/<Pack>, and a total. The gate is
applied to the total: lines and branches must each reach the threshold, and a
tracefile holding no branch data fails rather than passing by default, because
that is exactly how a wrong lcov switch went unnoticed.

With --llvm-json, the JSON summary `llvm-cov export -summary-only` writes adds
each file's MC/DC conditions to its pack, the table gains the MC/DC columns, and
the gate applies to that rate too. A summary with no conditions fails the same
way a tracefile with no branches does.

Usage:
    scripts/coverage_summary.py <tracefile> [--threshold <percent>] [--llvm-json <summary>]

Exit code:
    0  every rate meets the threshold
    1  a rate falls short, or a measurement the inputs promise is missing
"""

import argparse
import json
import re
import sys
from typing import Callable, Dict, List, NamedTuple, TextIO

PACK_PATTERN = re.compile(r"Platform/([A-Za-z0-9]+)/")
TOTAL = "**Total**"
COUNTER_FIELDS = {"LF": "lines_found", "LH": "lines_hit", "BRF": "branches_found", "BRH": "branches_hit"}


class Counts(NamedTuple):
    lines_found: int
    lines_hit: int
    branches_found: int
    branches_hit: int
    mcdc_found: int = 0
    mcdc_hit: int = 0

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
        elif key in COUNTER_FIELDS:
            field = COUNTER_FIELDS[key]
            current = packs[pack]
            packs[pack] = current._replace(**{field: getattr(current, field) + int(value)})
    return packs


def add_mcdc(packs: Dict[str, Counts], summary: TextIO) -> None:
    """Adds each file's MC/DC conditions from llvm-cov's JSON summary to its pack."""
    for export in json.load(summary)["data"]:
        for entry in export["files"]:
            pack = pack_of(entry["filename"])
            mcdc = entry["summary"]["mcdc"]
            current = packs.setdefault(pack, Counts(0, 0, 0, 0))
            packs[pack] = current._replace(
                mcdc_found=current.mcdc_found + int(mcdc["count"]), mcdc_hit=current.mcdc_hit + int(mcdc["covered"])
            )


def total(packs: Dict[str, Counts]) -> Counts:
    result = Counts(0, 0, 0, 0)
    for counts in packs.values():
        result = result.add(counts)
    return result


def rate(hit: int, found: int) -> float:
    return 100.0 * hit / found if found else 0.0


def percent(hit: int, found: int) -> str:
    # Truncated, not rounded, so a rate just short of the threshold is never
    # shown as the threshold itself.
    tenths = 1000 * hit // found if found else 0
    return f"{tenths // 10}.{tenths % 10}%"


def cells(hit: int, found: int) -> str:
    """The count and the rate; a pack with nothing to measure shows a dash."""
    return f"| {hit} / {found} | {percent(hit, found) if found else '-'} "


def failures(counts: Counts, threshold: float) -> List[str]:
    found = []
    if counts.lines_found == 0:
        found.append("no line data in the tracefile")
    elif rate(counts.lines_hit, counts.lines_found) < threshold:
        found.append(f"line coverage {percent(counts.lines_hit, counts.lines_found)} is below {threshold:g}%")
    if counts.branches_found == 0:
        found.append("no branch data in the tracefile")
    elif rate(counts.branches_hit, counts.branches_found) < threshold:
        found.append(f"branch coverage {percent(counts.branches_hit, counts.branches_found)} is below {threshold:g}%")
    if counts.mcdc_found and rate(counts.mcdc_hit, counts.mcdc_found) < threshold:
        found.append(f"MC/DC coverage {percent(counts.mcdc_hit, counts.mcdc_found)} is below {threshold:g}%")
    return found


def row(name: str, counts: Counts, with_mcdc: bool) -> str:
    line = f"| {name} " + cells(counts.lines_hit, counts.lines_found) + cells(counts.branches_hit, counts.branches_found)
    if with_mcdc:
        line += cells(counts.mcdc_hit, counts.mcdc_found)
    return line + "|"


def markdown(packs: Dict[str, Counts], label: Callable[[str], str] = str) -> str:
    """The table, Core first and the packs alphabetical, then the total row.

    `label` renders each row's name cell from the pack name, or from TOTAL for
    the last row; the docs build passes one that wraps the name in a link.
    """
    whole = total(packs)
    with_mcdc = whole.mcdc_found > 0
    header = "| Pack | Lines | | Branches | |" + (" MC/DC | |" if with_mcdc else "")
    rule = "|---|---:|---:|---:|---:|" + ("---:|---:|" if with_mcdc else "")
    lines = [header, rule]
    ordered = ["Core"] + sorted(name for name in packs if name != "Core")
    lines.extend(row(label(name), packs[name], with_mcdc) for name in ordered if name in packs)
    lines.append(row(label(TOTAL), whole, with_mcdc))
    return "\n".join(lines)


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("tracefile")
    parser.add_argument("--threshold", type=float, default=99.0, help="minimum percent for each rate")
    parser.add_argument("--llvm-json", help="llvm-cov export -summary-only JSON, for the MC/DC columns and gate")
    args = parser.parse_args(argv)

    with open(args.tracefile, encoding="utf-8") as handle:
        packs = aggregate(handle)
    if args.llvm_json:
        with open(args.llvm_json, encoding="utf-8") as handle:
            add_mcdc(packs, handle)

    print(markdown(packs))
    problems = failures(total(packs), args.threshold)
    if args.llvm_json and total(packs).mcdc_found == 0:
        problems.append("no MC/DC data in the llvm-cov summary")
    if problems:
        # A blank line ends the table, so a reader merging the two streams
        # gets the verdict beneath it rather than as another row; the flush
        # puts the table first when stdout is a pipe.
        print(flush=True)
    for problem in problems:
        print(f"FAIL: {problem}", file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
