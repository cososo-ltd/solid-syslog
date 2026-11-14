"""MkDocs build hook: the per-pack coverage table on the Coverage page.

The coverage lane leaves three things in its run artifact: the genhtml report,
one directory per pack with a page per source file, the lcov tracefile the gate
was judged on, and llvm-cov's JSON summary beside it, which carries the MC/DC
conditions the tracefile cannot. docs-build places them under
``docs/testing/coverage-report/`` before it builds, and this hook fills the
``<!-- coverage-table -->`` marker on the Coverage page with a line saying what
was measured, then one row per pack, each linking into that pack's part of the
report. The figures are computed by ``scripts/coverage_summary.py`` from the
same files the lane gated on, so the number a reader sees is the number that
passed. llvm-cov's own HTML report, staged beside it as
``docs/testing/mcdc-report/``, holds each decision's test vectors, and the page
links it when it is there.

The provenance line comes from ``scripts/docs_provenance.py``, which reads the
variables the footer reads, so it names the branch and commit, the pull
request, or the release that the footer names: the report in a release bundle
was measured from the tag, and saying "main" there would be false.

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
import docs_provenance  # noqa: E402

PAGE = "testing/coverage.md"
MARKER = "<!-- coverage-table -->"
# Relative to the Coverage page, which sits beside the directory.
REPORT = "coverage-report"
TRACEFILE = "coverage.filtered.info"
LLVM_SUMMARY = "coverage.llvm.json"
MCDC_REPORT = "mcdc-report"

MCDC_DETAIL = (
    "Each decision's conditions and the test vectors that exercised them are in the "
    f"[MC/DC report]({MCDC_REPORT}/index.html)."
)

ABSENT = (
    "This build was made without a coverage report. The published site and the "
    "offline documentation bundle carry one, measured by the coverage lane of the "
    "run that built them."
)


def _src_uri(file):
    return getattr(file, "src_uri", None) or file.src_path.replace(os.sep, "/")


def _tracefile(config):
    return os.path.join(config["docs_dir"], "testing", REPORT, TRACEFILE)


def _llvm_summary(config):
    return os.path.join(config["docs_dir"], "testing", REPORT, LLVM_SUMMARY)


def _mcdc_report(config):
    return os.path.join(config["docs_dir"], "testing", MCDC_REPORT)


provenance = docs_provenance.provenance


def _link(name):
    if name == coverage_summary.TOTAL:
        return f"[{name}]({REPORT}/index.html)"
    if name == "Core":
        return f"[{name}]({REPORT}/Core/Source/index.html)"
    return f"[{name}]({REPORT}/Platform/{name}/Source/index.html)"


def on_page_markdown(markdown, page, config, files, **kwargs):
    if _src_uri(page.file) != PAGE:
        return markdown
    path = _tracefile(config)
    if os.path.exists(path):
        with open(path, encoding="utf-8") as handle:
            packs = coverage_summary.aggregate(handle)
        if os.path.exists(_llvm_summary(config)):
            with open(_llvm_summary(config), encoding="utf-8") as handle:
                coverage_summary.add_mcdc(packs, handle)
        filled = provenance(os.environ) + "\n\n" + coverage_summary.markdown(packs, label=_link)
        if os.path.isdir(_mcdc_report(config)):
            filled += "\n\n" + MCDC_DETAIL
    else:
        filled = ABSENT
    return markdown.replace(MARKER, filled)
