"""Regression tests for the platform doorway hook (hooks/platform_backlinks.py).

Run:  python3 hooks/test_platform_backlinks.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')

The strict build proves the links resolve. What it cannot prove is that the
right pages get chips and the wrong ones do not — a Core header quietly
labelled with a platform, or a platform page silently missing its doorway,
builds perfectly well.
"""

import os
import sys
import types
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import platform_backlinks as h  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONFIG = {"config_file_path": os.path.join(ROOT, "mkdocs.yml")}


def page_of(src_uri):
    return types.SimpleNamespace(file=types.SimpleNamespace(src_uri=src_uri))


def render(src_uri, markdown="# Title\n\nBody.\n"):
    return h.on_page_markdown(markdown, page_of(src_uri), CONFIG, None)


class GeneratedApiPages(unittest.TestCase):
    def test_a_platform_header_is_labelled_with_its_platform(self):
        out = render("api/SolidSyslogMbedTlsStream_8h.md")
        self.assertIn("[Mbed TLS platform](../platforms/mbedtls/index.md){ .ss-chip .ss-chip--platform }", out)

    def test_the_label_is_the_platform_pages_own_heading(self):
        # Not the registry token, which is "LwipRaw".
        self.assertIn("[lwIP (Raw API) platform]", render("api/SolidSyslogLwipRawTcpStream_8h.md"))

    def test_a_core_header_is_not_labelled(self):
        markdown = "# Core\n\nBody.\n"
        self.assertEqual(render("api/SolidSyslogConfig_8h.md", markdown), markdown)

    def test_a_generated_index_is_not_labelled(self):
        markdown = "# Files\n"
        self.assertEqual(render("api/files.md", markdown), markdown)


class PlatformPages(unittest.TestCase):
    def test_chips_go_under_the_title_not_above_it(self):
        out = render("platforms/mbedtls/index.md")
        self.assertTrue(out.startswith("# Title\n\n["), out[:40])

    def test_a_platform_with_a_setup_guide_gets_both_chips(self):
        out = render("platforms/mbedtls/index.md")
        self.assertIn("[API reference](../../api/group__platform__mbedtls.md){ .ss-chip .ss-chip--api }", out)
        self.assertIn("[Setup](setup.md){ .ss-chip .ss-chip--setup }", out)

    def test_every_platform_has_both_chips_today(self):
        _, slugs, _labels = h._index(CONFIG)
        for slug in slugs:
            out = render(f"platforms/{slug}/index.md")
            self.assertIn(".ss-chip--api", out, slug)
            self.assertIn(".ss-chip--setup", out, slug)

    def test_the_setup_chip_appears_only_when_the_page_does(self):
        # Every platform has a setup page now, so the absent case has to be
        # constructed. The chip must never point at a page that is not there.
        root = os.path.dirname(CONFIG["config_file_path"])
        headers, slugs, labels = h._index(CONFIG)
        h._CACHE[root] = (headers, {**slugs, "posix": False}, labels)
        try:
            out = render("platforms/posix/index.md")
        finally:
            h._CACHE.pop(root, None)
        self.assertIn(".ss-chip--api", out)
        self.assertNotIn(".ss-chip--setup", out)

    def test_the_platforms_overview_is_not_a_platform(self):
        markdown = "# Platforms\n\nBody.\n"
        self.assertEqual(render("platforms/index.md", markdown), markdown)

    def test_a_setup_page_gets_no_chips(self):
        markdown = "# Mbed TLS setup\n\nBody.\n"
        self.assertEqual(render("platforms/mbedtls/setup.md", markdown), markdown)

    def test_an_ordinary_page_is_untouched(self):
        markdown = "# Porting\n\nBody.\n"
        self.assertEqual(render("porting.md", markdown), markdown)


class GroupPages(unittest.TestCase):
    # mkdoxy titles these in Doxygen's vocabulary and escapes the underscore.
    RAW = "# Group platform\\_atomics\n\n[**Modules**](index_groups.md) **>** [**platform\\_atomics**](x.md)\n"

    def test_the_doxygen_title_becomes_the_platform_name(self):
        out = render("api/group__platform__atomics.md", self.RAW)
        self.assertTrue(out.startswith("# C11 atomics platform"), out[:50])

    def test_group_and_modules_do_not_reach_the_reader(self):
        out = render("api/group__platform__atomics.md", self.RAW)
        self.assertNotIn("Group", out)
        self.assertNotIn("Modules", out)
        self.assertNotIn("platform_atomics", out.replace("\\", ""))

    def test_the_breadcrumb_points_at_the_platforms_overview(self):
        out = render("api/group__platform__atomics.md", self.RAW)
        self.assertIn("[**Platforms**](../platforms/index.md)", out)

    def test_the_back_link_stays_inside_this_build(self):
        raw = self.RAW + (
            "Obligations: [https://docs.cososo.co.uk/solid-syslog/platforms/atomics/]"
            "(https://docs.cososo.co.uk/solid-syslog/platforms/atomics/)\n"
        )
        out = render("api/group__platform__atomics.md", raw)
        self.assertIn("[C11 atomics](../platforms/atomics/index.md)", out)
        self.assertNotIn("docs.cososo.co.uk", out)

    def test_a_group_page_gets_no_platform_chip(self):
        out = render("api/group__platform__atomics.md", self.RAW)
        self.assertNotIn("ss-chip", out)


class Registry(unittest.TestCase):
    def test_every_registered_platform_has_a_docs_folder(self):
        _, slugs, _labels = h._index(CONFIG)
        for slug in slugs:
            self.assertTrue(
                os.path.isfile(os.path.join(ROOT, "docs", "platforms", slug, "index.md")),
                f"registry names {slug} but docs/platforms/{slug}/index.md is missing",
            )

    def test_the_registry_yields_all_ten_platforms(self):
        _, slugs, _labels = h._index(CONFIG)
        self.assertEqual(len(slugs), 10, sorted(slugs))


if __name__ == "__main__":
    unittest.main()
