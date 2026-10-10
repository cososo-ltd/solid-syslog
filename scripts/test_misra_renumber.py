"""Tests for misra_renumber.py: its line arithmetic, and the plan and rewrite
it makes against a fake repository.

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
import misra_renumber  # noqa: E402

BASE = "base-ref"
SOURCE = "Core/Source/A.c"
BASE_SOURCE = "first\nsecond\nsuppressed\nlast\n"


# Stands in for the repository: the base's files and the diffs git would
# report, with the working tree in a temporary directory.
class FakeRepositoryTest(unittest.TestCase):
    def setUp(self):
        self.base_files = {misra_renumber.SUPPRESSIONS: "", SOURCE: BASE_SOURCE}
        self.diffs = {}
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        original_git, original_root = misra_renumber.git, misra_renumber.REPO_ROOT
        misra_renumber.git = self.fake_git
        misra_renumber.REPO_ROOT = self.root
        self.addCleanup(setattr, misra_renumber, "git", original_git)
        self.addCleanup(setattr, misra_renumber, "REPO_ROOT", original_root)

    def fake_git(self, *args):
        if args[0] == "show":
            ref, path = args[1].split(":", 1)
            self.assertEqual(BASE, ref)
            return self.base_files[path]
        if args[0] == "diff":
            return self.diffs.get(args[-1], "")
        raise AssertionError(f"unexpected git call {args}")

    def given(self, base_suppressions, current_suppressions, working_source, diff):
        self.base_files[misra_renumber.SUPPRESSIONS] = base_suppressions
        (self.root / misra_renumber.SUPPRESSIONS).write_text(current_suppressions, encoding="utf-8")
        (self.root / "Core" / "Source").mkdir(parents=True)
        (self.root / SOURCE).write_text(working_source, encoding="utf-8")
        self.diffs[SOURCE] = diff

    def run_main(self, *argv):
        with contextlib.redirect_stdout(io.StringIO()):
            return misra_renumber.main(["--base", BASE, *argv])

    def suppressions(self):
        return (self.root / misra_renumber.SUPPRESSIONS).read_text(encoding="utf-8")


class PlanTest(FakeRepositoryTest):
    def test_an_entry_moves_by_the_lines_added_above_it(self):
        entry = f"misra-c2012-11.3:{SOURCE}:3\n"
        self.given(entry, entry, "new\nfirst\nsecond\nsuppressed\nlast\n", "@@ -0,0 +1 @@\n")

        updates, attention = misra_renumber.plan(BASE, entry)

        self.assertEqual({("misra-c2012-11.3", SOURCE, 3): 4}, updates)
        self.assertEqual([], attention)

    def test_an_entry_added_on_the_branch_is_left_alone(self):
        entry = f"misra-c2012-11.3:{SOURCE}:3\n"
        self.given("", entry, "new\nfirst\nsecond\nsuppressed\nlast\n", "@@ -0,0 +1 @@\n")

        self.assertEqual(({}, []), misra_renumber.plan(BASE, entry))

    def test_an_entry_whose_line_was_edited_needs_attention(self):
        entry = f"misra-c2012-11.3:{SOURCE}:3\n"
        self.given(entry, entry, "first\nsecond\nedited\nlast\n", "@@ -3 +3 @@\n")

        updates, attention = misra_renumber.plan(BASE, entry)

        self.assertEqual({}, updates)
        self.assertEqual([("misra-c2012-11.3", SOURCE, 3)], [item[0] for item in attention])

    def test_an_entry_whose_new_line_holds_other_text_needs_attention(self):
        entry = f"misra-c2012-11.3:{SOURCE}:3\n"
        self.given(entry, entry, "new\nfirst\nsecond\nnot it\nlast\n", "@@ -0,0 +1 @@\n")

        updates, attention = misra_renumber.plan(BASE, entry)

        self.assertEqual({}, updates)
        self.assertEqual([("misra-c2012-11.3", SOURCE, 3)], [item[0] for item in attention])


class MainWorkflowTest(FakeRepositoryTest):
    MOVED = f"misra-c2012-11.3:{SOURCE}:3\n"
    UNTOUCHED = "misra-c2012-11.5:Core/Source/B.c:7\n"

    def given_one_entry_to_move(self):
        self.given(self.MOVED + self.UNTOUCHED, self.MOVED + self.UNTOUCHED,
                   "new\nfirst\nsecond\nsuppressed\nlast\n", "@@ -0,0 +1 @@\n")

    def test_nothing_to_move_exits_0(self):
        self.given(self.MOVED, self.MOVED, BASE_SOURCE, "")

        self.assertEqual(0, self.run_main())

    def test_a_proposed_update_exits_1_and_writes_nothing(self):
        self.given_one_entry_to_move()

        self.assertEqual(1, self.run_main())
        self.assertEqual(self.MOVED + self.UNTOUCHED, self.suppressions())

    def test_apply_rewrites_only_the_moved_entry_and_exits_0(self):
        self.given_one_entry_to_move()

        self.assertEqual(0, self.run_main("--apply"))
        self.assertEqual(f"misra-c2012-11.3:{SOURCE}:4\n" + self.UNTOUCHED, self.suppressions())

    def test_an_entry_needing_attention_exits_2(self):
        self.given(self.MOVED, self.MOVED, "first\nsecond\nedited\nlast\n", "@@ -3 +3 @@\n")

        self.assertEqual(2, self.run_main())


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


class LineAtTest(unittest.TestCase):
    def test_a_form_feed_does_not_start_a_new_line(self):
        self.assertEqual("third", misra_renumber.line_at("first\fstill first\nsecond\nthird\n", 3))


class MainTest(unittest.TestCase):
    def test_a_base_git_cannot_resolve_exits_3(self):
        self.assertEqual(3, misra_renumber.main(["--base", "no-such-ref-for-this-test"]))


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
