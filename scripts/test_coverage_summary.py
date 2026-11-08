"""Tests for coverage_summary.py: the per-pack table and the gate."""

import contextlib
import io
import os
import tempfile
import textwrap
import unittest

import coverage_summary

TRACEFILE = textwrap.dedent(
    """\
    TN:
    SF:/work/Core/Source/SolidSyslog.c
    LF:10
    LH:9
    BRF:4
    BRH:3
    end_of_record
    SF:/work/Core/Source/SolidSyslogError.c
    LF:10
    LH:10
    BRF:2
    BRH:2
    end_of_record
    SF:/work/Platform/Posix/Source/SolidSyslogPosixClock.c
    LF:5
    LH:5
    BRF:2
    BRH:1
    end_of_record
    """
)


class Aggregation(unittest.TestCase):
    def test_groups_core_and_each_pack(self):
        packs = coverage_summary.aggregate(io.StringIO(TRACEFILE))
        self.assertEqual(sorted(packs), ["Core", "Posix"])

    def test_sums_lines_and_branches_across_a_pack(self):
        packs = coverage_summary.aggregate(io.StringIO(TRACEFILE))
        self.assertEqual(packs["Core"], coverage_summary.Counts(20, 19, 6, 5))

    def test_a_windows_path_is_grouped_by_its_pack(self):
        self.assertEqual(coverage_summary.pack_of(r"C:\repos\solid-syslog\Platform\Windows\Source\Clock.c"), "Windows")

    def test_total_is_the_sum_of_every_pack(self):
        packs = coverage_summary.aggregate(io.StringIO(TRACEFILE))
        self.assertEqual(coverage_summary.total(packs), coverage_summary.Counts(25, 24, 8, 6))


class Gate(unittest.TestCase):
    def test_passes_when_both_rates_meet_the_threshold(self):
        self.assertEqual(coverage_summary.failures(coverage_summary.Counts(100, 95, 100, 90), 90), [])

    def test_names_the_branch_rate_that_falls_short(self):
        found = coverage_summary.failures(coverage_summary.Counts(100, 95, 100, 89), 90)
        self.assertEqual(len(found), 1)
        self.assertIn("branch", found[0])

    def test_names_the_line_rate_that_falls_short(self):
        found = coverage_summary.failures(coverage_summary.Counts(100, 89, 100, 95), 90)
        self.assertEqual(len(found), 1)
        self.assertIn("line", found[0])

    def test_a_rate_just_short_is_not_shown_as_the_threshold(self):
        found = coverage_summary.failures(coverage_summary.Counts(10000, 8996, 100, 100), 90)
        self.assertEqual(found, ["line coverage 89.9% is below 90%"])

    def test_no_branch_data_is_a_failure_not_a_skip(self):
        found = coverage_summary.failures(coverage_summary.Counts(100, 100, 0, 0), 90)
        self.assertEqual(len(found), 1)
        self.assertIn("no branch data", found[0])

    def test_no_line_data_is_a_failure(self):
        found = coverage_summary.failures(coverage_summary.Counts(0, 0, 0, 0), 90)
        self.assertTrue(any("no line data" in f for f in found))


class Table(unittest.TestCase):
    def test_markdown_renders_each_name_cell_through_the_label(self):
        packs = coverage_summary.aggregate(io.StringIO(TRACEFILE))
        rows = coverage_summary.markdown(packs, label=lambda name: f"<{name}>").splitlines()
        self.assertTrue(rows[2].startswith("| <Core> |"))
        self.assertTrue(rows[-1].startswith("| <**Total**> |"))

    def test_markdown_has_a_row_per_pack_and_a_total(self):
        packs = coverage_summary.aggregate(io.StringIO(TRACEFILE))
        rows = coverage_summary.markdown(packs).splitlines()
        self.assertIn("| Core | 19 / 20 | 95.0% | 5 / 6 | 83.3% |", rows)
        self.assertEqual(rows[-1], "| **Total** | 24 / 25 | 96.0% | 6 / 8 | 75.0% |")


class Main(unittest.TestCase):
    def run_main(self, tracefile, *options):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "coverage.info")
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(tracefile)
            out, err = io.StringIO(), io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                status = coverage_summary.main([path, *options])
        return status, out.getvalue(), err.getvalue()

    def test_prints_the_table_and_exits_zero_when_the_gate_passes(self):
        status, out, err = self.run_main(TRACEFILE, "--threshold", "75")
        self.assertEqual(status, 0)
        self.assertIn("| **Total** |", out)
        self.assertEqual(err, "")

    def test_reports_each_failure_on_stderr_and_exits_one(self):
        without_branches = "".join(line for line in TRACEFILE.splitlines(keepends=True) if not line.startswith("BR"))
        status, _, err = self.run_main(without_branches)
        self.assertEqual(status, 1)
        self.assertIn("FAIL: no branch data in the tracefile", err)

    def test_a_failing_gate_ends_the_table_before_the_verdict(self):
        status, out, _ = self.run_main(TRACEFILE)
        self.assertEqual(status, 1)
        self.assertTrue(out.endswith("|\n\n"))

    def test_default_threshold_is_ninety(self):
        status, _, err = self.run_main(TRACEFILE)
        self.assertEqual(status, 1)
        self.assertIn("branch coverage 75.0% is below 90%", err)


if __name__ == "__main__":
    unittest.main()
