"""Regression tests for the meta-description hook (hooks/page_descriptions.py).

Run:  python3 hooks/test_page_descriptions.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')

The strict build already proves the happy path — every nav page carries a
description, or on_nav aborts. What needs its own test is the abort itself, and
the description text, which no build can check for length or duplication.
"""

import os
import sys
import types
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import page_descriptions as h  # noqa: E402

MAX_LENGTH = 200


def nav_of(*src_uris):
    pages = [types.SimpleNamespace(file=types.SimpleNamespace(src_uri=uri)) for uri in src_uris]
    return types.SimpleNamespace(pages=pages)


def page_of(src_uri, meta=None):
    return types.SimpleNamespace(file=types.SimpleNamespace(src_uri=src_uri), meta=meta or {})


class NavValidation(unittest.TestCase):
    def setUp(self):
        self.described = sorted(h.DESCRIPTIONS)

    def test_accepts_a_nav_of_described_pages(self):
        nav = nav_of(*self.described)
        self.assertIs(h.on_nav(nav, {}, None), nav)

    def test_undescribed_nav_page_aborts_the_build_and_is_named(self):
        with self.assertRaises(h.PluginError) as raised:
            h.on_nav(nav_of(*self.described, "brand-new.md"), {}, None)
        self.assertIn("no description for brand-new.md", str(raised.exception))

    def test_description_for_a_page_no_longer_in_the_nav_aborts_the_build(self):
        kept = [uri for uri in self.described if uri != "porting.md"]
        with self.assertRaises(h.PluginError) as raised:
            h.on_nav(nav_of(*kept), {}, None)
        self.assertIn("not in the nav: porting.md", str(raised.exception))

    def test_generated_api_pages_need_no_description(self):
        nav = nav_of(*self.described, "api/files.md")
        self.assertIs(h.on_nav(nav, {}, None), nav)


class PageMeta(unittest.TestCase):
    def test_sets_the_description_for_a_mapped_page(self):
        page = page_of("cra.md")
        h.on_page_markdown("# CRA", page, {}, None)
        self.assertEqual(page.meta["description"], h.DESCRIPTIONS["cra.md"])

    def test_front_matter_wins(self):
        page = page_of("cra.md", {"description": "hand written"})
        h.on_page_markdown("# CRA", page, {}, None)
        self.assertEqual(page.meta["description"], "hand written")

    def test_unmapped_page_falls_back_to_site_description(self):
        page = page_of("assets/postit/README.md")
        h.on_page_markdown("# Post-its", page, {}, None)
        self.assertNotIn("description", page.meta)

    def test_markdown_is_returned_unchanged(self):
        self.assertEqual(h.on_page_markdown("# CRA", page_of("cra.md"), {}, None), "# CRA")


class DescriptionText(unittest.TestCase):
    def test_each_description_is_a_single_line_within_snippet_length(self):
        for src_uri, description in sorted(h.DESCRIPTIONS.items()):
            self.assertNotIn("\n", description, src_uri)
            self.assertLessEqual(len(description), MAX_LENGTH, src_uri)
            self.assertEqual(description, description.strip(), src_uri)

    def test_no_two_pages_share_a_description(self):
        seen = {}
        for src_uri, description in sorted(h.DESCRIPTIONS.items()):
            self.assertNotIn(description, seen, "{} duplicates {}".format(src_uri, seen.get(description)))
            seen[description] = src_uri


if __name__ == "__main__":
    unittest.main()
