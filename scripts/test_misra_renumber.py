"""Tests for misra_renumber.py's line arithmetic.

Run:  python3 -m unittest discover -s scripts -p 'test_*.py'
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import misra_renumber  # noqa: E402


class ParseHunksTest(unittest.TestCase):
    def test_a_hunk_with_counts_gives_its_start_and_both_counts(self):
        self.assertEqual([(31, 1, 9)], misra_renumber.parse_hunks("@@ -31 +37,9 @@ enum"))

    def test_a_missing_count_is_one(self):
        self.assertEqual([(191, 1, 1)], misra_renumber.parse_hunks("@@ -191 +197 @@"))

    def test_an_insertion_has_an_old_count_of_zero(self):
        self.assertEqual([(9, 0, 1)], misra_renumber.parse_hunks("@@ -9,0 +10 @@"))

    def test_lines_that_are_not_hunk_headers_are_ignored(self):
        diff = "diff --git a/x b/x\n--- a/x\n+++ b/x\n@@ -2,0 +3,2 @@\n+one\n+two\n"
        self.assertEqual([(2, 0, 2)], misra_renumber.parse_hunks(diff))


class MapLineTest(unittest.TestCase):
    def test_a_line_above_every_hunk_stays_put(self):
        self.assertEqual(5, misra_renumber.map_line([(10, 0, 3)], 5))

    def test_lines_inserted_above_move_it_down(self):
        self.assertEqual(13, misra_renumber.map_line([(9, 0, 3)], 10))

    def test_lines_inserted_after_it_leave_it(self):
        self.assertEqual(10, misra_renumber.map_line([(10, 0, 3)], 10))

    def test_lines_removed_above_move_it_up(self):
        self.assertEqual(7, misra_renumber.map_line([(2, 3, 0)], 10))

    def test_a_replacement_above_moves_it_by_the_difference(self):
        self.assertEqual(18, misra_renumber.map_line([(5, 1, 9)], 10))

    def test_shifts_from_several_hunks_add_up(self):
        self.assertEqual(36, misra_renumber.map_line([(9, 0, 1), (15, 0, 2), (17, 0, 1), (18, 0, 1), (21, 0, 1)], 30))

    def test_an_edited_line_cannot_be_moved(self):
        self.assertIsNone(misra_renumber.map_line([(31, 1, 9)], 31))

    def test_a_removed_line_cannot_be_moved(self):
        self.assertIsNone(misra_renumber.map_line([(30, 3, 0)], 31))


class EntriesTest(unittest.TestCase):
    def test_an_entry_gives_its_rule_path_and_line(self):
        self.assertEqual([("misra-c2012-11.3", "Core/Source/A.c", 66)],
                         misra_renumber.entries("misra-c2012-11.3:Core/Source/A.c:66\n"))

    def test_comments_blank_lines_and_entries_without_a_line_are_ignored(self):
        text = "# D.007 - Rule 21.10\n\nmisra-c2012-21.10:Core/Source/A.c\n"
        self.assertEqual([], misra_renumber.entries(text))


class RewriteTest(unittest.TestCase):
    def test_only_the_updated_entries_change(self):
        text = "# comment\nmisra-c2012-11.3:A.c:66\nmisra-c2012-11.5:A.c:115\n"
        updated = misra_renumber.rewrite(text, {("misra-c2012-11.3", "A.c", 66): 98})
        self.assertEqual("# comment\nmisra-c2012-11.3:A.c:98\nmisra-c2012-11.5:A.c:115\n", updated)

    def test_line_endings_are_kept(self):
        updated = misra_renumber.rewrite("misra-c2012-11.3:A.c:66\r\n", {("misra-c2012-11.3", "A.c", 66): 98})
        self.assertEqual("misra-c2012-11.3:A.c:98\r\n", updated)


if __name__ == "__main__":
    unittest.main()
