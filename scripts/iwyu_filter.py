#!/usr/bin/env python3
"""Filter IWYU output to drop categories that don't apply to this project.

IWYU's analysis is enabled in CI as a ratchet against include-hygiene drift.
Three classes of 'should remove' finding are filtered out before the gate
evaluates, because each represents IWYU producing the wrong answer for this
codebase rather than a real drift:

1. '- struct <Name>;' — explicit forward declarations.
   In C, 'struct Foo* member;' inside another struct definition itself
   introduces 'struct Foo' at file scope, so IWYU considers preceding
   explicit 'struct Foo;' lines redundant. The project deliberately keeps
   them — clearer to a reader, more MISRA-aligned (Directive 4.8 still
   applies: forward-decl over full-include where a pointer is all that's
   used).

2. '- #include <stdbool.h>' — dual-language headers.
   Public Core headers expose 'bool' in C-callable function signatures and
   are consumed from both C TUs (production) and C++ TUs (CppUTest tests).
   IWYU sees the C++ side, where 'bool' is built-in, and concludes the
   include is unused. Removing it would break C consumers.

3. '- #include "CppUTest/TestHarness.h"' — test-framework macros.
   IWYU does not model how TEST_GROUP / TEST / CHECK_* macros expand.
   It thinks the include is unused because the symbols it provides are
   only invoked via macros. Removing it would break every test file.

Reads IWYU output (typically the stdout of iwyu_tool.py) on stdin, emits
filtered output on stdout. File blocks whose only findings are filtered
removals are suppressed entirely, so the report shows only actionable
findings.

Exit code:
  0 — no actionable findings remain after filtering
  1 — actionable findings remain (default 'check' behaviour)
"""

import re
import sys

FILTERED_REMOVALS = (
    re.compile(r"^- struct \w+;\s*//"),
    re.compile(r"^- #include <stdbool\.h>\s*//"),
    re.compile(r"^- #include \"CppUTest/TestHarness\.h\"\s*//"),
)


def _is_filtered_removal(line):
    return any(pat.match(line) for pat in FILTERED_REMOVALS)


def filter_iwyu(stream_in, stream_out):
    """Filter IWYU output; return count of files with actionable findings."""
    lines = stream_in.readlines()
    files_with_findings = 0
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        # 'should add these lines:' marks the start of a per-file block.
        # Block ends at the next '---' separator.
        if " should add these lines:" in line:
            block_end = i
            while block_end < n and not lines[block_end].startswith("---"):
                block_end += 1
            # Include the '---' line in the block.
            if block_end < n:
                block_end += 1

            block = lines[i:block_end]
            kept_block, has_finding = _filter_block(block)
            if has_finding:
                files_with_findings += 1
                stream_out.writelines(kept_block)
            i = block_end
        else:
            stream_out.write(line)
            i += 1

    return files_with_findings


def _filter_block(block):
    """Filter one per-file IWYU block; return (kept_lines, has_actionable_finding)."""
    add_lines = []
    remove_lines = []
    tail_lines = []

    section = "header"  # header | add | remove | tail
    for line in block:
        if " should add these lines:" in line:
            section = "add"
            header = line
            continue
        if " should remove these lines:" in line:
            section = "remove"
            continue
        if line.startswith("The full include-list for ") or line.startswith("---"):
            section = "tail"
            tail_lines.append(line)
            continue
        if section == "add":
            add_lines.append(line)
        elif section == "remove":
            remove_lines.append(line)
        elif section == "tail":
            tail_lines.append(line)

    add_real = [l for l in add_lines if l.strip()]
    remove_real = [
        l for l in remove_lines
        if l.strip() and not _is_filtered_removal(l)
    ]

    if not add_real and not remove_real:
        return [], False

    out = [header]
    out.extend(add_lines)
    out.append("\n")
    out.append(header.replace(" should add these lines:", " should remove these lines:"))
    if remove_real:
        out.extend(remove_real)
    out.append("\n")
    out.extend(tail_lines)
    return out, True


def main():
    files_with_findings = filter_iwyu(sys.stdin, sys.stdout)
    if "--filter-only" in sys.argv:
        return 0
    return 1 if files_with_findings else 0


if __name__ == "__main__":
    sys.exit(main())
