#!/usr/bin/env python3
"""Render the test report: what a CI run tested, organised so it can be read.

Reads the ``junit-<lane>`` artifacts of one run (``junit_report.py``) and writes
a directory of static pages, no script, that the documentation site publishes
under its Testing tab and the offline bundle carries:

    index.html      Overview: headline counts, failures first, by kind, by lane
                    grouped by kind, by pack.
    failures.html   Every failure with its message and a link to the line.
    skipped.html    Every skip by lane, each with the tags that explain it.
    lanes/<lane>    The lane's executables, their suites, each suite opening to
                    its tests with status, duration and a source link.
    suites.html     Suites across lanes: where each unit suite ran, and how many
                    of its tests.
    scenarios.html  Scenarios across lanes: the BDD matrix, grouped by feature.
    packs/<pack>    What ran for one pack, lane by lane.
    slowest.html    The slowest tests and suites across the run.

Every page carries the marker comments ``hooks/report_chrome.py`` fills with
the site's header and footer; opened from a build tree it keeps a plain header.

Usage:
    junit_report_pages.py <artifacts-dir> <output-dir>
        [--features Bdd/features] [--registry CMakeLists.txt]
        [--source-url https://github.com/<owner>/<repo>/blob/<commit>/]
"""

import argparse
import html
import os
import re
import sys
from collections import defaultdict
from typing import Dict, List, Optional

import docs_provenance
import junit_report as model

NAV = [
    ("index.html", "Overview"),
    ("failures.html", "Failures"),
    ("skipped.html", "Skipped"),
    ("suites.html", "Suites across lanes"),
    ("scenarios.html", "Scenarios across lanes"),
    ("slowest.html", "Slowest"),
]
KIND_ORDER = ["unit", "integration", "build", "bdd"]
KIND_NAMES = {"unit": "Unit", "integration": "Integration", "build": "Build", "bdd": "BDD"}
STATUS_WORD = {"passed": "pass", "failed": "FAIL", "skipped": "skip"}
SLOWEST_TESTS = 100
SLOWEST_SUITES = 30

esc = html.escape


def fmt_time(seconds: float) -> str:
    if seconds >= 60:
        minutes, rest = divmod(seconds, 60)
        return f"{int(minutes)} min {int(round(rest))} s"
    if seconds >= 1:
        return f"{seconds:.1f} s"
    return f"{int(round(seconds * 1000))} ms"


def slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def kind_name(kind: str) -> str:
    return KIND_NAMES.get(kind, kind.capitalize())


def kinds_in(run: model.Run) -> List[str]:
    present = {lane.kind for lane in run.lanes.values()}
    return [k for k in KIND_ORDER if k in present] + sorted(present - set(KIND_ORDER))


def lanes_of_kind(run: model.Run, kind: str) -> List[model.Lane]:
    return [lane for lane in run.lanes.values() if lane.kind == kind]


def source(test: model.Test, source_url: Optional[str]) -> str:
    if not test.file:
        return ""
    text = f"{test.file}:{test.line}" if test.line else test.file
    if not source_url:
        return f'<code>{esc(text)}</code>'
    anchor = f"#L{test.line}" if test.line else ""
    return f'<a class="tr-src" href="{esc(source_url)}{esc(test.file)}{anchor}"><code>{esc(text)}</code></a>'


def status_cell(status: str) -> str:
    return f'<td class="tr-status tr-{status}">{STATUS_WORD[status]}</td>'


def counts_cells(counts: model.Counts) -> str:
    failed = f'<td class="tr-num tr-bad">{counts.failed}</td>' if counts.failed else '<td class="tr-num">0</td>'
    skipped = f'<td class="tr-num tr-note">{counts.skipped}</td>' if counts.skipped else '<td class="tr-num">0</td>'
    return (
        f'<td class="tr-num">{counts.total}</td><td class="tr-num">{counts.passed}</td>{failed}{skipped}'
        f'<td class="tr-num">{fmt_time(counts.time)}</td>'
    )


COUNT_HEADERS = "<th>Tests</th><th>Passed</th><th>Failed</th><th>Skipped</th><th>Time</th>"


def page(title: str, body: str, depth: int, provenance: str, active: Optional[str] = None) -> str:
    rel = "../" * depth
    nav = "".join(
        f'<a href="{rel}{href}"{" class=\"tr-nav--active\"" if href == active else ""}>{esc(label)}</a>' for href, label in NAV
    )
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{esc(title)} - SolidSyslog tests</title>
  <!-- site-styles -->
  <link rel="stylesheet" href="{rel}report.css">
