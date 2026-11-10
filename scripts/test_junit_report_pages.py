"""Tests for junit_report_pages.py: the report pages rendered from the model."""

import json
import os
import tempfile
import unittest

import junit_report as model
import junit_report_pages as pages

SRC = "https://example.test/blob/abc/"


def unit(lane, suite, name, status="passed", time=0.001, line=10, executable="SolidSyslogTests", message=None):
    return model.Test(lane, executable, suite, name, status, time, f"Tests/SolidSyslog{suite}Test.cpp", line, message)


def scenario(lane, feature, name, status="passed", time=0.5, tags=("tcp",), steps=()):
    return model.Test(lane, "behave", feature, name, status, time, "Bdd/features/tls.feature", None, None, tags, steps)


STEPS = (model.Step("Given", "the oracle is running", "passed", 0.001), model.Step("When", "a message is sent", "passed", 0.25))


HOST_VERSIONS = model.Versions(
    "GNU", "12.2.0", "gcc", "Linux x86_64", False, ("Posix", "OpenSsl", "MbedTls"), "Linux",
    {
        "OpenSsl": (model.Upstream("OpenSSL", version="3.0.22"),),
        "MbedTls": (model.Upstream("MBEDTLS_DIR", version="3.6.7", tag="mbedtls-3.6.7", commit="068ff080b369adfac81509f9b57b2afabaf82dc5", path="/opt/mbedtls"),),
        "LittleFs": (model.Upstream("LITTLEFS_PATH", version="2.11", tag="v2.11.3", commit="6cb4e86540eca0d9ba62500a298385c9d863c8be"),),
    },
)

CROSS_VERSIONS = model.Versions(
    "GNU", "12.2.1", "arm-none-eabi-gcc", "Generic arm", True, ("FreeRtos", "PlusFat"), "FREERTOS_PLUSTCP",
    {
        "FreeRtos": (model.Upstream("FREERTOS_KERNEL_PATH", version="V11.3.3", tag="V11.3.3", commit="89592801dd8b73329a7583719b7e3567b50910e5"),),
        "PlusFat": (model.Upstream("FREERTOS_PLUS_FAT_PATH", commit="9ca465c3f2426c301b6b660286f3f964a2abdce5"),),
    },
)


def a_run():
    run = model.Run()
    run.features["tls"] = model.Feature("tls", "TLS", "The library speaks TLS to the oracle.\nWith pins.", ("tcp",), {"": ("tcp",)})
    run.lanes["unit-linux-gcc"] = model.Lane(
        "unit-linux-gcc",
        [
            unit("unit-linux-gcc", "Formatter", "WritesPri"),
            unit("unit-linux-gcc", "Formatter", "Escapes <sd>", "failed", 0.2, 42, message="Tests/SolidSyslogFormatterTest.cpp:44: expected <a> but was <b>"),
            unit("unit-linux-gcc", "PosixClock", "ReadsTheClock", "skipped", 0.0, 7),
        ],
        HOST_VERSIONS,
    )
    run.lanes["unit-linux-clang"] = model.Lane("unit-linux-clang", [unit("unit-linux-clang", "Formatter", "WritesPri", time=61.5)])
    run.lanes["integration-linux-openssl"] = model.Lane("integration-linux-openssl", [unit("integration-linux-openssl", "OpenSslStream", "Connects", executable="OpenSslIntegrationTests")])
    run.lanes["bdd-linux-openssl-syslogng"] = model.Lane(
        "bdd-linux-openssl-syslogng",
        [
            scenario("bdd-linux-openssl-syslogng", "TLS", "Handshake completes", steps=STEPS),
            scenario("bdd-linux-openssl-syslogng", "TLS", "Weak cipher refused", "skipped", 0, ("tcp", "tls13")),
        ],
        CROSS_VERSIONS,
    )
    return run


