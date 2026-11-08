"""MkDocs build hook: the per-pack coverage table on the Coverage page.

The coverage lane leaves two things in its run artifact: the genhtml report,
one directory per pack with a page per source file, and the lcov tracefile the
gate was judged on. docs-build places both under ``docs/testing/coverage-report/``
before it builds, and this hook fills the ``<!-- coverage-table -->`` marker on
the Coverage page with a line saying what was measured, then one row per pack,
each linking into that pack's part of the report. The figures are computed by
``scripts/coverage_summary.py`` from the same tracefile the lane gated on, so
the number a reader sees is the number that passed.

The provenance line reads ``SOLIDSYSLOG_DOCS_RELEASE``, ``SOLIDSYSLOG_DOCS_ORIGIN``
and ``SOLIDSYSLOG_DOCS_COMMIT``, three of the variables ``hooks/build_version.py``
reads for the footer, so it names the branch and commit, the pull request, or
the release that the footer names: the report in a release bundle was measured
from the tag, and saying "main" there would be false. Unlike the footer it has
no use for the last release number, so it does not wait for it.

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
SHORT_COMMIT_LENGTH = 7

ABSENT = (
    "This build was made without a coverage report. The published site and the "
    "offline documentation bundle carry one, measured by the coverage lane of the "
    "run that built them."
)


def _src_uri(file):
    return getattr(file, "src_uri", None) or file.src_path.replace(os.sep, "/")


def _tracefile(config):
    return os.path.join(config["docs_dir"], "testing", REPORT, TRACEFILE)


def provenance(environ):
    release = environ.get("SOLIDSYSLOG_DOCS_RELEASE", "").strip()
    if release:
        return f"Measured for release {release}."
    origin = environ.get("SOLIDSYSLOG_DOCS_ORIGIN", "").strip()
    commit = environ.get("SOLIDSYSLOG_DOCS_COMMIT", "").strip()[:SHORT_COMMIT_LENGTH]
    if origin and commit:
        return f"Measured from {origin} at {commit}."
    return "Measured by a local build."


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
            table = coverage_summary.markdown(coverage_summary.aggregate(handle), label=_link)
        filled = provenance(os.environ) + "\n\n" + table
    else:
        filled = ABSENT
    return markdown.replace(MARKER, filled)
