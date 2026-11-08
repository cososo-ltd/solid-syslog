"""One sentence saying which build a measurement came from.

The Testing pages carry figures measured by the CI run that built the site, so
each says where it was measured: the branch and commit, the pull request, or the
release. The variables are the ones the workflows set for the footer line in
``hooks/build_version.py``: ``SOLIDSYSLOG_DOCS_RELEASE`` on a release build,
otherwise ``SOLIDSYSLOG_DOCS_ORIGIN`` and ``SOLIDSYSLOG_DOCS_COMMIT``. Unlike the
footer this has no use for the last release number, so it does not wait for it.
"""

SHORT_COMMIT_LENGTH = 7


def provenance(environ, verb="Measured"):
    release = environ.get("SOLIDSYSLOG_DOCS_RELEASE", "").strip()
    if release:
        return f"{verb} for release {release}."
    origin = environ.get("SOLIDSYSLOG_DOCS_ORIGIN", "").strip()
    commit = environ.get("SOLIDSYSLOG_DOCS_COMMIT", "").strip()[:SHORT_COMMIT_LENGTH]
    if origin and commit:
        return f"{verb} from {origin} at {commit}."
    return f"{verb} by a local build."