class Formatting(unittest.TestCase):
    def test_times_read_in_the_unit_that_fits(self):
        self.assertEqual(pages.fmt_time(0.0), "0 ms")
        self.assertEqual(pages.fmt_time(0.012), "12 ms")
        self.assertEqual(pages.fmt_time(1.25), "1.2 s")
        self.assertEqual(pages.fmt_time(295.2), "4 min 55 s")

    def test_a_time_just_short_of_a_unit_boundary_rounds_into_the_next_unit(self):
        self.assertEqual(pages.fmt_time(0.9996), "1.0 s")
        self.assertEqual(pages.fmt_time(59.96), "1 min 0 s")
        self.assertEqual(pages.fmt_time(119.6), "2 min 0 s")

    def test_a_slug_is_safe_for_an_anchor_and_a_file_name(self):
        self.assertEqual(pages.slug("BDD scenarios"), "bdd-scenarios")

    def test_a_source_link_points_at_the_line(self):
        test = unit("l", "Formatter", "x", line=42)
        self.assertEqual(pages.source(test, SRC), '<a class="tr-src" href="https://example.test/blob/abc/Tests/SolidSyslogFormatterTest.cpp#L42"><code>Tests/SolidSyslogFormatterTest.cpp:42</code></a>')

    def test_without_a_source_url_the_position_is_plain_text(self):
        self.assertEqual(pages.source(unit("l", "Formatter", "x", line=42), None), "<code>Tests/SolidSyslogFormatterTest.cpp:42</code>")