</head>
<body>
  <!-- site-header -->
  <header class="ss-header"><a class="ss-header__brand" href="{rel}index.html"><span class="ss-header__name">SolidSyslog</span></a><span class="ss-header__section">Tests</span></header>
  <!-- /site-header -->
  <main class="tr">
    <nav class="tr-nav">{nav}</nav>
    <h1>{esc(title)}</h1>
    <p class="tr-prov">{esc(provenance)}</p>
{body}
  </main>
  <!-- site-footer -->
  <footer class="ss-footer">Part of the SolidSyslog documentation. Reported from the run's JUnit results.</footer>
  <!-- /site-footer -->
</body>
</html>
"""


def lane_href(lane: str, depth: int = 0) -> str:
    return f'{"../" * depth}lanes/{lane}.html'


def pack_href(pack: str, depth: int = 0) -> str:
    return f'{"../" * depth}packs/{slug(pack)}.html'


def suite_id(test: model.Test) -> str:
    return slug(f"{test.executable}-{test.suite}")


def by(tests, key):
    grouped: Dict = defaultdict(list)
    for test in tests:
        grouped[key(test)].append(test)
    return grouped


class Report:
    def __init__(self, run: model.Run, tokens: List[str], source_url: Optional[str], provenance: str):
        self.run = run
        self.source_url = source_url
        self.provenance = provenance
        self.pack_of = {}
        for test in run.tests:
            self.pack_of.setdefault(test.file, model.pack_of(test.file, tokens))
        self.failures = [t for t in run.tests if t.status == "failed"]
        self.skips = [t for t in run.tests if t.status == "skipped"]

    def pack(self, test: model.Test) -> str:
        return self.pack_of[test.file]

    def packs(self) -> Dict[str, Dict[str, List[model.Test]]]:
        grouped: Dict[str, Dict[str, List[model.Test]]] = defaultdict(lambda: defaultdict(list))
        for test in self.run.tests:
            grouped[self.pack(test)][test.lane].append(test)
        return grouped

    # ---- pages -----------------------------------------------------------

    def overview(self) -> str:
        run = self.run
        total = model.count(run.tests)
        tiles = [
            ("Tests", f"{total.total}"),
            ("Passed", f"{total.passed}"),
            ("Failed", f"{total.failed}"),
            ("Skipped", f"{total.skipped}"),
            ("Lanes", f"{len(run.lanes)}"),
            ("Test time", fmt_time(total.time)),
        ]
        tile_html = "".join(
            f'<div class="tr-tile{" tr-tile--bad" if label == "Failed" and total.failed else ""}"><span class="tr-tile__n">{esc(value)}</span><span class="tr-tile__l">{label}</span></div>'
            for label, value in tiles
        )
        if self.failures:
            failures = (
                f'<h2 id="failures">Failures</h2><p class="tr-bad">{len(self.failures)} failed. '
                f'<a href="failures.html">Every failure with its message and line.</a></p>'
                + self.failure_table(self.failures[:20], depth=0)
            )
        else:
            failures = '<h2 id="failures">Failures</h2><p class="tr-good">No failures in this run.</p>'
        kinds = "".join(
            f"<tr><td>{kind_name(kind)}</td><td class=\"tr-num\">{len(lanes_of_kind(run, kind))}</td>"
            f"{counts_cells(model.count(t for lane in lanes_of_kind(run, kind) for t in lane.tests))}</tr>"
            for kind in kinds_in(run)
        )
        lane_rows = []
        for kind in kinds_in(run):
            lane_rows.append(f'<tr class="tr-group"><th colspan="8">{kind_name(kind)}</th></tr>')
            for lane in lanes_of_kind(run, kind):
                executables = len({t.executable for t in lane.tests})
                suites = len({(t.executable, t.suite) for t in lane.tests})
                lane_rows.append(
                    f'<tr><td><a href="{lane_href(lane.name)}">{esc(lane.name)}</a></td>'
                    f'<td class="tr-num">{executables}</td><td class="tr-num">{suites}</td>{counts_cells(lane.counts)}</tr>'
                )
        pack_rows = []
        for pack, lanes in sorted(self.packs().items(), key=lambda item: item[0].lower()):
            tests = [t for group in lanes.values() for t in group]
            pack_rows.append(
                f'<tr><td><a href="{pack_href(pack)}">{esc(pack)}</a></td><td class="tr-num">{len(lanes)}</td>{counts_cells(model.count(tests))}</tr>'
            )
        body = f"""
    <div class="tr-tiles">{tile_html}</div>
    <p>Test time is the sum of every test's own duration, across lanes that ran in parallel; it is not the run's wall clock.</p>
    {failures}
    <h2 id="kinds">By kind</h2>
    <table class="tr-table"><thead><tr><th>Kind</th><th>Lanes</th>{COUNT_HEADERS}</tr></thead><tbody>{kinds}</tbody></table>
    <h2 id="lanes">By lane</h2>
    <table class="tr-table"><thead><tr><th>Lane</th><th>Executables</th><th>Suites</th>{COUNT_HEADERS}</tr></thead><tbody>{"".join(lane_rows)}</tbody></table>
    <h2 id="packs">By pack</h2>
    <p>Tests attributed by the test file that holds them. A pack's figure counts every lane that ran it.</p>
    <table class="tr-table"><thead><tr><th>Pack</th><th>Lanes</th>{COUNT_HEADERS}</tr></thead><tbody>{"".join(pack_rows)}</tbody></table>
