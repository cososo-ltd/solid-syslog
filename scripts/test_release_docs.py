"""Tests for release_docs.py: bringing the documentation to a release from the example stacks' sweeps."""

import contextlib
import io
import os
import tempfile
import unittest

import release_docs

LINK = "https://github.com/cososo-ltd/solid-syslog-example/tree/release/"
MAKE_LINK = "https://github.com/cososo-ltd/solid-syslog-example-make/tree/release/"

README = (
    "# SolidSyslog\n\n## What it costs\n\n"
    "**+7.2 KB flash** for a record on the wire, and **2.8 KB\nof RAM**. **+23.9 KB flash** for the whole\n"
    "path and **38.1 KB of RAM**.\n\n"
    "Published as [the example](" + LINK + "0.4.0) and\n[with Make](" + MAKE_LINK + "0.4.0).\n\n"
    "## Capabilities\n\nA 1.5 KB note that is not a cost.\n"
)
HARDENING_PATH = (
    "- [the example](" + LINK + "0.4.0) - consumed with CMake.\n\n"
    "Measured cumulatively, the path costs about 23.9 KB of flash and\n38.1 KB of RAM, most of it TLS.\n\n"
    "**Cost.** Flash ~400 B, no RAM.\n"
)
BUILD_INTEGRATION = "FetchContent_Declare(SolidSyslog\n    GIT_TAG        v0.4.0)\n"

# slug, flash above the Baseline, RAM above the Baseline
STAGES = (("Baseline", 0, 0), ("linked", 0, 0), ("udp", 11164, 3056), ("tcp", 12000, 3200), ("right-size", 28060, 38356))


