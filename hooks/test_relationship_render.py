"""Regression tests for the relationship diagrams' page links.

Run:  python3 hooks/test_relationship_render.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')

The diagrams are injected as raw SVG, so the links inside them are never seen by
MkDocs' link resolution: whatever shape they are written in is the shape that
ships. They were written for the published site, which uses directory URLs. The
offline bundle does not, so the shape has to follow the build.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import relationship_diagrams as diagrams  # noqa: E402
import relationship_render as render  # noqa: E402


class PageLinks(unittest.TestCase):
    def tearDown(self):
        render.use_directory_urls(True)

    def test_a_directory_url_build_links_to_the_page_as_a_directory(self):
        render.use_directory_urls(True)
        self.assertEqual('../SolidSyslogAddress_8h/', render.page_href('SolidSyslogAddress_8h'))

    def test_a_flat_build_links_to_the_sibling_file_without_climbing(self):
        render.use_directory_urls(False)
        self.assertEqual('SolidSyslogAddress_8h.html', render.page_href('SolidSyslogAddress_8h'))

    def test_the_hook_takes_the_shape_from_the_build_being_configured(self):
        diagrams.on_config({'use_directory_urls': False})
        self.assertEqual('SolidSyslogAddress_8h.html', render.page_href('SolidSyslogAddress_8h'))
        diagrams.on_config({'use_directory_urls': True})
        self.assertEqual('../SolidSyslogAddress_8h/', render.page_href('SolidSyslogAddress_8h'))


if __name__ == '__main__':
    unittest.main()
