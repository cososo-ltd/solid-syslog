"""Regression tests for the build-version hook (hooks/build_version.py).

Run:  python3 hooks/test_build_version.py
      (or: python3 -m unittest discover -s hooks -p 'test_*.py')
"""

import os
import sys
import unittest
from unittest import mock

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

    def test_branch_build_reports_the_branch_commit_and_release_it_was_given(self):
        self.assertEqual(
            'Documentation built from docs/s23-05 at abc1234, after release 0.2.0.',
            h.version_line({
                'SOLIDSYSLOG_DOCS_BRANCH': 'docs/s23-05',
                'SOLIDSYSLOG_DOCS_COMMIT': 'abc1234def5678',
                'SOLIDSYSLOG_DOCS_VERSION': '0.2.0',
            }))

    def test_release_build_names_the_release_alone(self):
        self.assertEqual(
            'Documentation for release 0.2.0.',
            h.version_line({
                'SOLIDSYSLOG_DOCS_RELEASE': '0.2.0',
                'SOLIDSYSLOG_DOCS_BRANCH': 'main',
                'SOLIDSYSLOG_DOCS_COMMIT': 'abc1234def5678',
                'SOLIDSYSLOG_DOCS_VERSION': '0.2.0',
            }))

    def test_unset_environment_is_a_local_build(self):
        self.assertEqual('Local documentation build.', h.version_line({}))

    def test_blank_variables_are_a_local_build(self):
        self.assertEqual(
            'Local documentation build.',
            h.version_line({
                'SOLIDSYSLOG_DOCS_RELEASE': '  ',
                'SOLIDSYSLOG_DOCS_BRANCH': '  ',
                'SOLIDSYSLOG_DOCS_COMMIT': '  ',
                'SOLIDSYSLOG_DOCS_VERSION': '  ',
            }))


class ConfigInjection(unittest.TestCase):
    def test_on_config_publishes_the_line_for_the_footer(self):
        config = {'extra': {}}
        with mock.patch.dict(os.environ, {'SOLIDSYSLOG_DOCS_RELEASE': '0.2.0'}, clear=True):
            h.on_config(config)
        self.assertEqual('Documentation for release 0.2.0.', config['extra']['build_version'])

    def test_on_config_is_not_swayed_by_the_developer_s_own_environment(self):
        config = {'extra': {}}
        with mock.patch.dict(os.environ, {}, clear=True):
            h.on_config(config)
        self.assertEqual('Local documentation build.', config['extra']['build_version'])


if __name__ == '__main__':
    unittest.main()
