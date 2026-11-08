#!/usr/bin/env python3
"""Render the test report: what a CI run tested, organised so it can be read.

Reads the ``junit-<lane>`` artifacts of one run (``junit_report.py``) and writes
a directory of static pages that the documentation site publishes under its
Testing tab and the offline bundle carries. The one script is the expand-all
control on a page of folds, inlined so the bundle fetches nothing, which is
what ``scripts/check_offline_bundle.py`` holds the bundle to. The pages drill
down:

    index.html      Overview: headline counts, failures first, then each kind
                    of testing with its lanes, and each pack.
    <kind>.html     One kind - unit, integration, build or BDD - with its lanes
                    and the view across them: for unit-style kinds, which lanes
                    ran each suite; for BDD, every feature's prose and
                    scenarios against every target.
    lanes/<lane>    One lane. Unit-style: each executable's suites, each
                    opening to its tests with status, duration and a source
                    link. BDD: each feature's prose and its scenarios, each
                    opening to its steps as they ran - living documentation.
    packs/<pack>    What ran for one pack, lane by lane.
    failures.html   Every failure with its message and a link to the line.
    skipped.html    Every skip by lane, each with the tags that explain it.
    slowest.html    The slowest tests and suites across the run.
    summary.json    The headline figures, for the Tests page that introduces
                    the report (hooks/test_report.py).

Every page carries the marker comments ``hooks/report_chrome.py`` fills with
the site's header and footer; opened from a build tree it keeps a plain header.

Usage:
    junit_report_pages.py <artifacts-dir> <output-dir>
        [--features Bdd/features] [--registry CMakeLists.txt]
        [--source-url https://github.com/<owner>/<repo>/blob/<commit>/]
"""

import argparse
import dataclasses
import html
import json
import os
import re
import sys
from collections import defaultdict
from typing import Dict, List, Optional, Tuple

import docs_provenance
import junit_report as model

KIND_ORDER = ["unit", "integration", "build", "bdd"]
KIND_NAMES = {"unit": "Unit", "integration": "Integration", "build": "Build", "bdd": "BDD"}
KIND_BLURB = {
    "unit": "CppUTest against the fakes, under each compiler and sanitiser the lanes run.",
    "integration": "CppUTest against a real upstream, where a fake would prove too little.",
    "build": "A build lane that also runs the unit tests it builds.",
    "bdd": "Gherkin scenarios driving a real target against the syslog oracle.",
}
STATUS_WORD = {"passed": "pass", "failed": "FAIL", "skipped": "skip", "undefined": "undefined", "untested": "untested", "error": "error"}
SLOWEST_TESTS = 100
SLOWEST_SUITES = 30

esc = html.escape


def fmt_time(seconds: float) -> str:
    """A duration in the unit that fits, rounded before the unit is chosen so
    a value just short of a boundary never reads as 1000 ms or 1 min 60 s."""
    milliseconds = int(round(seconds * 1000))
    if milliseconds < 1000:
        return f"{milliseconds} ms"
    tenths = int(round(seconds * 10))
    if tenths < 600:
        return f"{tenths // 10}.{tenths % 10} s"
    minutes, rest = divmod(int(round(seconds)), 60)
    return f"{minutes} min {rest} s"


def slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def kind_name(kind: str) -> str:
    return KIND_NAMES.get(kind, kind.capitalize())


def kinds_in(run: model.Run) -> List[str]:
    present = {lane.kind for lane in run.lanes.values()}
    return [k for k in KIND_ORDER if k in present] + sorted(present - set(KIND_ORDER))


def lanes_of_kind(run: model.Run, kind: str) -> List[model.Lane]:
    return [lane for lane in run.lanes.values() if lane.kind == kind]


def kind_href(kind: str, depth: int = 0) -> str:
    return f'{"../" * depth}{kind}.html'


def lane_href(lane: str, depth: int = 0) -> str:
    return f'{"../" * depth}lanes/{lane}.html'


def pack_href(pack: str, depth: int = 0) -> str:
    return f'{"../" * depth}packs/{slug(pack)}.html'


def suite_id(test: model.Test) -> str:
    return slug(f"{test.executable}-{test.suite}")


def scenario_id(test: model.Test) -> str:
    return slug(f"{test.suite}-{test.name}")


def anchor_of(test: model.Test) -> str:
    """Where a test sits on its lane page: a scenario block on a BDD lane, a
    suite block elsewhere."""
    return scenario_id(test) if model.kind_of(test.lane) == "bdd" else suite_id(test)


def source(test: model.Test, source_url: Optional[str]) -> str:
    if not test.file:
        return ""
    text = f"{test.file}:{test.line}" if test.line else test.file
    if not source_url:
        return f"<code>{esc(text)}</code>"
    anchor = f"#L{test.line}" if test.line else ""
    return f'<a class="tr-src" href="{esc(source_url)}{esc(test.file)}{anchor}"><code>{esc(text)}</code></a>'


