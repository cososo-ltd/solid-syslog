"""Regression tests for the Tests page hook (hooks/test_report.py).

Run:  python3 hooks/test_test_report.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')
"""

import json
import os
import sys
import tempfile
import types
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_report as h  # noqa: E402

SUMMARY = {
    "provenance": "Measured from main at 1234567.",
    "totals": {"total": 19543, "passed": 19442, "failed": 1, "skipped": 100, "time": 2673.4},
    "kinds": [
        {"kind": "unit", "name": "Unit", "href": "unit.html", "lanes": 5, "total": 16364, "passed": 16363, "failed": 1, "skipped": 0, "time": 805.2},
        {"kind": "bdd", "name": "BDD", "href": "bdd.html", "lanes": 8, "total": 688, "passed": 588, "failed": 0, "skipped": 100, "time": 1554.6},
    ],
    "lanes": [{"name": f"lane-{i}", "kind": "unit", "total": 1, "passed": 1, "failed": 0, "skipped": 0, "time": 0.0} for i in range(13)],
}

PAGE = "# Tests\n\nIntro.\n\n<!-- test-summary -->\n\nOutro.\n"


def page_of(src_uri):
    return types.SimpleNamespace(file=types.SimpleNamespace(src_uri=src_uri))


class WithAReport(unittest.TestCase):
    def setUp(self):
        self.docs = tempfile.TemporaryDirectory()
        report = os.path.join(self.docs.name, "testing", "test-report")
        os.makedirs(report)
        with open(os.path.join(report, "summary.json"), "w", encoding="utf-8") as handle:
            json.dump(SUMMARY, handle)
        self.out = h.on_page_markdown(PAGE, page_of("testing/tests.md"), {"docs_dir": self.docs.name}, None)

    def tearDown(self):
        self.docs.cleanup()

    def test_leads_with_the_provenance_and_one_sentence_of_totals(self):
        self.assertIn("Measured from main at 1234567.\n\n19543 tests across 13 lanes: 19442 passed, **1 failed**, 100 skipped, 44 min 33 s of test time in all.", self.out)

    def test_tabulates_each_kind(self):
        self.assertIn("| Unit | 5 | 16364 | 16363 | 1 | 0 | 13 min 25 s |", self.out)
        self.assertIn("| BDD | 8 | 688 | 588 | 0 | 100 | 25 min 55 s |", self.out)

    def test_links_every_doorway_into_the_report_with_the_kinds_between(self):
        for href in ("index.html", "unit.html", "bdd.html", "failures.html", "skipped.html", "slowest.html"):
            self.assertIn(f"](test-report/{href})", self.out)
        self.assertLess(self.out.index("test-report/unit.html"), self.out.index("test-report/bdd.html"))
        self.assertLess(self.out.index("test-report/bdd.html"), self.out.index("test-report/failures.html"))
        self.assertIn("[Unit](test-report/unit.html): its 5 lanes, and which lanes ran each suite.", self.out)

    def test_the_prose_around_the_marker_is_kept(self):
        self.assertTrue(self.out.startswith("# Tests\n\nIntro.\n\n"))
        self.assertTrue(self.out.endswith("\n\nOutro.\n"))


class WithoutAReport(unittest.TestCase):
    def test_says_so_and_links_to_nothing(self):
        with tempfile.TemporaryDirectory() as docs:
            out = h.on_page_markdown(PAGE, page_of("testing/tests.md"), {"docs_dir": docs}, None)
        self.assertIn("This build was made without a test report", out)
        self.assertNotIn("test-report/", out)


class OtherPages(unittest.TestCase):
    def test_are_returned_untouched(self):
        self.assertEqual(h.on_page_markdown(PAGE, page_of("testing/coverage.md"), {"docs_dir": "/nowhere"}, None), PAGE)


if __name__ == "__main__":
    unittest.main()
