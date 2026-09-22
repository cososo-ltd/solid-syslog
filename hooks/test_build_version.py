"""Regression tests for the build-version hook (hooks/build_version.py).

Run:  python3 hooks/test_build_version.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_version as h  # noqa: E402


class VersionLine(unittest.TestCase):
    def test_branch_build_names_the_branch_commit_and_last_release(self):
        self.assertEqual(
            'Documentation built from main at 73f3646, after release 0.1.0.',
            h.version_line({
                'SOLIDSYSLOG_DOCS_BRANCH': 'main',
                'SOLIDSYSLOG_DOCS_COMMIT': '73f3646a1b2c3d4e5f60718293a4b5c6d7e8f901',
                'SOLIDSYSLOG_DOCS_VERSION': '0.1.0',
            }))


if __name__ == '__main__':
    unittest.main()
