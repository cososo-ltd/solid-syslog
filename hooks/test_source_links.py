"""Regression tests for the source-link rewrite hook (hooks/source_links.py).

Run:  python3 hooks/test_source_links.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')

Covers the Markdown-parsing edge cases raised in review: fence character /
length awareness and closer validation, backtick-in-info-string openers,
equal-length inline-code masking, and the single-quoted / parenthesised
link-title forms -- plus the core out-of-docs vs in-docs rewrite behaviour.
"""

import os
import sys
import types
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import source_links as h  # noqa: E402

REPO = 'https://github.com/cososo-ltd/solid-syslog'
BT = chr(96)  # backtick, kept out of the source so editors do not fight it


def render(markdown, src_path='porting.md', repo_url=REPO):
    page = types.SimpleNamespace(file=types.SimpleNamespace(src_path=src_path))
    return h.on_page_markdown(markdown, page, {'repo_url': repo_url}, None)


class RewriteTargets(unittest.TestCase):
    def test_file_link_uses_blob(self):
        self.assertIn(REPO + '/blob/main/Core/Interface/Foo.h',
                      render('[h](../Core/Interface/Foo.h)'))

    def test_directory_link_uses_tree(self):
        self.assertIn(REPO + '/tree/main/Core/Interface',
                      render('[d](../Core/Interface/)'))

    def test_parent_of_docs_from_subpage(self):
        self.assertIn(REPO + '/blob/main/CONTRIBUTING.md',
                      render('[c](../../CONTRIBUTING.md)', src_path='security/threat-model.md'))

    def test_in_docs_link_untouched(self):
        self.assertEqual('[rp](../release-process.md)',
                         render('[rp](../release-process.md)', src_path='security/x.md'))

    def test_internal_anchor_and_absolute_untouched(self):
        self.assertEqual('[a](build-integration.md#tunables)',
                         render('[a](build-integration.md#tunables)'))
        self.assertEqual('[a](#section)', render('[a](#section)'))
        self.assertEqual('[a](https://example.com/x)',
                         render('[a](https://example.com/x)'))

    def test_fragment_preserved(self):
        self.assertIn(REPO + '/blob/main/README.md#architecture',
                      render('[a](../README.md#architecture)'))

    def test_ref_env_override(self):
        os.environ['SOLIDSYSLOG_DOCS_REF'] = 'v1.2.3'
        try:
            self.assertIn(REPO + '/blob/v1.2.3/Core/Foo.h', render('[h](../Core/Foo.h)'))
        finally:
            del os.environ['SOLIDSYSLOG_DOCS_REF']


class PublishedRootPages(unittest.TestCase):
    """A root document the site publishes resolves to its page, not to GitHub.

    Sending a reader out to the repository host for the licence or the security
    policy is what hooks/root_pages.py exists to prevent, so these targets are
    re-aimed rather than rewritten.
    """

    def test_published_root_page_from_subpage(self):
        self.assertEqual('[s](policy.md)',
                         render('[s](../../SECURITY.md)', src_path='security/triage-runbook.md'))

    def test_published_root_page_from_top_level_page(self):
        self.assertEqual('[l](license.md)', render('[l](../LICENSE.md)'))

    def test_published_root_page_keeps_fragment(self):
        self.assertEqual('[l](license.md#continuity)',
                         render('[l](../LICENSE.md#continuity)'))

    def test_published_licence_text_from_subpage(self):
        self.assertEqual(
            '[p](../licenses/polyform-noncommercial-1.0.0.md)',
            render('[p](../../LICENSES/PolyForm-Noncommercial-1.0.0.md)',
                   src_path='security/sbom.md'),
        )

    def test_unpublished_root_page_still_goes_to_the_host(self):
        self.assertIn(REPO + '/blob/main/CODE_OF_CONDUCT.md',
                      render('[c](../CODE_OF_CONDUCT.md)'))


class LinkTitles(unittest.TestCase):
    def test_double_quoted_title(self):
        out = render('[h](../Core/Foo.h "Title")')
        self.assertIn(REPO + '/blob/main/Core/Foo.h', out)
        self.assertIn('"Title"', out)

    def test_single_quoted_title(self):
        out = render("[h](../Core/Foo.h 'Title')")
        self.assertIn(REPO + '/blob/main/Core/Foo.h', out)
        self.assertIn("'Title'", out)

    def test_parenthesised_title(self):
        self.assertIn(REPO + '/blob/main/Core/Foo.h',
                      render('[h](../Core/Foo.h (Title))'))


class CodeExclusion(unittest.TestCase):
    def test_triple_fence_content_untouched(self):
        self.assertNotIn('blob/main/Core/Y.h',
                         render(BT * 3 + 'c\n// [x](../Core/Y.h)\n' + BT * 3))

    def test_four_backtick_fence_not_closed_by_three(self):
        md = BT * 4 + '\n' + BT * 3 + '\n[x](../Core/Y.h)\n' + BT * 4
        self.assertNotIn('blob/main/Core/Y.h', render(md))

    def test_delimiter_line_with_content_is_not_a_closer(self):
        md = BT * 3 + '\n' + BT * 3 + 'still-code\n[x](../Core/Y.h)\n' + BT * 3
        self.assertNotIn('blob/main/Core/Y.h', render(md))

    def test_inline_span_with_inner_backtick(self):
        line = 'Use ' + BT * 2 + 'a' + BT + 'b' + BT * 2 + ' then [z](../Core/Z.h).'
        out = render(line)
        self.assertIn(BT * 2 + 'a' + BT + 'b' + BT * 2, out)
        self.assertIn(REPO + '/blob/main/Core/Z.h', out)

    def test_link_inside_inline_code_untouched(self):
        self.assertNotIn('blob/main/Core/Y.h',
                         render('see ' + BT + '[x](../Core/Y.h)' + BT + ' here'))


class Helpers(unittest.TestCase):
    def test_fence_opener_rejects_backtick_in_info_string(self):
        self.assertIsNone(h._fence_opener(BT * 3 + 'c' + BT))
        self.assertEqual(BT * 3, h._fence_opener(BT * 3 + 'c'))
        self.assertEqual(BT * 4, h._fence_opener(BT * 4))

    def test_is_fence_closer_char_and_length(self):
        self.assertTrue(h._is_fence_closer(BT * 3, BT * 3))
        self.assertTrue(h._is_fence_closer(BT * 4, BT * 3))       # longer closer ok
        self.assertFalse(h._is_fence_closer(BT * 3, BT * 4))      # shorter is not
        self.assertFalse(h._is_fence_closer('~~~', BT * 3))       # wrong fence char
        self.assertFalse(h._is_fence_closer(BT * 3 + 'x', BT * 3))  # has info string

    def test_inline_code_matches_equal_length_runs(self):
        m = h._INLINE_CODE.search('a ' + BT + 'code' + BT + ' b')
        self.assertEqual(BT + 'code' + BT, m.group(0))
        m2 = h._INLINE_CODE.search(BT * 2 + 'a' + BT + 'b' + BT * 2)
        self.assertEqual(BT * 2 + 'a' + BT + 'b' + BT * 2, m2.group(0))


if __name__ == '__main__':
    unittest.main()