class Rendered(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.out = tempfile.TemporaryDirectory()
        cls.report = pages.Report(a_run(), ["Posix", "OpenSsl"], SRC, "Measured from main at 1234567.")
        cls.written = cls.report.write(cls.out.name)

    @classmethod
    def tearDownClass(cls):
        cls.out.cleanup()

    def read(self, relative):
        with open(os.path.join(self.out.name, relative), encoding="utf-8") as handle:
            return handle.read()

    def test_every_page_is_written(self):
        expected = [
            "index.html", "unit.html", "integration.html", "bdd.html", "failures.html", "skipped.html", "slowest.html",
            "report.css", "summary.json", "lanes/unit-linux-gcc.html", "lanes/bdd-linux-openssl-syslogng.html",
            "packs/core.html", "packs/posix.html", "packs/openssl.html", "packs/bdd-scenarios.html",
        ]
        for name in expected:
            self.assertTrue(os.path.exists(os.path.join(self.out.name, name)), name)
        self.assertEqual(self.written, 17)

    def test_the_nav_names_each_kind_present_in_order(self):
        out = self.read("index.html")
        self.assertIn('<a href="unit.html">Unit</a><a href="integration.html">Integration</a><a href="bdd.html">BDD</a>', out)

    def test_the_overview_leads_with_failures_then_each_kind_with_its_lanes(self):
        out = self.read("index.html")
        self.assertIn("1 failed.", out)
        self.assertLess(out.index('id="failures"'), out.index('id="unit"'))
        self.assertLess(out.index('id="unit"'), out.index('id="bdd"'))
        self.assertIn('<a href="lanes/unit-linux-gcc.html">unit-linux-gcc</a>', out)
        self.assertIn('<a href="packs/posix.html">Posix</a>', out)
        self.assertIn("Measured from main at 1234567.", out)

    def test_every_page_carries_the_markers_the_dressing_hook_fills(self):
        for name in ["index.html", "bdd.html", "lanes/unit-linux-gcc.html", "packs/core.html"]:
            out = self.read(name)
            for marker in ("<!-- site-styles -->", "<!-- site-header -->", "<!-- /site-header -->", "<!-- site-footer -->", "<!-- /site-footer -->"):
                self.assertIn(marker, out, name)

    def test_a_unit_kind_page_lists_its_lanes_and_where_each_suite_ran(self):
        out = self.read("unit.html")
        self.assertIn('<a href="lanes/unit-linux-gcc.html">unit-linux-gcc</a>', out)
        self.assertNotIn("integration-linux-openssl", out)
        self.assertIn('<a href="lanes/unit-linux-gcc.html#solidsyslogtests-formatter">2</a>', out)
        self.assertIn('<a href="lanes/unit-linux-clang.html#solidsyslogtests-formatter">1</a>', out)
        self.assertIn('<td class="tr-num tr-none">-</td>', out)

    def test_a_unit_kind_page_leads_with_packs_by_lane_and_folds_each_packs_suites(self):
        out = self.read("unit.html")
        self.assertIn('<tr><td><a href="#pack-posix">Posix</a></td>', out)
        self.assertIn('<details id="pack-posix"><summary><strong>Posix</strong>', out)
        self.assertIn('<a href="packs/posix.html">pack page</a>', out)
        self.assertLess(out.index('<a href="#pack-core">Core</a>'), out.index('<details id="pack-core">'))

    def test_pages_of_folds_offer_expand_and_collapse_and_the_overview_does_not(self):
        for name in ["unit.html", "lanes/unit-linux-gcc.html", "lanes/bdd-linux-openssl-syslogng.html"]:
            out = self.read(name)
            self.assertIn('data-tr-fold="open">Expand all</button>', out, name)
            self.assertIn("<script>", out, name)
        self.assertNotIn("Expand all", self.read("index.html"))
        self.assertNotIn("<script>", self.read("index.html"))

    def test_steps_read_from_their_keyword_with_no_numbering(self):
        out = self.read("lanes/bdd-linux-openssl-syslogng.html")
        self.assertIn('<ul class="tr-steps">', out)
        self.assertNotIn("<ol", out)

    def test_the_bdd_kind_page_reads_as_living_documentation_across_targets(self):
        out = self.read("bdd.html")
        self.assertIn('<section class="tr-feature" id="tls">', out)
        self.assertIn("The library speaks TLS to the oracle.<br>With pins.", out)
        self.assertIn('<a href="lanes/bdd-linux-openssl-syslogng.html#tls-handshake-completes">pass</a>', out)
        self.assertIn('<a href="lanes/bdd-linux-openssl-syslogng.html#tls-weak-cipher-refused">skip</a>', out)
        self.assertIn('<span class="tr-tag">@tls13</span>', out)

    def test_a_unit_lane_page_has_one_block_per_suite_opening_to_its_tests(self):
        out = self.read("lanes/unit-linux-gcc.html")
        self.assertEqual(out.count("<details"), 2)
        self.assertIn('<details id="solidsyslogtests-formatter" open>', out)
        self.assertIn('<details id="solidsyslogtests-posixclock">', out)
        self.assertIn('<td class="tr-status tr-failed">FAIL</td>', out)
        self.assertIn('<nav class="tr-crumbs"><a href="../index.html">Tests</a><span class="tr-crumbs__sep">/</span><a href="../unit.html">Unit</a>', out)
        self.assertIn('On this page: <a href="#solidsyslogtests">SolidSyslogTests</a>', out)

    def test_a_bdd_lane_page_shows_each_feature_with_its_prose_scenarios_and_steps(self):
        out = self.read("lanes/bdd-linux-openssl-syslogng.html")
        self.assertIn('<section class="tr-feature" id="tls"><h2>TLS <span class="tr-tag">@tcp</span>', out)
        self.assertIn("The library speaks TLS to the oracle.<br>With pins.", out)
        self.assertIn('<details id="tls-handshake-completes"><summary><span class="tr-badge tr-passed">pass</span> Handshake completes', out)
        self.assertIn('<li class="tr-step tr-step--passed"><span class="tr-step__kw">Given</span> the oracle is running <span class="tr-muted">1 ms</span></li>', out)
        self.assertIn("No steps were recorded for this scenario.", out)
        self.assertIn('<a href="../bdd.html">BDD</a>', out)

    def test_the_failures_page_shows_the_message_and_links_the_line(self):
        out = self.read("failures.html")
        self.assertIn("expected &lt;a&gt; but was &lt;b&gt;", out)
        self.assertIn('href="https://example.test/blob/abc/Tests/SolidSyslogFormatterTest.cpp#L42"', out)

    def test_the_skipped_page_links_each_skip_into_its_lane_with_its_tags(self):
        out = self.read("skipped.html")
        self.assertIn('<a href="lanes/bdd-linux-openssl-syslogng.html#tls-weak-cipher-refused">Weak cipher refused</a>', out)
        self.assertIn('<a href="lanes/unit-linux-gcc.html#solidsyslogtests-posixclock">ReadsTheClock</a>', out)
        self.assertIn("@tls13", out)

    def test_the_pack_page_counts_by_lane_and_names_the_suites(self):
        out = self.read("packs/posix.html")
        self.assertIn('<a href="../lanes/unit-linux-gcc.html">unit-linux-gcc</a>', out)
        self.assertIn("PosixClock", out)

    def test_the_slowest_page_puts_the_longest_test_first(self):
        out = self.read("slowest.html")
        self.assertLess(out.index("1 min 2 s"), out.index("200 ms"))

    def test_the_overview_says_what_each_lane_compiled_against(self):
        out = self.read("index.html")
        self.assertIn('<h2 id="versions">Versions</h2>', out)
        self.assertIn('<tr><td><a href="lanes/unit-linux-gcc.html">unit-linux-gcc</a></td><td>GNU 12.2.0</td><td>LittleFs 2.11 (v2.11.3), MbedTls 3.6.7, OpenSsl 3.0.22</td></tr>', out)
        self.assertIn('<tr><td><a href="lanes/bdd-linux-openssl-syslogng.html">bdd-linux-openssl-syslogng</a></td><td>GNU 12.2.1 (arm-none-eabi-gcc)</td><td>FreeRtos V11.3.3, PlusFat 9ca465c</td></tr>', out)
        self.assertIn('<tr><td><a href="lanes/unit-linux-clang.html">unit-linux-clang</a></td><td class="tr-none" colspan="2">no record uploaded</td></tr>', out)
        self.assertIn('<a href="#versions">Versions</a>', out)

    def test_a_lane_page_lists_what_the_build_compiled_against(self):
        out = self.read("lanes/bdd-linux-openssl-syslogng.html")
        self.assertIn('<h2 id="versions">Built against</h2>', out)
        self.assertIn("<tr><th>Compiler</th><td>GNU 12.2.1 (arm-none-eabi-gcc), cross-compiling for Generic arm</td></tr>", out)
        self.assertIn("<tr><th>BDD target</th><td>FREERTOS_PLUSTCP</td></tr>", out)
        self.assertIn("<tr><th>Packs</th><td>FreeRtos, PlusFat</td></tr>", out)
        self.assertIn("<tr><th>FreeRtos</th><td>V11.3.3 <span class=\"tr-muted\">V11.3.3 at 8959280, FREERTOS_KERNEL_PATH</span></td></tr>", out)
        self.assertIn("<tr><th>PlusFat</th><td>9ca465c <span class=\"tr-muted\">9ca465c, FREERTOS_PLUS_FAT_PATH</span></td></tr>", out)
        host = self.read("lanes/unit-linux-gcc.html")
        self.assertIn("<tr><th>Compiler</th><td>GNU 12.2.0 (gcc) on Linux x86_64</td></tr>", host)
        self.assertIn("<tr><th>OpenSsl</th><td>3.0.22 <span class=\"tr-muted\">OpenSSL</span></td></tr>", host)

    def test_a_lane_without_a_record_says_so(self):
        out = self.read("lanes/unit-linux-clang.html")
        self.assertIn("No version record was uploaded with this lane.", out)

    def test_the_pack_page_shows_the_upstream_each_lane_built_the_pack_against(self):
        out = self.read("packs/openssl.html")
        self.assertIn("<th>Built against</th>", out)
        self.assertIn('<tr><td><a href="../lanes/integration-linux-openssl.html">integration-linux-openssl</a></td><td class="tr-none">-</td>', out)

    def test_the_summary_carries_each_lanes_compiler(self):
        summary = json.loads(self.read("summary.json"))
        lanes = {lane["name"]: lane for lane in summary["lanes"]}
        self.assertEqual(lanes["unit-linux-gcc"]["compiler"], "GNU 12.2.0")
        self.assertNotIn("compiler", lanes["unit-linux-clang"])

    def test_the_summary_names_each_kind_with_its_page(self):
        summary = json.loads(self.read("summary.json"))
        self.assertEqual([k["href"] for k in summary["kinds"]], ["unit.html", "integration.html", "bdd.html"])
        self.assertEqual(summary["totals"]["failed"], 1)


class CommandLine(unittest.TestCase):
    def test_a_registry_without_the_platform_table_fails_rather_than_misattributing(self):
        import contextlib
        import io

        with tempfile.TemporaryDirectory() as root:
            lane = os.path.join(root, "junit-unit-linux-gcc")
            os.makedirs(lane)
            with open(os.path.join(lane, "cpputest_SolidSyslogTests_Formatter.xml"), "w", encoding="utf-8") as handle:
                handle.write('<testsuite name="Formatter"><testcase classname="SolidSyslogTests.Formatter" name="WritesPri" time="0.001"/></testsuite>')
            registry = os.path.join(root, "CMakeLists.txt")
            with open(registry, "w", encoding="utf-8") as handle:
                handle.write("project(Nothing)\n")
            err = io.StringIO()
            with contextlib.redirect_stderr(err):
                status = pages.main([root, os.path.join(root, "out"), "--registry", registry, "--features", os.path.join(root, "none")])
        self.assertEqual(status, 1)
        self.assertIn("declares no SOLIDSYSLOG_PLATFORM_REGISTRY", err.getvalue())


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
