"""MkDocs build hook: the per-pack coverage table on the Coverage page.

The coverage lane leaves two things in its run artifact: the genhtml report,
one directory per pack with a page per source file, and the lcov tracefile the
gate was judged on. docs-build places both under ``docs/testing/coverage-report/``
before it builds, and this hook fills the ``<!-- coverage-table -->`` marker on
the Coverage page with one row per pack, each linking into that pack's part of
the report. The figures are computed by ``scripts/coverage_summary.py`` from the
same tracefile the lane gated on, so the number a reader sees is the number that
passed.

A build with no report in hand - every local one - gets a sentence saying so and
no links, so a local ``mkdocs build --strict`` passes without the artifact. The
report directory itself is git-ignored and copied into the site untouched, which
is also how it reaches the offline bundle.
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import coverage_summary  # noqa: E402

PAGE = "testing/coverage.md"
MARKER = "<!-- coverage-table -->"
# Relative to the Coverage page, which sits beside the directory.
REPORT = "coverage-report"
TRACEFILE = "coverage.filtered.info"

ABSENT = (
    "This build was made without a coverage report. The published site and the "
    "offline documentation bundle carry one, measured by the coverage lane of the "
    "run that built them."
)


def _tracefile(config):
    return os.path.join(config["docs_dir"], "testing", REPORT, TRACEFILE)


def _index_of(pack):
    if pack == "Core":
        return f"{REPORT}/Core/Source/index.html"
    return f"{REPORT}/Platform/{pack}/Source/index.html"


def _table(packs):
    lines = ["| Pack | Lines | | Branches | |", "|---|---:|---:|---:|---:|"]
    ordered = ["Core"] + sorted(name for name in packs if name != "Core")
    lines.extend(coverage_summary.row(f"[{name}]({_index_of(name)})", packs[name]) for name in ordered if name in packs)
    lines.append(coverage_summary.row(f"[**Total**]({REPORT}/index.html)", coverage_summary.total(packs)))
    return "\n".join(lines)


def on_page_markdown(markdown, page, config, files, **kwargs):
    if page.file.src_uri != PAGE:
        return markdown
    path = _tracefile(config)
    if os.path.exists(path):
        with open(path, encoding="utf-8") as handle:
            filled = _table(coverage_summary.aggregate(handle))
    else:
        filled = ABSENT
    return markdown.replace(MARKER, filled)
