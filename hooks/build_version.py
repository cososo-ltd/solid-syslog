"""MkDocs build hook: state which build of the library the site documents."""

SHORT_COMMIT_LENGTH = 7


def version_line(environ):
    release = environ.get('SOLIDSYSLOG_DOCS_RELEASE', '')
    if release:
        return f'Documentation for release {release}.'
    branch = environ['SOLIDSYSLOG_DOCS_BRANCH']
    commit = environ['SOLIDSYSLOG_DOCS_COMMIT'][:SHORT_COMMIT_LENGTH]
    version = environ['SOLIDSYSLOG_DOCS_VERSION']
    return f'Documentation built from {branch} at {commit}, after release {version}.'
