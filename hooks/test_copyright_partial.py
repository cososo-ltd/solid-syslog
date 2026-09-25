"""Regression tests for the footer partial (overrides/partials/copyright.html).

Run:  python3 hooks/test_copyright_partial.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')

It lives beside the hook tests because that is where the docs-build lane runs
discovery, and because the value it renders is the one hooks/build_version.py
produces.

MkDocs builds its Jinja environment without autoescape, so every value a
template interpolates is emitted raw. The build-version line carries a branch
name, which on a fork's pull request is attacker-controlled, so the escape here
is load-bearing rather than defensive tidiness.
"""

import datetime
import os
import sys
import unittest

import jinja2

PARTIALS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        'overrides', 'partials')

# MkDocs supplies these to the real environment; the partial only needs them to
# render, so a stub that returns the path unchanged is enough.
BUILD_DATE = datetime.datetime(2026, 9, 25)


def render(build_version):
    env = jinja2.Environment(loader=jinja2.FileSystemLoader(PARTIALS))
    env.filters['url'] = lambda path: path
    config = {'copyright': 'Cozens Software Solutions Limited', 'extra': {}}
    if build_version is not None:
        config['extra']['build_version'] = build_version
    return env.get_template('copyright.html').render(config=config, build_date_utc=BUILD_DATE)


class BuildVersionLine(unittest.TestCase):
    def test_a_branch_name_carrying_markup_cannot_reach_the_page_as_markup(self):
        html = render('Documentation built from x"><script>alert(1)</script> at deadbee.')
        self.assertNotIn('<script>', html)
        self.assertIn('&lt;script&gt;alert(1)&lt;/script&gt;', html)

    def test_an_ordinary_line_renders_as_written(self):
        html = render('Documentation built from main at 397ea91, after release 0.1.0.')
        self.assertIn('<div class="ss-footer-build">Documentation built from main at '
                      '397ea91, after release 0.1.0.</div>', html)

    def test_no_line_leaves_no_empty_container(self):
        self.assertNotIn('ss-footer-build', render(None))


if __name__ == '__main__':
    unittest.main()
