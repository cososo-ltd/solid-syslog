"""Tests for check_test_groups.py, against a fake repository.

Run:  python3 -m unittest discover -s scripts -p 'test_*.py'
"""

import contextlib
import io
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_test_groups  # noqa: E402


def a_test(group, name):
    return f"TEST({group}, {name})\n{{\n}}\n\n"


class TestGroupsTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def given_file(self, path, text=""):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text, encoding="utf-8")

    def run_main(self):
        self.output = io.StringIO()
        with contextlib.redirect_stdout(self.output):
            return check_test_groups.main(["--root", str(self.root)])

    def test_passes_when_there_are_no_tests(self):
        self.given_file("Tests/EmptyTest.cpp")
        self.assertEqual(0, self.run_main())

    def test_passes_when_each_group_follows_the_last(self):
        self.given_file(
            "Tests/StreamTest.cpp",
            a_test("Stream", "Opens") + a_test("Stream", "Closes") + a_test("StreamPool", "Exhausts"),
        )
        self.assertEqual(0, self.run_main())

    def test_fails_naming_the_test_that_resumes_a_group(self):
        self.given_file(
            "Tests/StreamTest.cpp",
            a_test("Stream", "Opens") + a_test("StreamPool", "Exhausts") + a_test("Stream", "Closes"),
        )
        self.assertEqual(1, self.run_main())
        self.assertIn("Tests/StreamTest.cpp:9: TEST(Stream, Closes)", self.output.getvalue())

    def test_reports_a_resumption_once_however_many_tests_it_holds(self):
        self.given_file(
            "Tests/StreamTest.cpp",
            a_test("Stream", "Opens") + a_test("StreamPool", "Exhausts") + a_test("Stream", "Closes") + a_test("Stream", "Reads"),
        )
        self.assertEqual(1, self.run_main())
        self.assertNotIn("Reads", self.output.getvalue())

    def test_reports_every_resumption_of_a_group(self):
        self.given_file(
            "Tests/StreamTest.cpp",
            a_test("Stream", "Opens")
            + a_test("StreamPool", "Exhausts")
            + a_test("Stream", "Closes")
            + a_test("StreamPool", "Recovers")
            + a_test("Stream", "Reads"),
        )
        self.assertEqual(1, self.run_main())
        for name in ("TEST(Stream, Closes)", "TEST(StreamPool, Recovers)", "TEST(Stream, Reads)"):
            self.assertIn(name, self.output.getvalue())

    def test_a_group_definition_between_a_groups_tests_is_not_a_resumption(self):
        self.given_file(
            "Tests/StreamTest.cpp",
            a_test("Stream", "Opens") + "TEST_GROUP(StreamPool)\n{\n};\n\n" + a_test("Stream", "Closes"),
        )
        self.assertEqual(0, self.run_main())

    def test_reads_a_test_whose_names_are_on_the_lines_below_the_macro(self):
        self.given_file(
            "Tests/StreamTest.cpp",
            a_test("Stream", "Opens") + a_test("StreamPool", "Exhausts") + "TEST(\n    Stream,\n    Closes\n)\n{\n}\n",
        )
        self.assertEqual(1, self.run_main())
        self.assertIn("Tests/StreamTest.cpp:9: TEST(Stream, Closes)", self.output.getvalue())

    def test_counts_an_ignored_test_as_one_of_its_group(self):
        self.given_file(
            "Tests/StreamTest.cpp",
            a_test("Stream", "Opens") + a_test("StreamPool", "Exhausts") + "IGNORE_" + a_test("Stream", "Closes"),
        )
        self.assertEqual(1, self.run_main())

    def test_the_same_group_in_two_files_is_not_a_resumption(self):
        self.given_file("Tests/StreamTest.cpp", a_test("Stream", "Opens"))
        self.given_file("Tests/Pack/StreamTest.cpp", a_test("Stream", "Opens"))
        self.assertEqual(0, self.run_main())

    def test_files_outside_the_test_tree_are_not_read(self):
        self.given_file(
            "Core/Source/Stream.cpp",
            a_test("Stream", "Opens") + a_test("StreamPool", "Exhausts") + a_test("Stream", "Closes"),
        )
        self.given_file("Tests/EmptyTest.cpp")
        self.assertEqual(0, self.run_main())


if __name__ == "__main__":
    unittest.main()
