"""Regression tests for the root-document hook (hooks/root_pages.py).

Run:  python3 hooks/test_root_pages.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')

The strict build proves the published pages exist and their links resolve. What
it cannot prove is that the right files are published and that a link left
pointing at the repository root is caught rather than quietly rendered — a
SECURITY.md whose reporting route silently vanished still builds perfectly well.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import root_pages as h  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def relink(markdown, published_at):
    return h._relink(markdown, published_at, ROOT)


class WhatIsPublished(unittest.TestCase):
    def test_the_security_policy_is_published(self):
        # Five pages link it and it has no in-site equivalent.
        self.assertEqual(h.PUBLISHED["SECURITY.md"], "security/policy.md")

    def test_every_published_source_exists_at_the_root(self):
        for source in h.PUBLISHED:
            self.assertTrue(os.path.isfile(os.path.join(ROOT, source)), source)

    def test_the_maintainers_working_agreement_is_not_published(self):
        # CLAUDE.md is how we work, not part of the product's documentation.
        self.assertNotIn("CLAUDE.md", h.PUBLISHED)


class Relinking(unittest.TestCase):
    def test_a_docs_link_is_made_relative_to_the_published_page(self):
        out = relink("See [the model](docs/security/threat-model.md).", "security/policy.md")
        self.assertIn("[the model](threat-model.md)", out)

    def test_a_docs_link_from_a_page_at_the_site_root_keeps_its_path(self):
        out = relink("See [the model](docs/security/threat-model.md).", "support.md")
        self.assertIn("[the model](security/threat-model.md)", out)

    def test_docs_on_its_own_means_the_landing_page(self):
        self.assertIn("[docs](README.md)", relink("[docs](docs/)", "support.md"))

    def test_a_link_to_another_published_root_file_finds_its_new_home(self):
        out = relink("[Security issues](SECURITY.md)", "support.md")
        self.assertIn("[Security issues](security/policy.md)", out)

    def test_a_link_to_an_unpublished_root_file_becomes_plain_text(self):
        # Publishing the target instead would cascade — CONTRIBUTING.md reaches
        # CLAUDE.md — so the link is dropped rather than followed out of the site.
        out = relink("See [Contributing](CONTRIBUTING.md) first.", "support.md")
        self.assertEqual(out, "See Contributing first.")

    def test_an_external_url_is_left_alone(self):
        url = "[Talk to us](https://www.cososo.co.uk/?service=solidsyslog#contact)"
        self.assertEqual(relink(url, "support.md"), url)

    def test_a_fragment_survives_the_rewrite(self):
        out = relink("[scope](docs/security/threat-model.md#scope)", "security/policy.md")
        self.assertIn("[scope](threat-model.md#scope)", out)

    def test_a_link_already_inside_the_docs_tree_is_untouched(self):
        # Only a docs/-prefixed path is the root file's way of reaching a page.
        self.assertEqual(relink("[x](other.md)", "support.md"), "[x](other.md)")


class RealFiles(unittest.TestCase):
    def test_the_published_security_policy_keeps_a_route_to_the_threat_model(self):
        with open(os.path.join(ROOT, "SECURITY.md"), encoding="utf-8") as handle:
            out = relink(handle.read(), h.PUBLISHED["SECURITY.md"])
        self.assertIn("(threat-model.md)", out)
        self.assertNotIn("(docs/", out)


if __name__ == "__main__":
    unittest.main()
