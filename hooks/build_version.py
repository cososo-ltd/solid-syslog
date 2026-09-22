"""MkDocs build hook: say which build of the library the site documents.

The site publishes from ``main`` on every push, so what a reader sees is a
moving target that is usually ahead of every release. Until this hook there was
nothing on any page saying so, and nothing to check a pinned build against.

A bare version number would be the wrong fix: printing the last release on a
site built from ``main`` is wrong for every commit but the tagged one. So a
branch build names itself as one, and carries the commit that pins it and the
release it follows::

    Documentation built from main at 73f3646, after release 0.1.0.

A release build names the release alone. A commit appended there would undercut
the claim the line exists to make, that this is the documentation for that
release; the commit belongs in the offline bundle's manifest instead::

    Documentation for release 0.2.0.

A build with none of it set says so rather than inventing a provenance::

    Local documentation build.

The workflow supplies the parts, because only it knows them:
``SOLIDSYSLOG_DOCS_BRANCH``, ``SOLIDSYSLOG_DOCS_COMMIT``,
``SOLIDSYSLOG_DOCS_VERSION`` (the last release, read from
``.release-please-manifest.json``) and ``SOLIDSYSLOG_DOCS_RELEASE`` on a release
build only. Deliberately not ``git describe``: it needs full history, which
``docs-build`` does not check out, and its ``v0.1.0-77-g73f3646a`` reads as a
version string when it is not one.

``overrides/partials/copyright.html`` renders the result.
"""

import os

SHORT_COMMIT_LENGTH = 7


def version_line(environ):
    release = environ.get('SOLIDSYSLOG_DOCS_RELEASE', '').strip()
    if release:
        return f'Documentation for release {release}.'
    branch = environ.get('SOLIDSYSLOG_DOCS_BRANCH', '').strip()
    commit = environ.get('SOLIDSYSLOG_DOCS_COMMIT', '').strip()[:SHORT_COMMIT_LENGTH]
    version = environ.get('SOLIDSYSLOG_DOCS_VERSION', '').strip()
    if branch and commit and version:
        return f'Documentation built from {branch} at {commit}, after release {version}.'
    return 'Local documentation build.'


def on_config(config, **kwargs):
    config['extra']['build_version'] = version_line(os.environ)
    return config
