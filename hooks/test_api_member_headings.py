"""Unit tests for the member-heading hook."""

import unittest

import api_member_headings as hook


class FakeFile:
    def __init__(self, src_uri):
        self.src_uri = src_uri
        self.src_path = src_uri


class FakePage:
    def __init__(self, src_uri):
        self.file = FakeFile(src_uri)


def render(markdown, src_uri="api/structX.md"):
    return hook.on_page_markdown(markdown, FakePage(src_uri), None, None)


class DropsTheKind(unittest.TestCase):
    def test_a_member_heading_is_left_with_its_name(self):
        out = render("### variable Acquire\n\nThe entry.\n")
        self.assertIn("### Acquire {", out)

    def test_every_kind_mkdoxy_emits_is_covered(self):
        page = (
            "### function SolidSyslogFile_Close\n"
            "### define SOLIDSYSLOG_FILE_POOL_SIZE\n"
            "### enum SolidSyslogFileErrors\n"
            "### typedef SolidSyslogClockFunction\n"
            "### variable Acquire\n"
        )
        out = render(page, "api/SolidSyslogFile_8h.md")
        for name in ("SolidSyslogFile_Close", "SOLIDSYSLOG_FILE_POOL_SIZE", "SolidSyslogFileErrors", "SolidSyslogClockFunction", "Acquire"):
            self.assertIn("### " + name + " {", out)

    def test_an_anonymous_enum_keeps_its_kind(self):
        """Doxygen names it @0, and a heading reading "@0" says less."""
        page = "### enum @0\n\nThe unnamed one.\n"
        self.assertEqual(render(page, "api/SolidSyslogAtomicCounter_8h.md"), page)

    def test_prose_naming_a_kind_is_left_alone(self):
        out = render("### variable Size\n\nThe variable holds the block count.\n")
        self.assertIn("The variable holds the block count.", out)

    def test_hand_written_pages_are_untouched(self):
        page = "### function SolidSyslog_Log\n"
        self.assertEqual(render(page, "platforms/posix/index.md"), page)


class KeepsTheAnchorTheHeadingHad(unittest.TestCase):
    """Links point at these anchors from inside the site and outside it."""

    def test_a_function_keeps_the_anchor_links_point_at(self):
        out = render("### function SolidSyslogFile_Close\n", "api/SolidSyslogFile_8h.md")
        self.assertIn("### SolidSyslogFile_Close { #function-solidsyslogfile_close }", out)

    def test_a_macro_keeps_the_anchor_links_point_at(self):
        out = render("### define SOLIDSYSLOG_FILE_POOL_SIZE\n", "api/SolidSyslogTunablesDefaults_8h.md")
        self.assertIn("{ #define-solidsyslog_file_pool_size }", out)

    def test_a_member_keeps_the_anchor_links_point_at(self):
        out = render("### variable Acquire\n")
        self.assertIn("{ #variable-acquire }", out)

    def test_a_cross_page_link_to_a_member_still_resolves(self):
        """mkdoxy links header and directory pages at a struct's members."""
        page = "* [**Clock**](structSolidSyslogConfig.md#variable-clock)\n"
        out = render(page, "api/SolidSyslogPosixClock_8h.md")
        self.assertIn("(structSolidSyslogConfig.md#variable-clock)", out)


if __name__ == "__main__":
    unittest.main()
