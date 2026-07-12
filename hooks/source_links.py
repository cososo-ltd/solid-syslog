"""MkDocs build hook: rewrite repo-relative source links to canonical GitHub URLs.

The docs live under ``docs/`` but link to source files outside it
(``../Core/...``, ``../Platform/...``, ``../SECURITY.md``, and so on). Those
relative links are correct when the Markdown is read on GitHub, in a clone, in a
fork, or in the IDE, but they escape the built site's tree and would 404 on the
published pages.

This hook keeps the source Markdown relative and, at build time only, rewrites
any link that resolves *outside* ``docs/`` to
``{repo_url}/blob|tree/{ref}/{path}``. Links that stay within ``docs/``,
absolute URLs, in-page anchors, and mail/tel links are left untouched. It runs
in ``on_page_markdown`` (before MkDocs resolves and validates relative links),
so the site can keep strict ``not_found`` link validation.

A file target maps to ``/blob/``; a directory target (trailing slash) maps to
``/tree/``. The ref defaults to ``main`` and is pinned per build via the
``SOLIDSYSLOG_DOCS_REF`` environment variable (set it to a release tag or commit
SHA for a versioned or published build so the links stay self-consistent).

Rewriting is skipped inside fenced code blocks and inline code spans so example
code is never touched. Fence matching is delimiter-aware (a closer must be the
same fence character, at least as long as the opener, and carry no info string);
inline spans are matched by equal-length backtick runs.
"""

import os
import posixpath
import re

# Inline Markdown link / image: [text](dest) with an optional "...", '...', or
# (...) title.
_LINK = re.compile(
    r'''(!?\[[^\]]*\])\(\s*(<[^>]+>|[^)\s]+)((?:\s+(?:"[^"]*"|'[^']*'|\([^)]*\)))?)\s*\)'''
)
# Link reference definition: [id]: dest "title"
_LINK_DEF = re.compile(r'^(\s*\[[^\]]+\]:\s*)(<[^>]+>|\S+)(.*)$')
# A fenced-code delimiter line: optional indent, then a run of >=3 backticks or
# tildes, then the rest of the line (an info string on an opener, blank on a
# closer).
_FENCE = re.compile(r'^(?P<indent>\s*)(?P<marker>`{3,}|~{3,})(?P<rest>.*)$')
# Inline code span: a run of N backticks up to the next run of exactly N.
_INLINE_CODE = re.compile(r'(?P<ticks>`+)(?:[^`]|(?!(?P=ticks)(?!`))`)*?(?P=ticks)(?!`)')
_LEADING_UP = re.compile(r'^(?:\.\./)+')
_MASK_RE = re.compile('\x00(\\d+)\x00')


def _ref():
    return os.environ.get('SOLIDSYSLOG_DOCS_REF', '').strip() or 'main'


def _rewrite(target, src_dir, repo_url):
    """Return the canonical URL for an out-of-docs target, or None to leave it."""
    inner = target
    if inner.startswith('<') and inner.endswith('>'):
        inner = inner[1:-1]
    inner = inner.strip()
    if not inner or inner.startswith(('http://', 'https://', '//', '#', 'mailto:', 'tel:')):
        return None
    path, sep, frag = inner.partition('#')
    if not path:
        return None
    resolved = posixpath.normpath(posixpath.join(src_dir, path))
    if not resolved.startswith('..'):
        return None  # resolves within docs/ -- a normal internal link
    repo_path = _LEADING_UP.sub('', resolved)
    kind = 'tree' if path.endswith('/') else 'blob'
    url = '{}/{}/{}/{}'.format(repo_url, kind, _ref(), repo_path)
    if sep:
        url += '#' + frag
    return url


def _fence_opener(line):
    """Return the opener marker string if the line opens a fenced code block."""
    match = _FENCE.match(line)
    if match is None:
        return None
    marker = match.group('marker')
    # A backtick fence's info string may not contain a backtick (CommonMark);
    # a tilde fence's info string is unrestricted.
    if marker[0] == '`' and '`' in match.group('rest'):
        return None
    return marker


def _is_fence_closer(line, opener):
    """A closer: same fence char, length >= opener, and no info string."""
    match = _FENCE.match(line)
    if match is None:
        return False
    marker = match.group('marker')
    return (
        marker[0] == opener[0]
        and len(marker) >= len(opener)
        and match.group('rest').strip() == ''
    )


def on_page_markdown(markdown, page, config, files, **kwargs):
    repo_url = (config.get('repo_url') or '').rstrip('/')
    if not repo_url:
        return markdown
    src = page.file.src_path.replace(os.sep, '/')
    src_dir = posixpath.dirname(src)

    def link_sub(match):
        new = _rewrite(match.group(2), src_dir, repo_url)
        if new is None:
            return match.group(0)
        return '{}({}{})'.format(match.group(1), new, match.group(3))

    def def_sub(match):
        new = _rewrite(match.group(2), src_dir, repo_url)
        if new is None:
            return match.group(0)
        return '{}{}{}'.format(match.group(1), new, match.group(3))

    def rewrite_line(line):
        spans = []

        def mask(match):
            spans.append(match.group(0))
            return '\x00{}\x00'.format(len(spans) - 1)

        masked = _INLINE_CODE.sub(mask, line)
        masked = _LINK.sub(link_sub, masked)
        masked = _LINK_DEF.sub(def_sub, masked)
        return _MASK_RE.sub(lambda m: spans[int(m.group(1))], masked)

    out = []
    opener = None  # opening fence marker while inside a fenced code block
    for line in markdown.split('\n'):
        if opener is not None:
            out.append(line)
            if _is_fence_closer(line, opener):
                opener = None
            continue
        marker = _fence_opener(line)
        if marker is not None:
            opener = marker
            out.append(line)
            continue
        out.append(rewrite_line(line))
    return '\n'.join(out)