"""
        return page("Tests", body, 0, self.provenance, "index.html")

    def failure_table(self, failures: List[model.Test], depth: int) -> str:
        rows = "".join(
            f'<tr><td><a href="{lane_href(t.lane, depth)}#{suite_id(t)}">{esc(t.lane)}</a></td><td>{esc(t.suite)}</td>'
            f'<td>{esc(t.name)}</td><td>{source(t, self.source_url)}</td></tr>'
            for t in failures
        )
        return f'<table class="tr-table"><thead><tr><th>Lane</th><th>Suite</th><th>Test</th><th>Source</th></tr></thead><tbody>{rows}</tbody></table>'

    def failures_page(self) -> str:
        if not self.failures:
            body = '<p class="tr-good">No failures in this run.</p>'
        else:
            items = "".join(
                f'<section class="tr-failure" id="{slug(t.lane + "-" + t.suite + "-" + t.name)}">'
                f'<h3>{esc(t.suite)}: {esc(t.name)}</h3>'
                f'<p><a href="{lane_href(t.lane)}#{suite_id(t)}">{esc(t.lane)}</a>, {esc(t.executable)} &middot; {source(t, self.source_url)} &middot; {fmt_time(t.time)}</p>'
                f'<pre>{esc(t.message or "")}</pre></section>'
                for t in self.failures
            )
            body = f'<p class="tr-bad">{len(self.failures)} failed across {len({t.lane for t in self.failures})} lane(s).</p>{items}'
        return page("Failures", body, 0, self.provenance, "failures.html")

    def skipped_page(self) -> str:
        if not self.skips:
            body = "<p>Nothing was skipped in this run.</p>"
        else:
            sections = []
            for lane_name, tests in by(self.skips, lambda t: t.lane).items():
                rows = "".join(
                    f'<tr><td>{esc(t.suite)}</td><td>{esc(t.name)}</td><td>{self.tags(t)}</td></tr>' for t in tests
                )
                sections.append(
                    f'<h2 id="{slug(lane_name)}"><a href="{lane_href(lane_name)}">{esc(lane_name)}</a> <span class="tr-muted">{len(tests)} skipped</span></h2>'
                    f'<table class="tr-table"><thead><tr><th>Suite</th><th>Test</th><th>Tags</th></tr></thead><tbody>{rows}</tbody></table>'
                )
            body = (
                "<p>A scenario is skipped when the lane's tag filter excludes it, or a <code>@requires_</code> tag "
                "names a tunable the target is built below. The tags are what the filter saw.</p>" + "".join(sections)
            )
        return page("Skipped", body, 0, self.provenance, "skipped.html")

    def tags(self, test: model.Test) -> str:
        return " ".join(f'<span class="tr-tag">@{esc(tag)}</span>' for tag in test.tags)

    def lane_page(self, lane: model.Lane) -> str:
        counts = lane.counts
        sections = []
        for executable, tests in sorted(by(lane.tests, lambda t: t.executable).items()):
            suites = sorted(by(tests, lambda t: t.suite).items())
            ex_counts = model.count(tests)
            summary_rows = []
            for suite, cases in suites:
                c = model.count(cases)
                slowest = max(cases, key=lambda t: t.time)
                summary_rows.append(
                    f'<tr><td><a href="#{suite_id(cases[0])}">{esc(suite)}</a></td>{counts_cells(c)}'
                    f'<td>{esc(slowest.name)} <span class="tr-muted">{fmt_time(slowest.time)}</span></td></tr>'
                )
            detail_blocks = []
            for suite, cases in suites:
                c = model.count(cases)
                is_bdd = lane.kind == "bdd"
                head = "<th>Status</th><th>Test</th><th>Time</th>" + ("<th>Tags</th>" if is_bdd else "<th>Source</th>")
                rows = "".join(
                    f"<tr>{status_cell(t.status)}<td>{esc(t.name)}</td><td class=\"tr-num\">{fmt_time(t.time)}</td>"
                    f"<td>{self.tags(t) if is_bdd else source(t, self.source_url)}</td></tr>"
                    for t in cases
                )
                badge = f'<span class="tr-bad">{c.failed} failed</span> ' if c.failed else ""
                badge += f'<span class="tr-note">{c.skipped} skipped</span> ' if c.skipped else ""
                detail_blocks.append(
                    f'<details id="{suite_id(cases[0])}"{" open" if c.failed else ""}><summary><strong>{esc(suite)}</strong> '
                    f'<span class="tr-muted">{c.total} tests, {fmt_time(c.time)}</span> {badge}'
                    f'{" &middot; " + source(cases[0], self.source_url) if cases[0].file and not is_bdd else ""}</summary>'
                    f'<table class="tr-table"><thead><tr>{head}</tr></thead><tbody>{rows}</tbody></table></details>'
                )
            sections.append(
                f'<h2 id="{slug(executable)}">{esc(executable)} <span class="tr-muted">{ex_counts.total} tests in {len(suites)} suites, {fmt_time(ex_counts.time)}</span></h2>'
                f'<table class="tr-table"><thead><tr><th>Suite</th>{COUNT_HEADERS}<th>Slowest test</th></tr></thead><tbody>{"".join(summary_rows)}</tbody></table>'
                f'<h3>Tests by suite</h3>{"".join(detail_blocks)}'
            )
        body = (
            f'<p><span class="tr-kind">{kind_name(lane.kind)}</span> {counts.total} tests, {counts.passed} passed, '
            f'<span class="{"tr-bad" if counts.failed else ""}">{counts.failed} failed</span>, {counts.skipped} skipped, {fmt_time(counts.time)}.</p>'
            + "".join(sections)
        )
        return page(lane.name, body, 1, self.provenance)

    def suites_page(self) -> str:
        lanes = [lane for lane in self.run.lanes.values() if lane.kind != "bdd"]
        rows_by_key: Dict = defaultdict(dict)
        for lane in lanes:
            for (executable, suite), cases in by(lane.tests, lambda t: (t.executable, t.suite)).items():
                rows_by_key[(executable, suite)][lane.name] = (model.count(cases), suite_id(cases[0]))
        head = "".join(f'<th class="tr-vert"><a href="{lane_href(lane.name)}"><span>{esc(lane.name)}</span></a></th>' for lane in lanes)
        rows = []
        current = None
        for (executable, suite), per_lane in sorted(rows_by_key.items()):
            if executable != current:
                current = executable
                rows.append(f'<tr class="tr-group"><th colspan="{len(lanes) + 1}">{esc(executable)}</th></tr>')
            cells = []
            for lane in lanes:
                found = per_lane.get(lane.name)
                if not found:
                    cells.append('<td class="tr-num tr-none">-</td>')
                else:
                    counts, anchor = found
                    cls = "tr-bad" if counts.failed else ""
                    cells.append(f'<td class="tr-num {cls}"><a href="{lane_href(lane.name)}#{anchor}">{counts.total}</a></td>')
            rows.append(f"<tr><td>{esc(suite)}</td>{''.join(cells)}</tr>")
        body = (
            "<p>Each cell is how many of the suite's tests that lane ran; a failing suite is marked. "
            "A dash means the lane does not build the suite.</p>"
            f'<table class="tr-table tr-matrix"><thead><tr><th>Suite</th>{head}</tr></thead><tbody>{"".join(rows)}</tbody></table>'
        )
        return page("Suites across lanes", body, 0, self.provenance, "suites.html")

    def scenarios_page(self) -> str:
        lanes = lanes_of_kind(self.run, "bdd")
        features: Dict[str, Dict[str, Dict[str, model.Test]]] = defaultdict(lambda: defaultdict(dict))
        for lane in lanes:
            for t in lane.tests:
                features[t.suite][t.name][lane.name] = t
        head = "".join(f'<th class="tr-vert"><a href="{lane_href(lane.name)}"><span>{esc(lane.name)}</span></a></th>' for lane in lanes)
        rows = []
        for feature, scenarios in sorted(features.items()):
            rows.append(f'<tr class="tr-group"><th colspan="{len(lanes) + 2}">{esc(feature)}</th></tr>')
            for name, per_lane in scenarios.items():
                any_test = next(iter(per_lane.values()))
                cells = "".join(
                    status_cell(per_lane[lane.name].status) if lane.name in per_lane else '<td class="tr-none">-</td>' for lane in lanes
                )
                rows.append(f"<tr><td>{esc(name)}</td><td>{self.tags(any_test)}</td>{cells}</tr>")
        body = (
            "<p>Every scenario against every target it was offered to. A skip is the lane's tag filter or a "
            "<code>@requires_</code> tag; the tags column is what decided it.</p>"
            f'<table class="tr-table tr-matrix"><thead><tr><th>Scenario</th><th>Tags</th>{head}</tr></thead><tbody>{"".join(rows)}</tbody></table>'
        )
        return page("Scenarios across lanes", body, 0, self.provenance, "scenarios.html")

    def pack_page(self, pack: str, lanes: Dict[str, List[model.Test]]) -> str:
        all_tests = [t for tests in lanes.values() for t in tests]
        total = model.count(all_tests)
        lane_rows = "".join(
            f'<tr><td><a href="{lane_href(lane, 1)}">{esc(lane)}</a></td><td class="tr-num">{len({(t.executable, t.suite) for t in tests})}</td>{counts_cells(model.count(tests))}</tr>'
            for lane, tests in sorted(lanes.items())
        )
        suites: Dict[str, set] = defaultdict(set)
        for lane, tests in lanes.items():
            for t in tests:
                suites[t.suite].add(lane)
        suite_rows = "".join(
            f'<tr><td>{esc(suite)}</td><td class="tr-num">{len({t.name for t in all_tests if t.suite == suite})}</td>'
            f'<td>{", ".join(f"<a href=\"{lane_href(lane, 1)}#{slug(next(t.executable for t in lanes[lane] if t.suite == suite) + "-" + suite)}\">{esc(lane)}</a>" for lane in sorted(ran))}</td></tr>'
            for suite, ran in sorted(suites.items())
        )
        files = sorted({t.file for t in all_tests if t.file})
        body = f"""
    <p>{total.total} test results across {len(lanes)} lanes, from {len(files)} test files.</p>
    <h2>By lane</h2>
    <table class="tr-table"><thead><tr><th>Lane</th><th>Suites</th>{COUNT_HEADERS}</tr></thead><tbody>{lane_rows}</tbody></table>
    <h2>Suites</h2>
    <p>Distinct tests per suite, and the lanes that ran it.</p>
    <table class="tr-table"><thead><tr><th>Suite</th><th>Tests</th><th>Lanes</th></tr></thead><tbody>{suite_rows}</tbody></table>
