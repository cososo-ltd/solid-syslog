"""Tests for misra_public_api.py: the rule 8.7 findings deviation D.016 covers."""

import contextlib
import io
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

    def test_a_finding_on_an_object_a_public_header_declares_is_dropped(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "extern const struct SolidSyslogErrorSource SolidSyslogThingErrorSource;\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "const struct SolidSyslogErrorSource SolidSyslogThingErrorSource = {0};\n")
        finding = "Core/Source/SolidSyslogThing.c:1:37: style: misra violation [misra-c2012-8.7]"
        self.assertEqual(misra_public_api.uncovered([finding], self.tree.root.name), [])

    def test_a_finding_on_a_function_no_public_header_declares_stays(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "void SolidSyslogThing_Do(void);\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "void Thing_Helper(void)\n{\n}\n")
        finding = "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]"
        self.assertEqual(misra_public_api.uncovered([finding], self.tree.root.name), [finding])

    def test_a_name_a_public_header_only_mentions_in_a_comment_stays(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "/* Thing_Helper does the work. */\nvoid SolidSyslogThing_Do(void);\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "void Thing_Helper(void)\n{\n}\n")
        finding = "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]"
        self.assertEqual(misra_public_api.uncovered([finding], self.tree.root.name), [finding])

    def test_a_name_a_public_macro_only_expands_to_stays(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "#define SOLIDSYSLOG_THING_CALLBACK Thing_Helper\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "void Thing_Helper(void)\n{\n}\n")
        finding = "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]"
        self.assertEqual(misra_public_api.uncovered([finding], self.tree.root.name), [finding])

    def test_a_name_a_public_header_only_gives_a_parameter_stays(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "void SolidSyslogThing_Do(int Thing_Helper);\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "void Thing_Helper(void)\n{\n}\n")
        finding = "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]"
        self.assertEqual(misra_public_api.uncovered([finding], self.tree.root.name), [finding])

    def test_a_name_a_public_header_only_gives_a_struct_member_stays(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "struct SolidSyslogThing\n{\n    int Thing_Helper;\n    void (*Thing_Other)(void);\n};\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "void Thing_Helper(void)\n{\n}\nvoid Thing_Other(void)\n{\n}\n")
        findings = [
            "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]",
            "Core/Source/SolidSyslogThing.c:4:6: style: misra violation [misra-c2012-8.7]",
        ]
        self.assertEqual(misra_public_api.uncovered(findings, self.tree.root.name), findings)

    def test_a_name_a_public_header_only_gives_a_typedef_stays(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "typedef void (*Thing_Helper)(void);\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "void Thing_Helper(void)\n{\n}\n")
        finding = "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]"
        self.assertEqual(misra_public_api.uncovered([finding], self.tree.root.name), [finding])

    def test_a_name_a_public_header_only_quotes_stays(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "static const char SolidSyslogThing_Name[] = \"Thing_Helper\";\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "void Thing_Helper(void)\n{\n}\n")
        finding = "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]"
        self.assertEqual(misra_public_api.uncovered([finding], self.tree.root.name), [finding])

    def test_a_function_declared_inside_an_extern_c_block_is_dropped(self):
        header = '#ifdef __cplusplus\nextern "C"\n{\n#endif\n\n    void SolidSyslogThing_Do(\n        int count\n    );\n\n#ifdef __cplusplus\n}\n#endif\n'
        self.tree.write("Core/Interface/SolidSyslogThing.h", header)
        self.tree.write("Core/Source/SolidSyslogThing.c", "void SolidSyslogThing_Do(int count)\n{\n}\n")
        finding = "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]"
        self.assertEqual(misra_public_api.uncovered([finding], self.tree.root.name), [])

    def test_the_command_prints_each_uncovered_finding_and_fails(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "void SolidSyslogThing_Do(void);\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "void SolidSyslogThing_Do(void)\n{\n}\n")
        covered = "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]"
        other = "Core/Source/SolidSyslogThing.c:2:1: style: misra violation [misra-c2012-15.5]"
        self.tree.write("cppcheck-misra.log", f"Checking Core/Source/SolidSyslogThing.c ...\n{covered}\n{other}\n")
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = misra_public_api.main([os.path.join(self.tree.root.name, "cppcheck-misra.log"), "--root", self.tree.root.name])
        self.assertEqual(code, 1)
        self.assertEqual(out.getvalue(), other + "\n")

    def test_the_command_passes_when_every_finding_is_covered(self):
        self.tree.write("Core/Interface/SolidSyslogThing.h", "void SolidSyslogThing_Do(void);\n")
        self.tree.write("Core/Source/SolidSyslogThing.c", "void SolidSyslogThing_Do(void)\n{\n}\n")
        self.tree.write("cppcheck-misra.log", "Core/Source/SolidSyslogThing.c:1:6: style: misra violation [misra-c2012-8.7]\n")
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = misra_public_api.main([os.path.join(self.tree.root.name, "cppcheck-misra.log"), "--root", self.tree.root.name])
        self.assertEqual(code, 0)
        self.assertEqual(out.getvalue(), "")

    def test_the_command_fails_on_a_finding_that_is_not_misra(self):
        crash = "Core/Source/SolidSyslogThing.c:0:0: error: Bailing out from analysis: Failed to execute addon 'misra' [internalError]"
        self.tree.write("cppcheck-misra.log", f"Checking Core/Source/SolidSyslogThing.c ...\n{crash}\n")
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = misra_public_api.main([os.path.join(self.tree.root.name, "cppcheck-misra.log"), "--root", self.tree.root.name])
        self.assertEqual(code, 1)
        self.assertEqual(out.getvalue(), crash + "\n")


if __name__ == "__main__":
    unittest.main()