def status_cell(status: str) -> str:
    return f'<td class="tr-status tr-{status}">{STATUS_WORD.get(status, status)}</td>'


def status_badge(status: str) -> str:
    return f'<span class="tr-badge tr-{status}">{STATUS_WORD.get(status, status)}</span>'


def counts_cells(counts: model.Counts) -> str:
    failed = f'<td class="tr-num tr-bad">{counts.failed}</td>' if counts.failed else '<td class="tr-num">0</td>'
    skipped = f'<td class="tr-num tr-note">{counts.skipped}</td>' if counts.skipped else '<td class="tr-num">0</td>'
    return (
        f'<td class="tr-num">{counts.total}</td><td class="tr-num">{counts.passed}</td>{failed}{skipped}'
        f'<td class="tr-num">{fmt_time(counts.time)}</td>'
    )


def counts_phrase(counts: model.Counts, noun: str = "tests") -> str:
    parts = [f"{counts.total} {noun}", f"{counts.passed} passed"]
    if counts.failed:
        parts.append(f'<span class="tr-bad">{counts.failed} failed</span>')
    if counts.skipped:
        parts.append(f'<span class="tr-note">{counts.skipped} skipped</span>')
    parts.append(fmt_time(counts.time))
    return ", ".join(parts)


COUNT_HEADERS = "<th>Tests</th><th>Passed</th><th>Failed</th><th>Skipped</th><th>Time</th>"


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

    # ---- shared furniture --------------------------------------------------

    def nav(self) -> List[Tuple[str, str]]:
        return (
            [("index.html", "Overview")]
            + [(kind_href(kind), kind_name(kind)) for kind in kinds_in(self.run)]
            + [("failures.html", "Failures"), ("skipped.html", "Skipped"), ("slowest.html", "Slowest")]
        )

    def page(self, title: str, body: str, depth: int, active: Optional[str] = None, crumbs=(), jump=(), lead: str = "", folds: bool = False) -> str:
        """One page. `crumbs` are (href, label) pairs leading here from the
        overview, rendered as a trail above the title; `jump` are (anchor,
        label) pairs for the sections on this page, rendered beneath it;
        `lead` is a line of HTML under the title saying what the page holds;
        `folds` adds the expand-all and collapse-all controls, the one place
        the report uses a script, and a few lines of its own at that."""
        rel = "../" * depth
        nav = "".join(
            f'<a href="{rel}{href}"{" class=\"tr-nav--active\"" if href == active else ""}>{esc(label)}</a>' for href, label in self.nav()
        )
        trail = ""
        if crumbs:
            trail = (
                '<nav class="tr-crumbs">'
                + "".join(f'<a href="{href}">{esc(label)}</a><span class="tr-crumbs__sep">/</span>' for href, label in crumbs)
                + f"<span>{esc(title)}</span></nav>"
            )
        on_page = ""
        if jump:
            on_page = '<p class="tr-jump">On this page: ' + " &middot; ".join(f'<a href="#{anchor}">{esc(label)}</a>' for anchor, label in jump) + "</p>"
        lead_html = f'<p class="tr-lead">{lead}</p>' if lead else ""
        controls = (
            '<p class="tr-controls"><button type="button" data-tr-fold="open">Expand all</button> '
            '<button type="button" data-tr-fold="close">Collapse all</button></p>'
            if folds
            else ""
        )
        script = FOLD_SCRIPT if folds else ""
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
    {trail}
    <h1>{esc(title)}</h1>
    <p class="tr-prov">{esc(self.provenance)}</p>
    {lead_html}
    {on_page}
    {controls}
{body}
  </main>
  {script}
  <!-- site-footer -->
  <footer class="ss-footer">Part of the SolidSyslog documentation. Reported from the run's JUnit results.</footer>
  <!-- /site-footer -->