"""
        return page(pack, body, 1, self.provenance)

    def slowest_page(self) -> str:
        tests = sorted(self.run.tests, key=lambda t: t.time, reverse=True)[:SLOWEST_TESTS]
        test_rows = "".join(
            f'<tr><td class="tr-num">{fmt_time(t.time)}</td><td><a href="{lane_href(t.lane)}#{suite_id(t)}">{esc(t.lane)}</a></td>'
            f'<td>{esc(t.suite)}</td><td>{esc(t.name)}</td>{status_cell(t.status)}</tr>'
            for t in tests
        )
        suites = sorted(
            ((model.count(cases), key, cases[0]) for key, cases in by(self.run.tests, lambda t: (t.lane, t.executable, t.suite)).items()),
            key=lambda item: item[0].time,
            reverse=True,
        )[:SLOWEST_SUITES]
        suite_rows = "".join(
            f'<tr><td class="tr-num">{fmt_time(c.time)}</td><td><a href="{lane_href(lane)}#{suite_id(first)}">{esc(lane)}</a></td>'
            f'<td>{esc(suite)}</td><td class="tr-num">{c.total}</td></tr>'
            for c, (lane, _, suite), first in suites
        )
        lane_rows = "".join(
            f'<tr><td><a href="{lane_href(lane.name)}">{esc(lane.name)}</a></td><td class="tr-num">{lane.counts.total}</td><td class="tr-num">{fmt_time(lane.counts.time)}</td></tr>'
            for lane in sorted(self.run.lanes.values(), key=lambda lane: lane.counts.time, reverse=True)
        )
        body = f"""
    <p>Durations as each test runner reported them; CppUTest reports to the millisecond.</p>
    <h2>Test time by lane</h2>
    <table class="tr-table"><thead><tr><th>Lane</th><th>Tests</th><th>Time</th></tr></thead><tbody>{lane_rows}</tbody></table>
    <h2>Slowest suites</h2>
    <table class="tr-table"><thead><tr><th>Time</th><th>Lane</th><th>Suite</th><th>Tests</th></tr></thead><tbody>{suite_rows}</tbody></table>
    <h2>Slowest tests</h2>
    <table class="tr-table"><thead><tr><th>Time</th><th>Lane</th><th>Suite</th><th>Test</th><th>Status</th></tr></thead><tbody>{test_rows}</tbody></table>
