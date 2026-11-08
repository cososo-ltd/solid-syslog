"""Regression tests for the coverage page hook (hooks/coverage_report.py).

Run:  python3 hooks/test_coverage_report.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')

The strict build proves the links resolve when the report is present. What it
cannot prove is the shape of the table, or that a build with no report in hand
- every local one - still says something true and links to nothing.
"""

import os
import sys
import tempfile
import textwrap
import types
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coverage_report as h  # noqa: E402

TRACEFILE = textwrap.dedent(
    """\
    TN:
    SF:/work/Core/Source/SolidSyslog.c
    LF:10
    LH:9
    BRF:4
    BRH:3
    end_of_record
    SF:/work/Platform/Posix/Source/SolidSyslogPosixClock.c
    LF:5
    LH:5
    BRF:2
    BRH:1
    end_of_record
    """
)

PAGE = "# Coverage\n\nIntro.\n\n<!-- coverage-table -->\n\nOutro.\n"


def page_of(src_uri):
    return types.SimpleNamespace(file=types.SimpleNamespace(src_uri=src_uri))


class WithAReport(unittest.TestCase):
    def setUp(self):
        self.docs = tempfile.TemporaryDirectory()
        report = os.path.join(self.docs.name, "testing", "coverage-report")
        os.makedirs(report)
        with open(os.path.join(report, "coverage.filtered.info"), "w", encoding="utf-8") as handle:
            handle.write(TRACEFILE)
        self.out = h.on_page_markdown(PAGE, page_of("testing/coverage.md"), {"docs_dir": self.docs.name}, None)

    def tearDown(self):
        self.docs.cleanup()

    def test_each_pack_row_links_to_its_report_index(self):
        self.assertIn("| [Core](coverage-report/Core/Source/index.html) | 9 / 10 | 90.0% | 3 / 4 | 75.0% |", self.out)
        self.assertIn("| [Posix](coverage-report/Platform/Posix/Source/index.html) | 5 / 5 | 100.0% |", self.out)

    def test_the_total_row_links_to_the_report_root(self):
        self.assertIn("| [**Total**](coverage-report/index.html) | 14 / 15 |", self.out)

    def test_the_prose_around_the_marker_is_kept(self):
        self.assertTrue(self.out.startswith("# Coverage\n\nIntro.\n\n"))
        self.assertTrue(self.out.endswith("\n\nOutro.\n"))
        self.assertNotIn("<!-- coverage-table -->", self.out)


class WithoutAReport(unittest.TestCase):
    def setUp(self):
        self.docs = tempfile.TemporaryDirectory()
        self.out = h.on_page_markdown(PAGE, page_of("testing/coverage.md"), {"docs_dir": self.docs.name}, None)

    def tearDown(self):
        self.docs.cleanup()

    def test_says_the_build_has_no_report(self):
        self.assertIn("This build was made without a coverage report", self.out)

    def test_links_to_nothing_the_strict_build_could_miss(self):
        self.assertNotIn("coverage-report/", self.out)
        self.assertNotIn("<!-- coverage-table -->", self.out)


class OtherPages(unittest.TestCase):
    def test_are_returned_untouched(self):
        self.assertEqual(h.on_page_markdown(PAGE, page_of("ci.md"), {"docs_dir": "/nowhere"}, None), PAGE)


if __name__ == "__main__":
    unittest.main()