</body>
</html>
"""

    def pack(self, test: model.Test) -> str:
        return self.pack_of[test.file]

    def packs(self) -> Dict[str, Dict[str, List[model.Test]]]:
        grouped: Dict[str, Dict[str, List[model.Test]]] = defaultdict(lambda: defaultdict(list))
        for test in self.run.tests:
            grouped[self.pack(test)][test.lane].append(test)
        return grouped

    def tags(self, tags) -> str:
        return " ".join(f'<span class="tr-tag">@{esc(tag)}</span>' for tag in tags)

    def feature_of(self, test: model.Test) -> Optional[model.Feature]:
        return self.run.features.get(test.feature_stem or "")

    def lanes_table(self, lanes: List[model.Lane], depth: int = 0, noun: str = "Suites") -> str:
        rows = []
        for lane in lanes:
            executables = len({t.executable for t in lane.tests})
            suites = len({(t.executable, t.suite) for t in lane.tests})
            rows.append(
                f'<tr><td><a href="{lane_href(lane.name, depth)}">{esc(lane.name)}</a></td>'
                f'<td class="tr-num">{executables}</td><td class="tr-num">{suites}</td>{counts_cells(lane.counts)}</tr>'
            )
        return (
            f'<table class="tr-table"><thead><tr><th>Lane</th><th>Executables</th><th>{noun}</th>{COUNT_HEADERS}</tr></thead>'
            f'<tbody>{"".join(rows)}</tbody></table>'
        )

    # ---- overview ----------------------------------------------------------

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
                f'<a href="failures.html">Every failure with its message and line.</a></p>' + self.failure_table(self.failures[:20], depth=0)
            )
        else:
            failures = '<h2 id="failures">Failures</h2><p class="tr-good">No failures in this run.</p>'
        kind_sections = []
        for kind in kinds_in(run):
            lanes = lanes_of_kind(run, kind)
            counts = model.count(t for lane in lanes for t in lane.tests)
            kind_sections.append(
                f'<h2 id="{kind}"><a href="{kind_href(kind)}">{kind_name(kind)}</a> <span class="tr-muted">{counts_phrase(counts)}</span></h2>'
                f'<p>{esc(KIND_BLURB.get(kind, ""))} <a href="{kind_href(kind)}">The {kind_name(kind)} page</a> has the view across these lanes.</p>'
                + self.lanes_table(lanes)
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
    {"".join(kind_sections)}
    <h2 id="packs">By pack</h2>
    <p>Tests attributed by the test file that holds them. A pack's figure counts every lane that ran it.</p>
    <table class="tr-table"><thead><tr><th>Pack</th><th>Lanes</th>{COUNT_HEADERS}</tr></thead><tbody>{"".join(pack_rows)}</tbody></table>
"""
        jump = [("failures", "Failures")] + [(kind, kind_name(kind)) for kind in kinds_in(run)] + [("packs", "By pack")]
        return self.page("Tests", body, 0, "index.html", jump=jump)

    # ---- kind pages --------------------------------------------------------

    def kind_page(self, kind: str) -> str:
        lanes = lanes_of_kind(self.run, kind)
        counts = model.count(t for lane in lanes for t in lane.tests)
        if kind == "bdd":
            across, jump = self.features_across(lanes)
            across_title = "Features across targets"
            noun = "Features"
        else:
            across, jump = self.suites_across(lanes)
            across_title = "Packs and suites across lanes"
            noun = "Suites"
        body = (
            f'<h2 id="lanes">Lanes</h2>{self.lanes_table(lanes, noun=noun)}'
            f'<h2 id="across">{across_title}</h2>{across}'
        )
        crumbs = [("index.html", "Tests")]
        lead = f"{esc(KIND_BLURB.get(kind, ''))} {counts_phrase(counts)} across {len(lanes)} lanes."
        return self.page(kind_name(kind), body, 0, kind_href(kind), crumbs=crumbs, jump=[("lanes", "Lanes"), ("across", across_title)] + jump, lead=lead, folds=True)

    def suites_across(self, lanes: List[model.Lane]) -> Tuple[str, List[Tuple[str, str]]]:
        """A pack-by-lane table answering where each pack is tested, each pack
        folding open to its suites across the same lanes."""
        per_pack: Dict[str, Dict[Tuple[str, str], Dict[str, Tuple[model.Counts, str]]]] = defaultdict(lambda: defaultdict(dict))
        pack_lane_counts: Dict[str, Dict[str, model.Counts]] = defaultdict(dict)
        for lane in lanes:
            for (executable, suite), cases in by(lane.tests, lambda t: (t.executable, t.suite)).items():
                pack = self.pack(cases[0])
                per_pack[pack][(executable, suite)][lane.name] = (model.count(cases), suite_id(cases[0]))
                counts = pack_lane_counts[pack].setdefault(lane.name, model.Counts())
                for t in cases:
                    counts.add(t)
        head = "".join(f'<th class="tr-vert"><a href="{lane_href(lane.name)}"><span>{esc(lane.name)}</span></a></th>' for lane in lanes)

        def cell(found) -> str:
            if not found:
                return '<td class="tr-num tr-none">-</td>'
            counts, href = found
            cls = " tr-bad" if counts.failed else ""
            return f'<td class="tr-num{cls}"><a href="{href}">{counts.total}</a></td>'

        pack_rows = []
        folds = []
        for pack in sorted(per_pack, key=str.lower):
            cells = "".join(
                cell((pack_lane_counts[pack][lane.name], lane_href(lane.name)) if lane.name in pack_lane_counts[pack] else None) for lane in lanes
            )
            pack_rows.append(f'<tr><td><a href="#pack-{slug(pack)}">{esc(pack)}</a></td>{cells}</tr>')
            suite_rows = []
            current = None
            for (executable, suite), per_lane in sorted(per_pack[pack].items()):
                if executable != current:
                    current = executable
                    suite_rows.append(f'<tr class="tr-group"><th colspan="{len(lanes) + 1}">{esc(executable)}</th></tr>')
                suite_cells = "".join(
                    cell((per_lane[lane.name][0], f"{lane_href(lane.name)}#{per_lane[lane.name][1]}") if lane.name in per_lane else None) for lane in lanes
                )
                suite_rows.append(f"<tr><td>{esc(suite)}</td>{suite_cells}</tr>")
            total = sum(c.total for c in pack_lane_counts[pack].values())
            folds.append(
                f'<details id="pack-{slug(pack)}"><summary><strong>{esc(pack)}</strong> <span class="tr-muted">{len(per_pack[pack])} suites, '
                f'{total} test results across {len(pack_lane_counts[pack])} lanes</span> &middot; <a href="{pack_href(pack)}">pack page</a></summary>'
                f'<table class="tr-table tr-matrix"><thead><tr><th>Suite</th>{head}</tr></thead><tbody>{"".join(suite_rows)}</tbody></table></details>'
            )
        table = (
            "<p>Each cell is how many of the pack's tests that lane ran, linking to the lane; a dash means the lane does not "
            "build the pack. Open a pack for its suites across the same lanes, each cell linking to the suite as it ran there.</p>"
            f'<table class="tr-table tr-matrix"><thead><tr><th>Pack</th>{head}</tr></thead><tbody>{"".join(pack_rows)}</tbody></table>'
            + "".join(folds)
        )
        jump = [(f"pack-{slug(pack)}", pack) for pack in sorted(per_pack, key=str.lower)]
        return table, jump

    def features_across(self, lanes: List[model.Lane]) -> Tuple[str, List[Tuple[str, str]]]:
        features: Dict[str, Dict[str, Dict[str, model.Test]]] = defaultdict(lambda: defaultdict(dict))
        for lane in lanes:
            for t in lane.tests:
                features[t.suite][t.name][lane.name] = t
        head = "".join(f'<th class="tr-vert"><a href="{lane_href(lane.name)}"><span>{esc(lane.name)}</span></a></th>' for lane in lanes)
        sections = []
        for feature_name, scenarios in sorted(features.items()):
            any_test = next(iter(next(iter(scenarios.values())).values()))
            feature = self.feature_of(any_test)
            prose = self.feature_prose(feature)
            rows = []
            for name, per_lane in scenarios.items():
                first = next(iter(per_lane.values()))
                cells = "".join(
                    f'<td class="tr-status tr-{per_lane[lane.name].status}"><a href="{lane_href(lane.name)}#{scenario_id(first)}">{STATUS_WORD[per_lane[lane.name].status]}</a></td>'
                    if lane.name in per_lane
                    else '<td class="tr-none">-</td>'
                    for lane in lanes
                )
                rows.append(f"<tr><td>{esc(name)}</td><td>{self.tags(first.tags)}</td>{cells}</tr>")
            sections.append(
                f'<section class="tr-feature" id="{slug(feature_name)}"><h3>{esc(feature_name)} {self.tags(feature.feature_tags) if feature else ""}</h3>{prose}'
                f'<table class="tr-table tr-matrix"><thead><tr><th>Scenario</th><th>Tags</th>{head}</tr></thead><tbody>{"".join(rows)}</tbody></table></section>'
            )
        intro = (
            "<p>Every feature as its file describes it, and every scenario against every target it was offered to; each "
            "cell links to the scenario as it ran there. A skip is the target's tag filter or a <code>@requires_</code> "
            "tag, and the tags column is what decided it.</p>"
        )
        jump = [(slug(name), name) for name in sorted(features)]
        return intro + "".join(sections), jump

    def feature_prose(self, feature: Optional[model.Feature]) -> str:
        if not feature or not feature.description:
            return ""
        return f'<p class="tr-prose">{esc(feature.description).replace(chr(10), "<br>")}</p>'

    # ---- lane pages --------------------------------------------------------

    def lane_page(self, lane: model.Lane) -> str:
        counts = lane.counts
        kind = lane.kind
        if kind == "bdd":
            body, jump = self.bdd_lane_body(lane)
            noun = "scenarios"
        else:
            body, jump = self.unit_lane_body(lane)
            noun = "tests"
        crumbs = [("../index.html", "Tests"), (kind_href(kind, 1), kind_name(kind))]
        lead = f'<span class="tr-kind">{kind_name(kind)}</span> {counts_phrase(counts, noun)}.'
        return self.page(lane.name, body, 1, crumbs=crumbs, jump=jump, lead=lead, folds=True)

    def unit_lane_body(self, lane: model.Lane) -> Tuple[str, List[Tuple[str, str]]]:
        sections = []
        executables = sorted(by(lane.tests, lambda t: t.executable).items())
        for executable, tests in executables:
            suites = sorted(by(tests, lambda t: t.suite).items())
            ex_counts = model.count(tests)
            blocks = []
            for suite, cases in suites:
                c = model.count(cases)
                rows = "".join(
                    f"<tr>{status_cell(t.status)}<td>{esc(t.name)}</td><td class=\"tr-num\">{fmt_time(t.time)}</td><td>{source(t, self.source_url)}</td></tr>"
                    for t in cases
                )
                flags = (f' <span class="tr-bad">{c.failed} failed</span>' if c.failed else "") + (f' <span class="tr-note">{c.skipped} skipped</span>' if c.skipped else "")
                where = f" &middot; {source(dataclasses.replace(cases[0], line=None), self.source_url)}" if cases[0].file else ""
                blocks.append(
                    f'<details id="{suite_id(cases[0])}"{" open" if c.failed else ""}><summary><strong>{esc(suite)}</strong> '
                    f'<span class="tr-muted">{c.total} tests, {fmt_time(c.time)}</span>{flags}{where}</summary>'
                    f'<table class="tr-table"><thead><tr><th>Status</th><th>Test</th><th>Time</th><th>Source</th></tr></thead><tbody>{rows}</tbody></table></details>'
                )
            sections.append(
                f'<h2 id="{slug(executable)}">{esc(executable)} <span class="tr-muted">{len(suites)} suites, {counts_phrase(ex_counts)}</span></h2>' + "".join(blocks)
            )
        jump = [(slug(executable), executable) for executable, _ in executables]
        return "".join(sections), jump

    def bdd_lane_body(self, lane: model.Lane) -> Tuple[str, List[Tuple[str, str]]]:
        sections = []
        features = sorted(by(lane.tests, lambda t: t.suite).items())
        for feature_name, scenarios in features:
            feature = self.feature_of(scenarios[0])
            c = model.count(scenarios)
            blocks = []
            for t in scenarios:
                steps = "".join(
                    f'<li class="tr-step tr-step--{s.status}"><span class="tr-step__kw">{esc(s.keyword)}</span> {esc(s.text)} '
                    f'<span class="tr-muted">{fmt_time(s.time)}</span></li>'
                    for s in t.steps
                )
                detail = f'<ul class="tr-steps">{steps}</ul>' if steps else '<p class="tr-muted">No steps were recorded for this scenario.</p>'
                if t.message:
                    detail += f"<pre>{esc(t.message)}</pre>"
                blocks.append(
                    f'<details id="{scenario_id(t)}"{" open" if t.status == "failed" else ""}><summary>{status_badge(t.status)} {esc(t.name)} '
                    f'{self.tags(t.tags)} <span class="tr-muted">{fmt_time(t.time)}</span></summary>{detail}</details>'
                )
            sections.append(
                f'<section class="tr-feature" id="{slug(feature_name)}"><h2>{esc(feature_name)} {self.tags(feature.feature_tags) if feature else ""} '
                f'<span class="tr-muted">{counts_phrase(c, "scenarios")}</span></h2>{self.feature_prose(feature)}{"".join(blocks)}</section>'
            )
        jump = [(slug(name), name) for name, _ in features]
        return "".join(sections), jump

    # ---- cross-cutting pages ----------------------------------------------

    def failure_table(self, failures: List[model.Test], depth: int) -> str:
        rows = "".join(
            f'<tr><td><a href="{lane_href(t.lane, depth)}#{anchor_of(t)}">{esc(t.lane)}</a></td><td>{esc(t.suite)}</td>'
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
                f'<p><a href="{lane_href(t.lane)}#{anchor_of(t)}">{esc(t.lane)}</a>, {esc(t.executable)} &middot; {source(t, self.source_url)} &middot; {fmt_time(t.time)}</p>'
                f'<pre>{esc(t.message or "")}</pre></section>'
                for t in self.failures
            )
            body = f'<p class="tr-bad">{len(self.failures)} failed across {len({t.lane for t in self.failures})} lane(s).</p>{items}'
        return self.page("Failures", body, 0, "failures.html", crumbs=[("index.html", "Tests")])

    def skipped_page(self) -> str:
        if not self.skips:
            body = "<p>Nothing was skipped in this run.</p>"
            jump = []
        else:
            sections = []
            grouped = by(self.skips, lambda t: t.lane)
            for lane_name, tests in grouped.items():
                rows = "".join(
                    f'<tr><td>{esc(t.suite)}</td><td><a href="{lane_href(lane_name)}#{anchor_of(t)}">{esc(t.name)}</a></td><td>{self.tags(t.tags)}</td></tr>'
                    for t in tests
                )
                sections.append(
                    f'<h2 id="{slug(lane_name)}"><a href="{lane_href(lane_name)}">{esc(lane_name)}</a> <span class="tr-muted">{len(tests)} skipped</span></h2>'
                    f'<table class="tr-table"><thead><tr><th>Feature or suite</th><th>Scenario or test</th><th>Tags</th></tr></thead><tbody>{rows}</tbody></table>'
                )
            body = (
                "<p>A scenario is skipped when the lane's tag filter excludes it, or a <code>@requires_</code> tag "
                "names a tunable the target is built below. The tags are what the filter saw.</p>" + "".join(sections)
            )
            jump = [(slug(lane), lane) for lane in grouped]
        return self.page("Skipped", body, 0, "skipped.html", crumbs=[("index.html", "Tests")], jump=jump)

    def pack_page(self, pack: str, lanes: Dict[str, List[model.Test]]) -> str:
        all_tests = [t for tests in lanes.values() for t in tests]
        total = model.count(all_tests)
        lane_rows = "".join(
            f'<tr><td><a href="{lane_href(lane, 1)}">{esc(lane)}</a></td><td class="tr-num">{len({(t.executable, t.suite) for t in tests})}</td>{counts_cells(model.count(tests))}</tr>'
            for lane, tests in sorted(lanes.items())
        )
        # One grouping pass: suite -> lane -> its first test there, and the
        # distinct names per suite, rather than a scan of the pack per suite.
        first_in: Dict[str, Dict[str, model.Test]] = defaultdict(dict)
        names: Dict[str, set] = defaultdict(set)
        for lane, tests in lanes.items():
            for t in tests:
                first_in[t.suite].setdefault(lane, t)
                names[t.suite].add(t.name)
        suite_rows = []
        for suite, per_lane in sorted(first_in.items()):
            links = [f'<a href="{lane_href(lane, 1)}#{anchor_of(first)}">{esc(lane)}</a>' for lane, first in sorted(per_lane.items())]
            suite_rows.append(f'<tr><td>{esc(suite)}</td><td class="tr-num">{len(names[suite])}</td><td>{", ".join(links)}</td></tr>')
        files = sorted({t.file for t in all_tests if t.file})
        body = f"""
    <h2 id="lanes">By lane</h2>
    <table class="tr-table"><thead><tr><th>Lane</th><th>Suites</th>{COUNT_HEADERS}</tr></thead><tbody>{lane_rows}</tbody></table>
    <h2 id="suites">Suites</h2>
    <p>Distinct tests per suite, and the lanes that ran it.</p>
    <table class="tr-table"><thead><tr><th>Suite</th><th>Tests</th><th>Lanes</th></tr></thead><tbody>{"".join(suite_rows)}</tbody></table>
"""
        crumbs = [("../index.html", "Tests"), ("../index.html#packs", "Packs")]
        lead = f"{total.total} test results across {len(lanes)} lanes, from {len(files)} test files."
        return self.page(pack, body, 1, crumbs=crumbs, jump=[("lanes", "By lane"), ("suites", "Suites")], lead=lead)

    def slowest_page(self) -> str:
        tests = sorted(self.run.tests, key=lambda t: t.time, reverse=True)[:SLOWEST_TESTS]
        test_rows = "".join(
            f'<tr><td class="tr-num">{fmt_time(t.time)}</td><td><a href="{lane_href(t.lane)}#{anchor_of(t)}">{esc(t.lane)}</a></td>'
            f'<td>{esc(t.suite)}</td><td>{esc(t.name)}</td>{status_cell(t.status)}</tr>'
            for t in tests
        )
        suites = sorted(
            ((model.count(cases), key, cases[0]) for key, cases in by(self.run.tests, lambda t: (t.lane, t.executable, t.suite)).items()),
            key=lambda item: item[0].time,
            reverse=True,
        )[:SLOWEST_SUITES]
        suite_rows = "".join(
            f'<tr><td class="tr-num">{fmt_time(c.time)}</td><td><a href="{lane_href(lane)}#{slug(suite) if model.kind_of(lane) == "bdd" else suite_id(first)}">{esc(lane)}</a></td>'
            f'<td>{esc(suite)}</td><td class="tr-num">{c.total}</td></tr>'
            for c, (lane, _, suite), first in suites
        )
        lane_rows = "".join(
            f'<tr><td><a href="{lane_href(lane.name)}">{esc(lane.name)}</a></td><td class="tr-num">{lane.counts.total}</td><td class="tr-num">{fmt_time(lane.counts.time)}</td></tr>'
            for lane in sorted(self.run.lanes.values(), key=lambda lane: lane.counts.time, reverse=True)
        )
        body = f"""
    <h2 id="lanes">Test time by lane</h2>
    <table class="tr-table"><thead><tr><th>Lane</th><th>Tests</th><th>Time</th></tr></thead><tbody>{lane_rows}</tbody></table>
    <h2 id="suites">Slowest suites</h2>
    <table class="tr-table"><thead><tr><th>Time</th><th>Lane</th><th>Suite</th><th>Tests</th></tr></thead><tbody>{suite_rows}</tbody></table>
    <h2 id="tests">Slowest tests</h2>
    <table class="tr-table"><thead><tr><th>Time</th><th>Lane</th><th>Suite</th><th>Test</th><th>Status</th></tr></thead><tbody>{test_rows}</tbody></table>
"""
        lead = "Durations as each test runner reported them; CppUTest reports to the millisecond."
        jump = [("lanes", "Test time by lane"), ("suites", "Slowest suites"), ("tests", "Slowest tests")]
        return self.page("Slowest", body, 0, "slowest.html", crumbs=[("index.html", "Tests")], jump=jump, lead=lead)

    # ---- writing -----------------------------------------------------------

    def write(self, out_dir: str) -> int:
        written = 0
        written += self._write(out_dir, "index.html", self.overview())
        for kind in kinds_in(self.run):
            written += self._write(out_dir, kind_href(kind), self.kind_page(kind))
        written += self._write(out_dir, "failures.html", self.failures_page())
        written += self._write(out_dir, "skipped.html", self.skipped_page())
        written += self._write(out_dir, "slowest.html", self.slowest_page())
        for lane in self.run.lanes.values():
            written += self._write(out_dir, f"lanes/{lane.name}.html", self.lane_page(lane))
        for pack, lanes in self.packs().items():
            written += self._write(out_dir, f"packs/{slug(pack)}.html", self.pack_page(pack, lanes))
        written += self._write(out_dir, "report.css", CSS)
        written += self._write(out_dir, "summary.json", json.dumps(self.summary(), indent=1))
        return written

    def summary(self) -> dict:
        """The figures the Tests page shows above its links into the report,
        read by hooks/test_report.py at the documentation build."""

        def counts(c: model.Counts) -> dict:
            return {"total": c.total, "passed": c.passed, "failed": c.failed, "skipped": c.skipped, "time": round(c.time, 3)}

        return {
            "provenance": self.provenance,
            "totals": counts(model.count(self.run.tests)),
            "kinds": [
                {
                    "kind": kind,
                    "name": kind_name(kind),
                    "href": kind_href(kind),
                    "lanes": len(lanes_of_kind(self.run, kind)),
                    **counts(model.count(t for lane in lanes_of_kind(self.run, kind) for t in lane.tests)),
                }
                for kind in kinds_in(self.run)
            ],
            "lanes": [{"name": lane.name, "kind": lane.kind, **counts(lane.counts)} for lane in self.run.lanes.values()],
        }

    @staticmethod
    def _write(out_dir: str, relative: str, text: str) -> int:
        path = os.path.join(out_dir, relative)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8", newline="\n") as handle:
            handle.write(text)
        return 1