"""
        return page("Slowest", body, 0, self.provenance, "slowest.html")

    # ---- writing ---------------------------------------------------------

    def write(self, out_dir: str) -> int:
        written = 0
        written += self._write(out_dir, "index.html", self.overview())
        written += self._write(out_dir, "failures.html", self.failures_page())
        written += self._write(out_dir, "skipped.html", self.skipped_page())
        written += self._write(out_dir, "suites.html", self.suites_page())
        written += self._write(out_dir, "scenarios.html", self.scenarios_page())
        written += self._write(out_dir, "slowest.html", self.slowest_page())
        for lane in self.run.lanes.values():
            written += self._write(out_dir, f"lanes/{lane.name}.html", self.lane_page(lane))
        for pack, lanes in self.packs().items():
            written += self._write(out_dir, f"packs/{slug(pack)}.html", self.pack_page(pack, lanes))
        written += self._write(out_dir, "report.css", CSS)
        return written

    @staticmethod
    def _write(out_dir: str, relative: str, text: str) -> int:
        path = os.path.join(out_dir, relative)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
        return 1


CSS = """\
/* The test report's own styles. The documentation build adds the site's
   stylesheets before this one (hooks/report_chrome.py), so what follows also
   corrects what Material's global rules would do to these pages. */
body { margin: 0; font-family: "Open Sans", "Segoe UI", Roboto, Helvetica, Arial, sans-serif; color: #1f1f1f; }
body[data-md-color-scheme] { font-size: 0.8rem; }
body[data-md-color-scheme] .md-footer { margin-top: auto; }
.md-header__title.md-header__title--active { font-size: 0.9rem; }
.md-header__topic a, .md-header__topic a:link, .md-header__topic a:visited { color: inherit; }
.ss-header { display: flex; align-items: center; gap: 12px; padding: 10px 16px; border-bottom: 1px solid #e0e0e0; }
.ss-header__name { font-weight: 700; font-size: 18px; color: #1f1f1f; text-decoration: none; }
.ss-header__brand { text-decoration: none; }
.ss-header__section { color: #5f5f5f; }
.ss-footer { margin: 24px 16px; font-size: 13px; color: #7a7a7a; }
.tr { max-width: 1220px; margin: 0 auto; padding: 8px 16px 32px; line-height: 1.5; }
.tr h1 { font-family: "Montserrat", "Segoe UI", Roboto, Helvetica, Arial, sans-serif; font-size: 1.6em; margin: 16px 0 4px; }
.tr h2 { font-family: "Montserrat", "Segoe UI", Roboto, Helvetica, Arial, sans-serif; font-size: 1.2em; margin: 28px 0 8px; }
.tr h3 { font-size: 1em; margin: 20px 0 8px; }
.tr a { color: #3e2199; text-decoration: none; }
.tr a:hover { text-decoration: underline; }
.tr-nav { display: flex; flex-wrap: wrap; gap: 4px 18px; padding: 8px 0; border-bottom: 1px solid #e0e0e0; font-size: 0.95em; }
.tr-nav a { color: #3e2199; }
.tr-nav--active { font-weight: 700; }
.tr-prov { color: #7a7a7a; margin: 0 0 16px; }
.tr-tiles { display: flex; flex-wrap: wrap; gap: 12px; margin: 12px 0; }
.tr-tile { min-width: 110px; padding: 10px 14px; border: 1px solid #e0e0e0; border-radius: 6px; display: flex; flex-direction: column; }
.tr-tile__n { font-size: 1.6em; font-weight: 700; color: #3e2199; }
.tr-tile__l { color: #5f5f5f; font-size: 0.9em; }
.tr-tile--bad .tr-tile__n { color: #b3261e; }
.tr-table { border-collapse: collapse; width: 100%; margin: 8px 0 16px; font-size: 0.95em; }
.tr-table th, .tr-table td { padding: 5px 8px; border-bottom: 1px solid #eee; text-align: left; vertical-align: top; }
.tr-table thead th { background: #3e2199; color: #fff; font-weight: 600; }
.tr-table tr.tr-group th { background: #f1eefa; color: #1f1f1f; font-weight: 700; }
.tr-num { text-align: right !important; white-space: nowrap; font-variant-numeric: tabular-nums; }
.tr-status { font-weight: 700; white-space: nowrap; width: 1%; }
.tr-passed { color: #1b7f3b; }
.tr-failed { color: #b3261e; }
.tr-skipped { color: #946200; }
.tr-bad { color: #b3261e; font-weight: 700; }
.tr-good { color: #1b7f3b; }
.tr-note { color: #946200; }
.tr-none { color: #bbb; text-align: center !important; }
.tr-muted { color: #7a7a7a; font-weight: 400; font-size: 0.92em; }
.tr-kind { display: inline-block; padding: 1px 8px; border-radius: 10px; background: #f1eefa; color: #3e2199; font-size: 0.9em; margin-right: 6px; }
.tr-tag { display: inline-block; padding: 0 6px; border-radius: 8px; background: #f1f1f1; color: #444; font-size: 0.85em; margin: 1px 2px 1px 0; white-space: nowrap; }
.tr-src code, .tr code { font-size: 0.9em; }
.tr details { margin: 6px 0; border: 1px solid #e8e8e8; border-radius: 6px; padding: 4px 10px; }
.tr details[open] { padding-bottom: 8px; }
.tr summary { cursor: pointer; padding: 4px 0; }
.tr-failure { margin: 16px 0; padding: 8px 12px; border-left: 4px solid #b3261e; background: #fcf6f6; }
.tr-failure pre { white-space: pre-wrap; margin: 6px 0 0; font-size: 0.9em; }
.tr-matrix td, .tr-matrix th { padding: 3px 6px; }
.tr-matrix th.tr-vert { height: 200px; white-space: nowrap; vertical-align: bottom; padding: 0 2px; }
.tr-matrix th.tr-vert > a { display: block; transform: rotate(-90deg); transform-origin: left bottom; width: 20px; color: #fff; }
.tr-matrix th.tr-vert > a > span { display: inline-block; width: 190px; text-align: left; margin-left: 0; }
"""


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("artifacts_dir")
    parser.add_argument("output_dir")
    parser.add_argument("--features", default=model.FEATURES_DIR, help="feature files, for the tags a skip is explained by")
    parser.add_argument("--registry", default="CMakeLists.txt", help="the CMake file declaring SOLIDSYSLOG_PLATFORM_REGISTRY")
    parser.add_argument("--source-url", default=None, help="prefix for links to a test's source, ending in a slash")
    args = parser.parse_args(argv)

    run = model.read_run(args.artifacts_dir, features_dir=args.features)
    if not run.lanes:
        print(f"no junit-* artifacts under {args.artifacts_dir}", file=sys.stderr)
        return 1
    with open(args.registry, encoding="utf-8") as handle:
        tokens = model.registry_tokens(handle.read())
    report = Report(run, tokens, args.source_url, docs_provenance.provenance(os.environ))
    written = report.write(args.output_dir)
    total = model.count(run.tests)
    print(f"{written} files: {len(run.lanes)} lanes, {total.total} tests, {total.failed} failed, {total.skipped} skipped")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
