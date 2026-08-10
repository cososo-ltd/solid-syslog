"""Unit tests for the member-summary hook."""

import os
import unittest

import api_member_summaries as hook


class FakeFile:
    def __init__(self, src_uri):
        self.src_uri = src_uri
        self.src_path = src_uri


class FakePage:
    def __init__(self, src_uri):
        self.file = FakeFile(src_uri)


PAGE = """# Struct

## Detailed Description

Prose that stays.

## Public Attributes

| Type | Name |
| ---: | :--- |
|  void(\\* | [**Format**](#variable-format)  <br>_Write it._ |

## Public Attributes Documentation

### variable Format

The full entry.

## Something Else

Kept.
"""


class DropsRedundantSummaries(unittest.TestCase):
    def test_summary_table_is_removed(self):
        out = hook.on_page_markdown(PAGE, FakePage("api/structX.md"), None, None)
        self.assertNotIn("## Public Attributes\n", out)
        self.assertNotIn("void(\\*", out)

    def test_documentation_section_survives(self):
        out = hook.on_page_markdown(PAGE, FakePage("api/structX.md"), None, None)
        self.assertIn("## Public Attributes Documentation", out)
        self.assertIn("### variable Format", out)

    def test_unrelated_sections_survive(self):
        out = hook.on_page_markdown(PAGE, FakePage("api/structX.md"), None, None)
        self.assertIn("## Detailed Description", out)
        self.assertIn("## Something Else", out)
        self.assertIn("Prose that stays.", out)

    def test_hand_written_pages_are_untouched(self):
        out = hook.on_page_markdown(PAGE, FakePage("platforms/posix/index.md"), None, None)
        self.assertEqual(out, PAGE)

    def test_summary_without_its_documentation_is_kept(self):
        """A summary is only redundant when the detail section carries it."""
        orphan = "# F\n\n## Public Attributes\n\n| Type | Name |\n| ---: | :--- |\n|  int | [**X**](#x) |\n"
        out = hook.on_page_markdown(orphan, FakePage("api/structY.md"), None, None)
        self.assertEqual(out, orphan)

    def test_sections_with_no_counterpart_are_kept(self):
        """Classes / Files / Directories are content, not a preview of it."""
        page = "# Dir\n\n## Files\n\n| file | [**a.h**](a.md) |\n\n## Classes\n\n| struct | [**B**](b.md) |\n"
        out = hook.on_page_markdown(page, FakePage("api/dir_x.md"), None, None)
        self.assertEqual(out, page)

    def test_last_section_on_the_page_is_removed_cleanly(self):
        page = "# S\n\n## Public Types Documentation\n\n### enum E\n\nBody.\n\n## Public Types\n\n| enum | [**E**](#e) |\n"
        out = hook.on_page_markdown(page, FakePage("api/structZ.md"), None, None)
        self.assertIn("## Public Types Documentation", out)
        self.assertNotIn("\n## Public Types\n", out)


BRIEF_PAGE = """# Struct SolidSyslogThing

_A thing._ [More...](#detailed-description)

* `#include <SolidSyslogThing.h>`

## Detailed Description

The rest of the description.
"""


class MergesTheBrief(unittest.TestCase):
    def test_brief_moves_into_the_detailed_description(self):
        out = hook.on_page_markdown(BRIEF_PAGE, FakePage("api/structSolidSyslogThing.md"), None, None)
        self.assertNotIn("More...", out)
        detail = out.split("## Detailed Description")[1]
        self.assertIn("A thing.", detail)
        self.assertIn("The rest of the description.", detail)

    def test_brief_is_kept_when_there_is_no_detailed_description(self):
        page = "# Struct X\n\n_Only a brief._ [More...](#detailed-description)\n"
        out = hook.on_page_markdown(page, FakePage("api/structX.md"), None, None)
        self.assertIn("Only a brief.", out)


class InjectsTheDeclaration(unittest.TestCase):
    def setUp(self):
        import tempfile

        self.root = tempfile.mkdtemp()
        interface = os.path.join(self.root, "Core", "Interface")
        os.makedirs(interface)
        with open(os.path.join(interface, "SolidSyslogThing.h"), "w", encoding="utf-8") as handle:
            handle.write(
                "/** @file brief */\n"
                "struct SolidSyslogThing\n"
                "{\n"
                "    /** doc that should not appear */\n"
                "    void (*Format)(struct SolidSyslogThing* base);\n"
                "    struct { int Nested; } Inner;\n"
                "};\n"
            )

    def test_declaration_is_injected_after_the_include(self):
        out = hook.on_page_markdown(BRIEF_PAGE, FakePage("api/structSolidSyslogThing.md"), {"config_file_path": os.path.join(self.root, "mkdocs.yml")}, None)
        self.assertIn("```c", out)
        self.assertIn("void (*Format)(struct SolidSyslogThing* base);", out)

    def test_nested_braces_do_not_truncate_the_declaration(self):
        out = hook.on_page_markdown(BRIEF_PAGE, FakePage("api/structSolidSyslogThing.md"), {"config_file_path": os.path.join(self.root, "mkdocs.yml")}, None)
        self.assertIn("Inner;", out)
        self.assertIn("};", out)

    def test_member_comments_are_stripped(self):
        out = hook.on_page_markdown(BRIEF_PAGE, FakePage("api/structSolidSyslogThing.md"), {"config_file_path": os.path.join(self.root, "mkdocs.yml")}, None)
        self.assertNotIn("doc that should not appear", out)

    def test_declaration_is_dedented(self):
        """The real headers indent inside SOLIDSYSLOG_EXTERN_C_BEGIN."""
        interface = os.path.join(self.root, "Core", "Interface")
        with open(os.path.join(interface, "SolidSyslogThing.h"), "w", encoding="utf-8") as handle:
            handle.write(
                "SOLIDSYSLOG_EXTERN_C_BEGIN\n"
                "\n"
                "    struct SolidSyslogThing\n"
                "    {\n"
                "        void (*Format)(struct SolidSyslogThing* base);\n"
                "    };\n"
            )
        out = hook.on_page_markdown(BRIEF_PAGE, FakePage("api/structSolidSyslogThing.md"), {"config_file_path": os.path.join(self.root, "mkdocs.yml")}, None)
        self.assertIn("\nstruct SolidSyslogThing\n{\n    void (*Format)", out)

    def test_missing_header_leaves_the_page_alone(self):
        page = BRIEF_PAGE.replace("SolidSyslogThing.h", "SolidSyslogAbsent.h")
        out = hook.on_page_markdown(page, FakePage("api/structSolidSyslogThing.md"), {"config_file_path": os.path.join(self.root, "mkdocs.yml")}, None)
        self.assertNotIn("```c", out)


if __name__ == "__main__":
    unittest.main()