# The one script in the report: the expand-all and collapse-all buttons on a
# page of folds. Inline, so the offline bundle fetches nothing, and the page
# reads the same without it.
FOLD_SCRIPT = """<script>
document.querySelectorAll("[data-tr-fold]").forEach(function (button) {
  button.addEventListener("click", function () {
    var open = button.getAttribute("data-tr-fold") === "open";
    document.querySelectorAll("main details").forEach(function (fold) { fold.open = open; });
  });
});
</script>"""

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
.tr h3 { font-size: 1.05em; margin: 20px 0 8px; }
.tr a { color: #3e2199; text-decoration: none; }
.tr a:hover { text-decoration: underline; }
.tr-nav { display: flex; flex-wrap: wrap; gap: 4px 18px; padding: 8px 0; border-bottom: 1px solid #e0e0e0; font-size: 0.95em; }
.tr-nav a { color: #3e2199; }
.tr-nav--active { font-weight: 700; }
.tr-prov { color: #7a7a7a; margin: 0 0 8px; }
.tr-lead { margin: 0 0 8px; }
.tr-crumbs { margin: 10px 0 0; font-size: 0.92em; color: #5f5f5f; }
.tr-crumbs__sep { margin: 0 6px; color: #aaa; }
.tr-jump { margin: 0 0 16px; padding: 6px 10px; background: #f7f6fb; border-radius: 6px; font-size: 0.95em; }
.tr-controls { margin: 0 0 12px; }
.tr-controls button { font: inherit; font-size: 0.9em; padding: 2px 10px; border: 1px solid #c9c2e6; border-radius: 6px; background: #f7f6fb; color: #3e2199; cursor: pointer; }
.tr-controls button:hover { background: #ece8f7; }
.tr-matrix thead th { position: sticky; top: 0; z-index: 1; }
.tr-matrix tbody tr:hover td { background: #f7f6fb; }
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
.tr-status a { color: inherit; }
.tr-badge { display: inline-block; min-width: 3em; text-align: center; padding: 0 6px; border-radius: 8px; font-size: 0.85em; font-weight: 700; margin-right: 4px; }
.tr-badge.tr-passed { background: #e3f3e8; }
.tr-badge.tr-failed { background: #fbe4e2; }
.tr-badge.tr-skipped { background: #fbf1dc; }
.tr-passed { color: #1b7f3b; }
.tr-failed { color: #b3261e; }
.tr-skipped { color: #946200; }
.tr-bad { color: #b3261e; font-weight: 700; }
.tr-good { color: #1b7f3b; }
.tr-note { color: #946200; }
.tr-none { color: #bbb; text-align: center !important; }
.tr-muted { color: #7a7a7a; font-weight: 400; font-size: 0.92em; }
.tr-kind { display: inline-block; padding: 1px 8px; border-radius: 10px; background: #f1eefa; color: #3e2199; font-size: 0.9em; margin-right: 6px; }
.tr-tag { display: inline-block; padding: 0 6px; border-radius: 8px; background: #f1f1f1; color: #444; font-size: 0.85em; margin: 1px 2px 1px 0; white-space: nowrap; font-weight: 400; }
.tr-src code, .tr code { font-size: 0.9em; }
.tr details { margin: 6px 0; border: 1px solid #e8e8e8; border-radius: 6px; padding: 4px 10px; }
.tr details[open] { padding-bottom: 8px; }
.tr summary { cursor: pointer; padding: 4px 0; }
.tr-feature { margin: 8px 0 24px; }
.tr-prose { margin: 4px 0 10px; color: #333; }
.tr-steps { margin: 6px 0 4px; padding-left: 0.6em; list-style: none; }
.tr-step { margin: 2px 0; }
.tr-step__kw { font-weight: 700; color: #3e2199; }
.tr-step--skipped { color: #946200; }
.tr-step--failed { color: #b3261e; font-weight: 700; }
.tr-step--undefined, .tr-step--untested { color: #7a7a7a; }
.tr-failure { margin: 16px 0; padding: 8px 12px; border-left: 4px solid #b3261e; background: #fcf6f6; }
.tr-failure pre { white-space: pre-wrap; margin: 6px 0 0; font-size: 0.9em; }
.tr pre { white-space: pre-wrap; font-size: 0.9em; }
.tr-matrix td, .tr-matrix th { padding: 3px 6px; }
.tr-matrix th.tr-vert { height: 200px; white-space: nowrap; vertical-align: bottom; padding: 0 2px; }
.tr-matrix th.tr-vert > a { display: block; transform: rotate(-90deg); transform-origin: left bottom; width: 20px; color: #fff; }
.tr-matrix th.tr-vert > a > span { display: inline-block; width: 190px; text-align: left; margin-left: 0; }
"""


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("artifacts_dir")
    parser.add_argument("output_dir")
    parser.add_argument("--features", default=model.FEATURES_DIR, help="feature files, for their prose and the tags a skip is explained by")
    parser.add_argument("--registry", default="CMakeLists.txt", help="the CMake file declaring SOLIDSYSLOG_PLATFORM_REGISTRY")
    parser.add_argument("--source-url", default=None, help="prefix for links to a test's source, ending in a slash")
    args = parser.parse_args(argv)

    run = model.read_run(args.artifacts_dir, features_dir=args.features)
    if not run.lanes:
        print(f"no junit-* artifacts under {args.artifacts_dir}", file=sys.stderr)
        return 1
    with open(args.registry, encoding="utf-8") as handle:
        tokens = model.registry_tokens(handle.read())
    if not tokens:
        # Without the registry every test would be attributed to Core, and the
        # pack views would be wrong under a green step.
        print(f"{args.registry} declares no SOLIDSYSLOG_PLATFORM_REGISTRY, so packs cannot be attributed", file=sys.stderr)
        return 1
    report = Report(run, tokens, args.source_url, docs_provenance.provenance(os.environ, verb="Tested"))
    written = report.write(args.output_dir)
    total = model.count(run.tests)
    print(f"{written} files: {len(run.lanes)} lanes, {total.total} tests, {total.failed} failed, {total.skipped} skipped")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
