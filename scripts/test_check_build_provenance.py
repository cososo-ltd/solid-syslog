"""Tests for check_build_provenance.py, against a fake build tree.

Run:  python3 -m unittest discover -s scripts -p 'test_*.py'
"""

import contextlib
import io
import os
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_build_provenance  # noqa: E402


class BuildProvenanceTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name).resolve()
        self.checkout = self.root / "checkout"
        self.build = self.checkout / "build" / "cross"
        self.kernel = self.root / "opt" / "kernel"
        self.wrapper = self.root / "opt" / "cmsis-freertos"
        self.toolchain = self.root / "toolchain" / "include"
        self.given_file(self.kernel / "include" / "FreeRTOS.h")
        self.given_file(self.kernel / "include" / "task.h", '#define tskKERNEL_VERSION_NUMBER "V11.3.3"\n')
        self.given_file(self.wrapper / "Source" / "include" / "task.h", '#define tskKERNEL_VERSION_NUMBER "V11.3.0"\n')
        self.given_file(
            self.build / "CMakeCache.txt",
            f"CMAKE_HOME_DIRECTORY:INTERNAL={self.checkout.as_posix()}\n"
            f"SOLIDSYSLOG_FREERTOS_KERNEL_PATH:PATH={self.kernel.as_posix()}\n"
            f"SOLIDSYSLOG_CMSIS_FREERTOS_PATH:PATH={self.wrapper.as_posix()}\n"
            "SOLIDSYSLOG_LWIP_PATH:PATH=\n",
        )
        self.given_file(
            self.build / "CMakeFiles" / "3.25.1" / "CMakeCCompiler.cmake",
            f'set(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES "{self.toolchain.as_posix()}")\n',
        )

    def given_file(self, path, text=""):
        path = Path(path)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")

    def given_object_that_read(self, *files, name="main.c.obj.d"):
        prerequisites = " \\\n ".join(Path(file).as_posix() for file in files)
        self.given_file(
            self.build / "CMakeFiles" / "target.dir" / name,
            f"CMakeFiles/target.dir/{name[:-2]}: {prerequisites}\n",
        )

    def run_main(self, *extra):
        self.output = io.StringIO()
        with contextlib.redirect_stdout(self.output):
            return check_build_provenance.main(["--build", str(self.build), *extra])

    def test_passes_when_every_file_is_inside_a_wired_subtree(self):
        self.given_object_that_read(
            self.checkout / "Bdd" / "Targets" / "T" / "main.c",
            self.kernel / "include" / "task.h",
            self.kernel / "tasks.c",
            self.kernel / "portable" / "GCC" / "ARM_CM3" / "portmacro.h",
            self.toolchain / "stdint.h",
            self.build / "generated.h",
        )
        self.assertEqual(0, self.run_main())

    def test_summary_counts_files_by_tree(self):
        self.given_object_that_read(
            self.checkout / "main.c",
            self.kernel / "include" / "task.h",
            self.kernel / "tasks.c",
            self.toolchain / "stdint.h",
        )
        self.run_main()
        self.assertIn("| FREERTOS_KERNEL_PATH | 2 |", self.output.getvalue())
        self.assertIn("| this checkout | 1 |", self.output.getvalue())
        self.assertIn("| compiler | 1 |", self.output.getvalue())

    def test_fails_naming_a_file_outside_every_pinned_tree(self):
        stray = self.root / "elsewhere" / "stray.h"
        self.given_object_that_read(self.checkout / "main.c", stray)
        self.assertEqual(1, self.run_main())
        self.assertIn(f"::error::{stray}: outside every pinned tree", self.output.getvalue())

    def test_fails_naming_a_file_inside_a_repository_but_outside_its_wired_subtrees(self):
        bundled = self.wrapper / "Source" / "list.c"
        self.given_object_that_read(self.checkout / "main.c", bundled)
        self.assertEqual(1, self.run_main())
        self.assertIn(f"::error::{bundled}: inside the repository but outside its wired subtrees", self.output.getvalue())

    def test_fails_when_a_kernel_header_name_resolves_outside_the_pinned_kernel(self):
        shadow = self.wrapper / "CMSIS" / "RTOS2" / "FreeRTOS" / "Include" / "FreeRTOS.h"
        self.given_object_that_read(self.checkout / "main.c", shadow)
        self.assertEqual(1, self.run_main())
        self.assertIn(f"::error::{shadow}: FreeRTOS.h is owned by {self.kernel}", self.output.getvalue())

    def test_the_port_header_is_a_kernel_name_too(self):
        shadow = self.wrapper / "CMSIS" / "RTOS2" / "FreeRTOS" / "Include" / "portmacro.h"
        self.given_object_that_read(self.checkout / "main.c", shadow)
        self.assertEqual(1, self.run_main())
        self.assertIn("portmacro.h is owned by", self.output.getvalue())

    def test_fails_when_a_target_header_name_resolves_outside_the_checkout(self):
        ours = self.checkout / "Bdd" / "Targets" / "T" / "FreeRTOSConfig.h"
        theirs = self.kernel / "portable" / "MemMang" / "FreeRTOSConfig.h"
        self.given_object_that_read(self.checkout / "main.c", ours, theirs)
        self.assertEqual(1, self.run_main())
        self.assertIn(f"::error::{theirs}: FreeRTOSConfig.h is owned by the target, at {ours}", self.output.getvalue())

    def test_a_target_name_may_recur_inside_the_checkout(self):
        self.given_object_that_read(
            self.checkout / "Bdd" / "Targets" / "T" / "lwipopts.h",
            self.checkout / "Bdd" / "Targets" / "U" / "lwipopts.h",
        )
        self.assertEqual(0, self.run_main())

    def test_counts_a_file_once_however_many_objects_read_it(self):
        self.given_object_that_read(self.checkout / "a.c", self.kernel / "include" / "task.h", name="a.c.obj.d")
        self.given_object_that_read(self.checkout / "b.c", self.kernel / "include" / "task.h", name="b.c.obj.d")
        self.run_main()
        self.assertIn("| FREERTOS_KERNEL_PATH | 1 |", self.output.getvalue())

    def test_a_relative_prerequisite_is_relative_to_the_build_directory(self):
        self.given_object_that_read(self.checkout / "main.c", "generated/tunables.h")
        self.assertEqual(0, self.run_main())
        self.assertIn("| build tree | 1 |", self.output.getvalue())

    def test_an_escaped_space_stays_inside_one_path(self):
        spaced = self.checkout / "with space" / "main.c"
        self.given_file(
            self.build / "CMakeFiles" / "target.dir" / "main.c.obj.d",
            f"main.c.obj: {spaced.as_posix().replace(' ', chr(92) + ' ')}\n",
        )
        self.assertEqual(0, self.run_main())
        self.assertIn("1 distinct files read", self.output.getvalue())

    def test_summary_shows_the_kernel_version_pairing(self):
        self.given_object_that_read(self.checkout / "main.c")
        self.run_main()
        self.assertIn("the pinned kernel is V11.3.3; the kernel CMSIS-FreeRTOS bundles is V11.3.0", self.output.getvalue())

    def test_an_extra_allowed_directory_is_accepted(self):
        extra = self.root / "extra"
        self.given_object_that_read(self.checkout / "main.c", extra / "x.h")
        self.assertEqual(0, self.run_main("--allow", str(extra)))

    def test_appends_the_summary_to_the_github_step_summary(self):
        self.given_object_that_read(self.checkout / "main.c")
        step_summary = self.root / "summary.md"
        with mock.patch.dict(os.environ, {"GITHUB_STEP_SUMMARY": str(step_summary)}):
            self.run_main()
        self.assertIn("### Provenance of `cross`", step_summary.read_text(encoding="utf-8"))

    def test_returns_two_when_nothing_was_built(self):
        self.assertEqual(2, self.run_main())
        self.assertIn("was it built?", self.output.getvalue())


if __name__ == "__main__":
    unittest.main()
