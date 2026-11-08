"""MkDocs build hook: the headline figures on the Tests page.

docs-build generates the test report under ``docs/testing/test-report/`` from
the JUnit artifacts of the run that builds the site (``scripts/junit_report_pages.py``),
and that script leaves ``summary.json`` beside the pages. This hook fills the
``<!-- test-summary -->`` marker on the Tests page from it: the provenance
line, one sentence of totals, a table by kind, and the doorways into the
report. A build without the report in hand - every local one - gets a sentence
saying so and no links, so a local ``mkdocs build --strict`` passes without
the artifacts. The directory is git-ignored and copied into the site untouched,
which is also how it reaches the offline bundle, and ``hooks/report_chrome.py``
dresses its pages in the site's header and footer after the build.
"""

import json
import os

PAGE = "testing/tests.md"
MARKER = "<!-- test-summary -->"
# Relative to the Tests page, which sits beside the directory.
REPORT = "test-report"
SUMMARY = "summary.json"

ABSENT = (
    "This build was made without a test report. The published site and the "
    "offline documentation bundle carry one, generated from the JUnit results of "
    "the run that built them."
)

DOORWAYS = [
    ("index.html", "Overview", "headline counts, failures first, then every lane by kind and every pack"),
    ("failures.html", "Failures", "each with its message and a link to the line"),
    ("skipped.html", "Skipped", "each with the tags that decided it"),
    ("suites.html", "Suites across lanes", "where each unit suite ran, and how many of its tests"),
    ("scenarios.html", "Scenarios across lanes", "every BDD scenario against every target"),
    ("slowest.html", "Slowest", "test time by lane, and the slowest suites and tests"),
]


def _src_uri(file):
    return getattr(file, "src_uri", None) or file.src_path.replace(os.sep, "/")


def _summary_path(config):
    return os.path.join(config["docs_dir"], "testing", REPORT, SUMMARY)


def fmt_time(seconds):
    if seconds >= 60:
        minutes, rest = divmod(seconds, 60)
        return f"{int(minutes)} min {int(round(rest))} s"
    if seconds >= 1:
        return f"{seconds:.1f} s"
    return f"{int(round(seconds * 1000))} ms"


def render(summary):
    totals = summary["totals"]
    failed = f"**{totals['failed']} failed**" if totals["failed"] else "none failed"
    sentence = (
        f"{totals['total']} tests across {len(summary['lanes'])} lanes: {totals['passed']} passed, "
        f"{failed}, {totals['skipped']} skipped, {fmt_time(totals['time'])} of test time in all."
    )
    rows = ["| Kind | Lanes | Tests | Passed | Failed | Skipped | Time |", "|---|---:|---:|---:|---:|---:|---:|"]
    rows.extend(
        f"| {k['name']} | {k['lanes']} | {k['total']} | {k['passed']} | {k['failed']} | {k['skipped']} | {fmt_time(k['time'])} |"
        for k in summary["kinds"]
    )
    doors = [f"- [{label}]({REPORT}/{href}): {text}." for href, label, text in DOORWAYS]
    return "\n\n".join([summary["provenance"], sentence, "\n".join(rows), "\n".join(doors)])


def on_page_markdown(markdown, page, config, files, **kwargs):
    if _src_uri(page.file) != PAGE:
        return markdown
    path = _summary_path(config)
    if os.path.exists(path):
        with open(path, encoding="utf-8") as handle:
            filled = render(json.load(handle))
    else:
        filled = ABSENT
    return markdown.replace(MARKER, filled)
