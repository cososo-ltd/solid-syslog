"""MkDocs build hook: state which build of the library the site documents."""

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
