#!/usr/bin/env python3
"""Rejuggle stale line numbers in misra_suppressions.txt.

Production-code edits that shift line numbers (typical: an include reorder
from IWYU, a refactor that adds a helper) leave entries in
misra_suppressions.txt pointing at the wrong source line. The cppcheck-misra
gate then fails on the same finding it has always passed — just at a new
line — and the human has to re-do clerical work.

This script automates the clerical bit. It runs the CI's cppcheck-misra
invocation and, for each (rule, file) pair where misra_suppressions.txt
has stale lines and the report shows the same finding at different lines,
proposes (or applies) the renumbering.

Usage:
    scripts/misra_renumber.py             # show proposed updates
    scripts/misra_renumber.py --apply     # write back updated suppressions

Exit code:
    0  no actionable updates
    1  proposed updates exist (default 'check' mode) — re-run with --apply
    2  ambiguous mismatch needing human attention

Algorithm (two passes):
    Pass 1 — run cppcheck WITHOUT --suppressions-list and audit. For each
    (rule, file) pair where the suppression entries match the findings
    in count but not in lines, remap ascending-by-ascending. This catches
    bulk line drifts from refactors that touch many sites in one file.

    Pass 2 — run cppcheck WITH --suppressions-list to see which
    suppressions are still failing. cppcheck dedupes findings per
    location: if 11.2 and 11.3 both fire on the same line and the 11.2
    suppression is correct (or absent), the 11.3 finding gets shadowed in
    pass 1's view. Pass 2's residual failures resolve those cases — for
    each failure (rule, file, line) we look for a suppression with the
    same (rule, file) and update its line if there's exactly one match.

Conservatism:
    - Pass 1: only renumber when the (rule, file) pair has the same
      NUMBER of suppression entries as findings. Mismatched counts mean
      a real finding was added or removed; the script bails on that
      pair so a human looks at it.
    - Pass 2: only renumber when the (rule, file) pair has exactly one
      suppression entry. Multiple-entry cases (e.g. several casts in one
      file all needing different new lines) need pass 1 to handle them.
    - The cppcheck command list must match .github/workflows/ci.yml's
      cppcheck-misra step. If the CI lane changes its arguments, this
      script needs the same update — flagged as a deviation from
      single-source-of-truth (acceptable for a one-off helper).
"""

import argparse
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SUPPRESSIONS = REPO_ROOT / "misra_suppressions.txt"

# Mirror of .github/workflows/ci.yml `Run cppcheck-misra` step minus the
# --suppressions-list flag (we want to see every finding, including the
# ones a current suppression is silencing).
CPPCHECK_CMD = [
    "cppcheck",
    "--addon=misra",
    "--inline-suppr",
    "--std=c11",
    "-ICore/Interface",
    "-IPlatform/Atomics/Interface",
    "-IPlatform/Posix/Interface",
    "-IPlatform/Windows/Interface",
    "-IPlatform/OpenSsl/Interface",
    "-IPlatform/MbedTls/Interface",
    "-IPlatform/FreeRtos/Interface",
    "-IPlatform/PlusTcp/Interface",
    "-IPlatform/FatFs/Interface",
    "--xml",
    "--xml-version=2",
    "Core/Source/",
    "Platform/Atomics/Source/",
    "Platform/Posix/Source/",
    "Platform/Windows/Source/",
    "Platform/OpenSsl/Source/",
    "Platform/MbedTls/Source/",
    "Platform/FreeRtos/Source/",
    "Platform/PlusTcp/Source/",
    "Platform/FatFs/Source/",
]

SUPPRESSION_LINE = re.compile(r"^(misra-c2012-[0-9.]+):([^:]+):(\d+)\s*$")


def run_cppcheck(with_suppressions=False):
    """Run cppcheck-misra and return XML stderr as a string."""
    cmd = list(CPPCHECK_CMD)
    if with_suppressions:
        cmd.insert(1, f"--suppressions-list={SUPPRESSIONS}")
    proc = subprocess.run(
        cmd, cwd=REPO_ROOT, capture_output=True, text=True, check=False
    )
    return proc.stderr


def findings_by_rule_file(xml_text):
    """Parse cppcheck XML; return dict (rule, file) -> sorted list of lines."""
    root = ET.fromstring(xml_text)
    by_pair = defaultdict(list)
    for err in root.findall(".//error"):
        rule = err.get("id", "")
        if not rule.startswith("misra-c2012-"):
            continue
        loc = err.find("location")
        if loc is None:
            continue
        file_path = loc.get("file", "")
        line = int(loc.get("line", "0"))
        by_pair[(rule, file_path)].append(line)
    return {pair: sorted(lines) for pair, lines in by_pair.items()}


def parse_suppressions():
    """Return list of (raw_line, rule, file, line_or_None). Comments/blanks
    keep line_or_None=None and are passed through untouched on rewrite."""
    parsed = []
    for raw in SUPPRESSIONS.read_text().splitlines(keepends=True):
        stripped = raw.strip()
        if not stripped or stripped.startswith("#"):
            parsed.append((raw, None, None, None))
            continue
        match = SUPPRESSION_LINE.match(stripped)
        if match:
            rule, path, line = match.group(1), match.group(2), int(match.group(3))
            parsed.append((raw, rule, path, line))
        else:
            parsed.append((raw, None, None, None))
    return parsed


