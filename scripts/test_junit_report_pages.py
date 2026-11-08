"""Tests for junit_report_pages.py: the report pages rendered from the model."""

import os
import tempfile
import unittest

import junit_report as model
import junit_report_pages as pages

SRC = "https://example.test/blob/abc/"


def unit(lane, suite, name, status="passed", time=0.001, line=10, executable="SolidSyslogTests", message=None):
    return model.Test(lane, executable, suite, name, status, time, f"Tests/SolidSyslog{suite}Test.cpp", line, message)


def scenario(lane, feature, name, status="passed", time=0.5, tags=("tcp",)):
    return model.Test(lane, "behave", feature, name, status, time, "Bdd/features/x.feature", None, None, tags)


def a_run():
    run = model.Run()
    run.lanes["unit-linux-gcc"] = model.Lane(
        "unit-linux-gcc",
        [
            unit("unit-linux-gcc", "Formatter", "WritesPri"),
            unit("unit-linux-gcc", "Formatter", "Escapes <sd>", "failed", 0.2, 42, message="Tests/SolidSyslogFormatterTest.cpp:44: expected <a> but was <b>"),
            unit("unit-linux-gcc", "PosixClock", "ReadsTheClock", "skipped", 0.0, 7),
        ],
    )
    run.lanes["unit-linux-clang"] = model.Lane("unit-linux-clang", [unit("unit-linux-clang", "Formatter", "WritesPri", time=61.5)])
    run.lanes["bdd-linux-openssl-syslogng"] = model.Lane(
        "bdd-linux-openssl-syslogng",
        [scenario("bdd-linux-openssl-syslogng", "TLS", "Handshake completes"), scenario("bdd-linux-openssl-syslogng", "TLS", "Weak cipher refused", "skipped", 0, ("tcp", "tls13"))],
    )
    return run


class Formatting(unittest.TestCase):
    def test_times_read_in_the_unit_that_fits(self):
        self.assertEqual(pages.fmt_time(0.0), "0 ms")
        self.assertEqual(pages.fmt_time(0.0004), "0 ms")
        self.assertEqual(pages.fmt_time(0.012), "12 ms")
        self.assertEqual(pages.fmt_time(1.25), "1.2 s")
        self.assertEqual(pages.fmt_time(295.2), "4 min 55 s")

    def test_a_slug_is_safe_for_an_anchor_and_a_file_name(self):
        self.assertEqual(pages.slug("BDD scenarios"), "bdd-scenarios")
        self.assertEqual(pages.slug("SolidSyslogTests-Formatter"), "solidsyslogtests-formatter")

    def test_a_source_link_points_at_the_line(self):
        test = unit("l", "Formatter", "x", line=42)
        self.assertEqual(pages.source(test, SRC), '<a class="tr-src" href="https://example.test/blob/abc/Tests/SolidSyslogFormatterTest.cpp#L42"><code>Tests/SolidSyslogFormatterTest.cpp:42</code></a>')

    def test_without_a_source_url_the_position_is_plain_text(self):
        self.assertEqual(pages.source(unit("l", "Formatter", "x", line=42), None), "<code>Tests/SolidSyslogFormatterTest.cpp:42</code>")