class ReleaseDocsTest(unittest.TestCase):
    def setUp(self):
        self.root = tempfile.TemporaryDirectory()
        self.addCleanup(self.root.cleanup)
        self.write("README.md", README)
        self.write("docs/hardening-path.md", HARDENING_PATH)
        self.write("docs/build-integration.md", BUILD_INTEGRATION)
        self.cmake = self.sweep("cmake", STAGES)
        self.make = self.sweep("make", STAGES)

    def write(self, path, text):
        full = os.path.join(self.root.name, path)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        with open(full, "w", encoding="utf-8", newline="") as f:
            f.write(text)

    def read(self, path):
        with open(os.path.join(self.root.name, path), encoding="utf-8", newline="") as f:
            return f.read()

    def sweep(self, name, stages, verdict="PASS"):
        """A sweep's output: all of a stage's flash in .text, all of its RAM in .bss."""
        stage_rows = ["index,slug,commit,verdict"]
        figure_rows = ["index,slug,commit,key,current,baseline,above_baseline"]
        for index, (slug, flash, ram) in enumerate(stages):
            stage_rows.append(f'{index},{slug},c{index},"{verdict}"')
            for key, above in (("flash_text", flash), ("flash_data", 0), ("static_bss", ram), ("stack_log", 7)):
                figure_rows.append(f"{index},{slug},c{index},{key},{1000 + above},1000,{above}")
        self.write(f"sweeps/{name}/stages.csv", "\n".join(stage_rows) + "\n")
        self.write(f"sweeps/{name}/figures.csv", "\n".join(figure_rows) + "\n")
        return os.path.join(self.root.name, "sweeps", name)

    def run_main(self, *arguments):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = release_docs.main(["0.5.0", self.cmake, self.make, "--root", self.root.name, *arguments])
        return status, out.getvalue(), err.getvalue()

    def test_readme_takes_the_first_record_and_the_whole_path_from_the_sweep(self):
        self.assertEqual(0, self.run_main()[0])
        readme = self.read("README.md")
        self.assertIn("**+11.2 KB flash** for a record on the wire, and **3.1 KB\nof RAM**.", readme)
        self.assertIn("**+28.1 KB flash** for the whole\npath and **38.4 KB of RAM**.", readme)

    def test_a_figure_outside_the_costs_paragraph_is_left_alone(self):
        self.run_main()
        self.assertIn("A 1.5 KB note that is not a cost.", self.read("README.md"))

    def test_flash_counts_the_initialisers_and_ram_counts_them_too(self):
        self.write(
            "sweeps/cmake/figures.csv",
            self.read("sweeps/cmake/figures.csv").replace("2,udp,c2,flash_data,1000,1000,0", "2,udp,c2,flash_data,1100,1000,100"),
        )
        self.assertEqual(("11.3", "3.2"), release_docs.headline(release_docs.load(self.cmake), self.cmake)[:2])

    def test_the_hardening_path_states_the_whole_path(self):
        self.run_main()
        self.assertIn("costs about 28.1 KB of flash and\n38.4 KB of RAM, most of it TLS.", self.read("docs/hardening-path.md"))

    def test_a_stage_cost_in_prose_is_not_rewritten(self):
        self.run_main()
        self.assertIn("**Cost.** Flash ~400 B, no RAM.", self.read("docs/hardening-path.md"))

    def test_every_link_to_a_release_branch_names_the_version(self):
        self.write("docs/security/notes.md", "See [with Make](" + MAKE_LINK + "0.3.0).\n")
        self.run_main()
        for page in ("README.md", "docs/hardening-path.md", "docs/security/notes.md"):
            self.assertNotIn("release/0.4.0", self.read(page))
            self.assertNotIn("release/0.3.0", self.read(page))
        self.assertIn("[the example](" + LINK + "0.5.0) and\n[with Make](" + MAKE_LINK + "0.5.0).", self.read("README.md"))
        self.assertEqual("See [with Make](" + MAKE_LINK + "0.5.0).\n", self.read("docs/security/notes.md"))

    def test_a_link_to_the_branch_placeholder_is_left_alone(self):
        self.write("docs/release-process.md", "Cut `release/<version>` on " + LINK[: -len("release/")] + "main.\n")
        self.run_main()
        self.assertIn("`release/<version>`", self.read("docs/release-process.md"))

    def test_git_tag_names_the_version(self):
        self.run_main()
        self.assertEqual("FetchContent_Declare(SolidSyslog\n    GIT_TAG        v0.5.0)\n", self.read("docs/build-integration.md"))

    def test_reports_what_it_rewrote(self):
        _, out, _ = self.run_main()
        self.assertIn("first record 11.2 KB flash, 3.1 KB RAM; whole path 28.1 KB flash, 38.4 KB RAM", out)
        self.assertIn("rewritten: README.md", out)
        self.assertIn("rewritten: docs/build-integration.md", out)

    def test_a_second_run_changes_nothing(self):
        self.run_main()
        status, out, _ = self.run_main()
        self.assertEqual(0, status)
        self.assertNotIn("rewritten", out)

    def test_check_fails_and_writes_nothing_where_the_pages_are_behind(self):
        status, out, _ = self.run_main("--check")
        self.assertEqual(1, status)
        self.assertIn("differs: README.md", out)
        self.assertEqual(README, self.read("README.md"))

    def test_check_passes_once_the_pages_are_brought_to_the_release(self):
        self.run_main()
        self.assertEqual(0, self.run_main("--check")[0])

    def test_the_figures_are_the_cmake_stacks_where_the_make_stack_rounds_differently(self):
        self.make = self.sweep("make", STAGES[:-1] + (("right-size", 28044, 38352),))
        status, out, _ = self.run_main()
        self.assertEqual(0, status)
        self.assertIn("**+28.1 KB flash** for the whole", self.read("README.md"))
        self.assertIn("the Make stack rounds to 11.2 / 3.1 / 28.0 / 38.4 KB", out)

    def test_says_nothing_of_the_make_stack_where_it_rounds_the_same(self):
        self.assertNotIn("Make stack", self.run_main()[1])

    def test_a_make_sweep_with_a_stage_that_failed_is_refused(self):
        self.make = self.sweep("make", STAGES, verdict="FAIL (exit 1)")
        self.assertEqual(1, self.run_main()[0])
        self.assertEqual(README, self.read("README.md"))

    def test_a_sweep_with_a_stage_that_failed_is_refused(self):
        self.cmake = self.sweep("cmake", STAGES, verdict="FAIL (exit 1)")
        status, _, err = self.run_main()
        self.assertEqual(1, status)
        self.assertIn("did not pass", err)

    def test_a_sweep_whose_baseline_is_not_its_own_is_refused(self):
        self.cmake = self.sweep("cmake", tuple((slug, flash + 4108, ram + 232) for slug, flash, ram in STAGES))
        status, _, err = self.run_main()
        self.assertEqual(1, status)
        self.assertIn("the Baseline measures +4,108 B flash and +232 B RAM above its own", err)
        self.assertEqual(README, self.read("README.md"))

    def test_a_sweep_without_the_first_record_stage_is_refused(self):
        self.cmake = self.sweep("cmake", tuple(stage for stage in STAGES if stage[0] != "udp"))
        status, _, err = self.run_main()
        self.assertEqual(1, status)
        self.assertIn("no stage 'udp'", err)

    def test_a_readme_stating_another_number_of_figures_is_refused(self):
        self.write("README.md", README.replace("**38.1 KB of RAM**", "**little RAM**"))
        status, _, err = self.run_main()
        self.assertEqual(1, status)
        self.assertIn("README.md states 3 figures where 4 are measured", err)

    def test_a_stack_nothing_links_is_refused(self):
        self.write("README.md", README.replace(MAKE_LINK + "0.4.0", "https://example.invalid/"))
        status, _, err = self.run_main()
        self.assertEqual(1, status)
        self.assertIn("nothing links a release branch of solid-syslog-example-make", err)

    def test_a_version_that_is_not_one_is_refused(self):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = release_docs.main(["v0.5.0", self.cmake, self.make, "--root", self.root.name])
        self.assertEqual(1, status)
        self.assertIn("'v0.5.0' is not a version", err.getvalue())

    def test_names_the_stages_whose_cost_moved_since_the_last_release(self):
        previous = self.sweep(
            "previous", (("Baseline", 0, 0), ("linked", 0, 0), ("udp", 7156, 2776), ("tcp", 7992, 2920), ("gone", 8000, 2920))
        )
        _, out, _ = self.run_main("--previous", previous)
        self.assertIn("stages of docs/hardening-path.md to re-take:", out)
        self.assertIn("  Stage 2 udp: flash +7,156 -> +11,164 B, RAM +2,776 -> +3,056 B", out)
        self.assertIn("  Stage 4 right-size: new, flash +16,060 B, RAM +35,156 B", out)
        self.assertIn("  gone: no longer a stage", out)
        self.assertNotIn("tcp", out)
        self.assertNotIn("linked", out)
        self.assertNotIn("Baseline", out)

    def test_names_no_stage_where_none_moved(self):
        _, out, _ = self.run_main("--previous", self.cmake)
        self.assertNotIn("re-take", out)


if __name__ == "__main__":
    unittest.main()