def propose_updates(parsed, findings):
    """For each (rule, file) group, if counts match but line sets differ,
    propose remapping ascending-by-ascending. Returns dict
    (index_in_parsed -> new_line_number) and a list of ambiguous pairs."""
    suppressions_by_pair = defaultdict(list)  # (rule, file) -> [index_in_parsed]
    for i, (_, rule, path, _) in enumerate(parsed):
        if rule is not None:
            suppressions_by_pair[(rule, path)].append(i)

    updates = {}
    ambiguous = []
    for pair, indices in suppressions_by_pair.items():
        old_lines = sorted(parsed[i][3] for i in indices)
        new_lines = findings.get(pair, [])
        if old_lines == new_lines:
            continue
        if len(old_lines) != len(new_lines):
            ambiguous.append((pair, old_lines, new_lines))
            continue
        # Same count, different lines: map ascending-by-ascending.
        indices_sorted = sorted(indices, key=lambda i: parsed[i][3])
        for i, new_line in zip(indices_sorted, new_lines):
            old_line = parsed[i][3]
            if new_line != old_line:
                updates[i] = new_line
    return updates, ambiguous


def apply_updates(parsed, updates):
    """Rewrite misra_suppressions.txt with the proposed updates applied."""
    lines = []
    for i, (raw, rule, path, old_line) in enumerate(parsed):
        if i in updates:
            new_line = updates[i]
            replacement = f"{rule}:{path}:{new_line}"
            # Preserve trailing whitespace / newline from the original line.
            trailing = raw[raw.rstrip().__len__():]
            lines.append(replacement + trailing)
        else:
            lines.append(raw)
    SUPPRESSIONS.write_text("".join(lines))


def propose_pass2_updates(parsed, residual):
    """Pass 2: for each residual (rule, file, line) failure, find the lone
    suppression entry with the same (rule, file) and propose updating its line.
    Skips (rule, file) pairs that have multiple suppression entries."""
    suppressions_by_pair = defaultdict(list)
    for i, (_, rule, path, _) in enumerate(parsed):
        if rule is not None:
            suppressions_by_pair[(rule, path)].append(i)

    updates = {}
    ambiguous = []
    for pair, new_lines in residual.items():
        indices = suppressions_by_pair.get(pair, [])
        if len(indices) == 0:
            ambiguous.append((pair, [], new_lines))
            continue
        if len(indices) != 1 or len(new_lines) != 1:
            ambiguous.append(
                (pair, sorted(parsed[i][3] for i in indices), new_lines)
            )
            continue
        i = indices[0]
        if parsed[i][3] != new_lines[0]:
            updates[i] = new_lines[0]
    return updates, ambiguous


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--apply", action="store_true", help="Write the proposed updates back."
    )
    args = parser.parse_args()

    # Pass 1: full audit against unsuppressed findings.
    findings = findings_by_rule_file(run_cppcheck(with_suppressions=False))
    parsed = parse_suppressions()
    updates, ambiguous = propose_updates(parsed, findings)

    # Pass 2: residual failures with current suppressions applied — handles
    # cppcheck's per-location dedup that hides one rule behind another.
    if args.apply and updates:
        apply_updates(parsed, updates)
        parsed = parse_suppressions()
    residual = findings_by_rule_file(run_cppcheck(with_suppressions=True))
    pass2_updates, pass2_ambiguous = propose_pass2_updates(parsed, residual)

    for pair, old, new in ambiguous + pass2_ambiguous:
        rule, path = pair
        print(f"AMBIGUOUS  {rule}:{path}  old={old}  new={new}", file=sys.stderr)

    all_updates = list(sorted(updates.items()))
    if args.apply:
        if updates:
            print(f"Applied {len(updates)} pass-1 update(s).")
        all_updates = list(sorted(pass2_updates.items()))

    for i, new_line in all_updates:
        _, rule, path, old_line = parsed[i]
        print(f"  {rule}:{path}:{old_line} -> {new_line}")

    if args.apply:
        if pass2_updates:
            apply_updates(parsed, pass2_updates)
            print(f"Applied {len(pass2_updates)} pass-2 update(s) to "
                  f"{SUPPRESSIONS.relative_to(REPO_ROOT)}.")
        return 0 if not (ambiguous or pass2_ambiguous) else 2

    total = len(updates) + len(pass2_updates)
    if total == 0:
        if ambiguous or pass2_ambiguous:
            print("No simple renumbers proposed; ambiguous pairs need human review.")
            return 2
        print("No stale suppression lines found.")
        return 0

    print(f"\nRun with --apply to write {total} update(s).")
    return 1 if not (ambiguous or pass2_ambiguous) else 2


if __name__ == "__main__":
    sys.exit(main())