class Rendered(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.out = tempfile.TemporaryDirectory()
        cls.report = pages.Report(a_run(), ["Posix"], SRC, "Measured from main at 1234567.")
        cls.written = cls.report.write(cls.out.name)

    @classmethod
    def tearDownClass(cls):
        cls.out.cleanup()

    def read(self, relative):
        with open(os.path.join(self.out.name, relative), encoding="utf-8") as handle:
            return handle.read()

    def test_every_page_kind_is_written(self):
        for name in ["index.html", "failures.html", "skipped.html", "suites.html", "scenarios.html", "slowest.html", "report.css",
                     "summary.json", "lanes/unit-linux-gcc.html", "lanes/bdd-linux-openssl-syslogng.html", "packs/core.html", "packs/posix.html", "packs/bdd-scenarios.html"]:
            self.assertTrue(os.path.exists(os.path.join(self.out.name, name)), name)
        self.assertEqual(self.written, 14)

    def test_the_overview_leads_with_the_failure_count_and_links_to_it(self):
        out = self.read("index.html")
        self.assertIn("1 failed.", out)
        self.assertLess(out.index('id="failures"'), out.index('id="kinds"'))
        self.assertIn('<a href="failures.html">', out)

    def test_the_overview_groups_lanes_by_kind_and_carries_the_provenance(self):
        out = self.read("index.html")
        self.assertLess(out.index(">Unit</th>"), out.index(">BDD</th>"))
        self.assertIn('<a href="lanes/unit-linux-gcc.html">unit-linux-gcc</a>', out)
        self.assertIn("Measured from main at 1234567.", out)
        self.assertIn('<a href="packs/posix.html">Posix</a>', out)

    def test_every_page_carries_the_markers_the_dressing_hook_fills(self):
        for name in ["index.html", "lanes/unit-linux-gcc.html", "packs/core.html"]:
            out = self.read(name)
            for marker in ("<!-- site-styles -->", "<!-- site-header -->", "<!-- /site-header -->", "<!-- site-footer -->", "<!-- /site-footer -->"):
                self.assertIn(marker, out, name)

    def test_a_deeper_page_reaches_the_stylesheet_and_the_nav_through_its_parent(self):
        out = self.read("lanes/unit-linux-gcc.html")
        self.assertIn('href="../report.css"', out)
        self.assertIn('<a href="../failures.html">Failures</a>', out)

    def test_the_failures_page_shows_the_message_and_links_the_line(self):
        out = self.read("failures.html")
        self.assertIn("expected &lt;a&gt; but was &lt;b&gt;", out)
        self.assertIn('href="https://example.test/blob/abc/Tests/SolidSyslogFormatterTest.cpp#L42"', out)
        self.assertIn("Escapes &lt;sd&gt;", out)

    def test_the_lane_page_opens_a_failing_suite_and_folds_the_rest(self):
        out = self.read("lanes/unit-linux-gcc.html")
        self.assertIn('<details id="solidsyslogtests-formatter" open>', out)
        self.assertIn('<details id="solidsyslogtests-posixclock">', out)
        self.assertIn('<td class="tr-status tr-failed">FAIL</td>', out)
        self.assertIn('<td class="tr-status tr-skipped">skip</td>', out)

    def test_a_lane_page_says_where_it_sits_and_what_is_on_it(self):
        out = self.read("lanes/unit-linux-gcc.html")
        self.assertIn('<nav class="tr-crumbs"><a href="../index.html">Tests</a><span class="tr-crumbs__sep">/</span><a href="../index.html#lanes">Lanes</a>', out)
        self.assertIn('On this page: <a href="#solidsyslogtests">SolidSyslogTests</a>', out)

    def test_the_matrices_offer_a_jump_to_each_group(self):
        self.assertIn('<a href="#solidsyslogtests">SolidSyslogTests</a>', self.read("suites.html"))
        self.assertIn('<a href="#tls">TLS</a>', self.read("scenarios.html"))
        self.assertIn('<tr class="tr-group" id="tls">', self.read("scenarios.html"))

    def test_the_bdd_lane_page_shows_tags_in_place_of_source(self):
        out = self.read("lanes/bdd-linux-openssl-syslogng.html")
        self.assertIn('<span class="tr-tag">@tls13</span>', out)
        self.assertIn("<th>Tags</th>", out)

    def test_the_skipped_page_lists_each_skip_with_its_tags_under_its_lane(self):
        out = self.read("skipped.html")
        self.assertIn("Weak cipher refused", out)
        self.assertIn("ReadsTheClock", out)
        self.assertIn("@tls13", out)

    def test_the_suites_matrix_counts_per_lane_and_dashes_where_a_lane_lacks_the_suite(self):
        out = self.read("suites.html")
        self.assertIn('<a href="lanes/unit-linux-gcc.html#solidsyslogtests-formatter">2</a>', out)
        self.assertIn('<a href="lanes/unit-linux-clang.html#solidsyslogtests-formatter">1</a>', out)
        self.assertIn('<td class="tr-num tr-none">-</td>', out)
        self.assertNotIn("bdd-linux-openssl-syslogng", out)

    def test_the_scenarios_matrix_has_a_status_per_lane_under_each_feature(self):
        out = self.read("scenarios.html")
        self.assertIn(">TLS</th>", out)
        self.assertIn("Handshake completes</td>", out)
        self.assertIn('<td class="tr-status tr-skipped">skip</td>', out)

    def test_the_pack_page_counts_by_lane_and_names_the_suites(self):
        out = self.read("packs/posix.html")
        self.assertIn('<a href="../lanes/unit-linux-gcc.html">unit-linux-gcc</a>', out)
        self.assertIn("PosixClock", out)

    def test_the_slowest_page_puts_the_longest_test_first(self):
        out = self.read("slowest.html")
        self.assertLess(out.index("1 min 2 s"), out.index("200 ms"))


class WithoutFailures(unittest.TestCase):
    def test_the_overview_and_failures_page_say_so(self):
        run = model.Run()
        run.lanes["unit-linux-gcc"] = model.Lane("unit-linux-gcc", [unit("unit-linux-gcc", "Formatter", "WritesPri")])
        report = pages.Report(run, [], None, "p")
        self.assertIn("No failures in this run.", report.overview())
        self.assertIn("No failures in this run.", report.failures_page())
        self.assertIn("Nothing was skipped in this run.", report.skipped_page())


if __name__ == "__main__":
    unittest.main()
