"""Tests for check_vxworks64_header_proof.py, against a fake repository.

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
import check_vxworks64_header_proof  # noqa: E402


class HeaderProofTest(unittest.TestCase):
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
            return check_vxworks64_header_proof.main(["--root", str(self.root)])

    def test_passes_when_the_proof_includes_the_only_public_header(self):
        self.given_file("Core/Interface/SolidSyslog.h")
        self.given_file(
            "Bdd/Targets/VxWorks64/BddTargetVxWorks64Headers.c",
            '#include "vxWorks.h"\n\n#include "SolidSyslog.h"\n',
        )
        self.assertEqual(0, self.run_main())

    def test_fails_naming_a_core_header_the_proof_omits(self):
        self.given_file("Core/Interface/SolidSyslog.h")
        self.given_file("Core/Interface/SolidSyslogStore.h")
        self.given_file(
            "Bdd/Targets/VxWorks64/BddTargetVxWorks64Headers.c",
            '#include "vxWorks.h"\n\n#include "SolidSyslog.h"\n',
        )
        self.assertEqual(1, self.run_main())
        self.assertIn("Core/Interface/SolidSyslogStore.h", self.output.getvalue())

    def test_fails_naming_a_pack_header_the_proof_omits(self):
        self.given_file("Platform/VxWorks64/Interface/SolidSyslogVxWorks64File.h")
        self.given_file(
            "Bdd/Targets/VxWorks64/BddTargetVxWorks64Headers.c",
            '#include "vxWorks.h"\n',
        )
        self.assertEqual(1, self.run_main())
        self.assertIn(
            "Platform/VxWorks64/Interface/SolidSyslogVxWorks64File.h",
            self.output.getvalue(),
        )


if __name__ == "__main__":
    unittest.main()
