"""Tests for misra_public_api.py: the rule 8.7 findings deviation D.016 covers."""

import os
import tempfile
import unittest

import misra_public_api


class ATree:
    def __init__(self):
        self.root = tempfile.TemporaryDirectory()

    def write(self, relative, text):
        path = os.path.join(self.root.name, relative)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(text)

    def cleanup(self):
        self.root.cleanup()


class Filtering(unittest.TestCase):
    def setUp(self):
        self.tree = ATree()

    def tearDown(self):
        self.tree.cleanup()

    def test_a_finding_on_a_function_a_public_header_declares_is_dropped(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "void SolidSyslogThing_Do(void);\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "#include \"SolidSyslogThing.h\"\nvoid SolidSyslogThing_Do(void)\n{\n}\n")
        finding = "Core/Source/SolidSyslogThing.c:2:6: style: misra violation [misra-c2012-8.7]"
        self.assertEqual(misra_public_api.uncovered([finding], self.tree.root.name), [])


if __name__ == "__main__":
    unittest.main()
