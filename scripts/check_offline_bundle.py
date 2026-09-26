#!/usr/bin/env python3
"""Assert that a built documentation bundle works with no network and no server.

The bundle attached to a release is opened by extracting it and double-clicking
``index.html``. That is a ``file://`` URI, which breaks two things an ordinary
MkDocs build relies on, and neither breaks loudly:

- **Directory links.** With ``use_directory_urls`` a link reads ``tls/``, which
  a browser resolves against the filesystem as a folder and renders as a listing
  rather than the page. ``mkdocs-offline.yml`` turns the setting off; this
  asserts the result, by resolving every relative link against the bundle.
- **External resources.** A stylesheet, script or image fetched from another
  host simply does not load, so the page renders subtly wrong. Worse, an
  analytics beacon in an artefact handed to a customer as evidence calls home.
  This asserts that nothing outside the bundle is fetched, from the markup and
  from inside the stylesheets alike: a CSS ``@import`` or ``url()`` is a network
  request that no amount of reading the HTML will reveal.

A link this cannot check is one built by JavaScript at runtime. Material's search
is exactly that, and is disabled in the bundle for its own reasons.

Usage:  python3 scripts/check_offline_bundle.py <bundle-dir>
"""

import html.parser
import os
import re
import sys
import urllib.parse

# Tags whose attribute makes the browser fetch a resource. An <a href> is a
# navigation the reader chooses; these load without being asked, so an external
# one is a silent network call on every page view.
RESOURCE = {
    'link': 'href',
    'script': 'src',
    'img': 'src',
    'source': 'src',
    'iframe': 'src',
}
NAVIGATION = {'a': 'href'}

# A fetch from another host. For a resource this is the fault the bundle exists
# to avoid; for an <a> it is a link the reader chooses to follow, which is fine.
NETWORK_SCHEMES = ('http://', 'https://', '//')

# Addresses that resolve without either the network or a file beside the page.
# A data: URI in particular is self-contained, so it is not a fault.
INERT_SCHEMES = ('data:', 'mailto:', 'tel:', 'javascript:', '#')

# A <link> only fetches for the relations that load on their own.
FETCHING_RELATIONS = frozenset({
    'stylesheet', 'icon', 'shortcut', 'apple-touch-icon',
    'preload', 'modulepreload', 'prefetch', 'manifest',
})

# A CSS comment. Stripped before the scan below, because prose mentioning
# @import or url() is not a fetch, and a checker that says otherwise trains
# people to write around it.
CSS_COMMENT = re.compile(r'/\*.*?\*/', re.DOTALL)

# @import "x" / @import url(x) / url(x) inside a stylesheet.
CSS_TARGET = re.compile(
    r'''@import\s+(?:url\(\s*)?["']?([^"')\s;]+)|url\(\s*["']?([^"')\s]+)''',
    re.IGNORECASE,
)


class Links(html.parser.HTMLParser):
    """Collects (kind, attribute value) for every link and resource in a page."""

    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.found = []

    def handle_starttag(self, tag, attrs):
        values = dict(attrs)
        if tag in RESOURCE:
            target = values.get(RESOURCE[tag])
            # A <link> is only a fetch for the kinds that load automatically.
            # rel is a space-separated token list, so it is matched by token
            # rather than as a whole string: rel="preload stylesheet" fetches
            # exactly as rel="stylesheet" does.
            if tag == 'link':
                relations = set((values.get('rel') or '').lower().split())
                if not relations & FETCHING_RELATIONS:
                    target = None
            if target:
                self.found.append(('resource', tag, target))
        elif tag in NAVIGATION:
            target = values.get(NAVIGATION[tag])
            if target:
                self.found.append(('navigation', tag, target))


def pages(root):
    for directory, _, names in os.walk(root):
        for name in names:
            if name.endswith('.html'):
                yield os.path.join(directory, name)


def is_network(target):
    return target.lower().startswith(NETWORK_SCHEMES)


def is_inert(target):
    return target.lower().startswith(INERT_SCHEMES)


def resolve(page, root, target):
    """Return the path a relative target names, or None if it leaves the bundle."""
    path = urllib.parse.urldefrag(target)[0]
    path = urllib.parse.urlparse(path).path
    if not path:
        return None
    path = urllib.parse.unquote(path)
    if path.startswith('/'):
        # Root-relative: correct when served, broken from a file:// URI.
        return os.path.join(root, path.lstrip('/'))
    page_dir = os.path.dirname(page)
    return os.path.normpath(os.path.join(page_dir, path))


def stylesheets(root):
    for directory, _, names in os.walk(root):
        for name in names:
            if name.endswith('.css'):
                yield os.path.join(directory, name)


def check_stylesheets(root):
    """Every address a stylesheet resolves for itself, which the markup never shows."""
    faults = []
    checked = 0
    for sheet in sorted(stylesheets(root)):
        where = os.path.relpath(sheet, root)
        with open(sheet, encoding='utf-8') as handle:
            text = CSS_COMMENT.sub(' ', handle.read())
        for match in CSS_TARGET.finditer(text):
            target = match.group(1) or match.group(2)
            if not target:
                continue
            checked += 1
            if is_network(target):
                faults.append(f'{where}: fetches {target} from outside the bundle')
            elif is_inert(target):
                continue
            elif not os.path.exists(resolve(sheet, root, target)):
                faults.append(f'{where}: refers to {target}, which is not in the bundle')
    return faults, checked


def check(root):
    faults = []
    checked = 0
    for page in sorted(pages(root)):
        parser = Links()
        with open(page, encoding='utf-8') as handle:
            parser.feed(handle.read())
        where = os.path.relpath(page, root)
        for kind, tag, target in parser.found:
            if is_network(target):
                if kind == 'resource':
                    faults.append(f'{where}: <{tag}> fetches {target} from outside the bundle')
                continue
            if is_inert(target):
                continue
            resolved = resolve(page, root, target)
            checked += 1
            if resolved is None:
                continue
            if target.startswith('/'):
                faults.append(f'{where}: <{tag}> uses the root-relative path {target}, '
                              'which resolves only when the bundle is served')
            elif not os.path.exists(resolved):
                faults.append(f'{where}: <{tag}> points at {target}, which is not in the bundle')
    return faults, checked


def check_manifest(root, expected_version):
    manifest = os.path.join(root, 'MANIFEST.txt')
    if not os.path.exists(manifest):
        return ['MANIFEST.txt is missing from the bundle root']
    with open(manifest, encoding='utf-8') as handle:
        text = handle.read()
    # Matched on the field rather than an exact line, because the manifest
    # column-aligns its values and the alignment is not the contract.
    recorded = re.search(r'^Version:\s+(\S+)$', text, re.MULTILINE)
    if recorded is None:
        return ['MANIFEST.txt records no Version field']
    if recorded.group(1) != expected_version:
        return [f'MANIFEST.txt records version {recorded.group(1)}, '
                f'but this tree is at {expected_version}']
    return []


def main(argv):
    if len(argv) != 2:
        print(__doc__.strip().splitlines()[-1], file=sys.stderr)
        return 2
    root = argv[1]
    if not os.path.isdir(root):
        print(f'no such bundle directory: {root}', file=sys.stderr)
        return 2

    repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    import json
    with open(os.path.join(repo, '.release-please-manifest.json'), encoding='utf-8') as handle:
        version = json.load(handle)['.']

    faults, checked = check(root)
    css_faults, css_checked = check_stylesheets(root)
    faults += css_faults
    checked += css_checked
    faults += check_manifest(root, version)

    for fault in faults:
        print(fault)
    if faults:
        print(f'\n{len(faults)} fault(s): the bundle does not work offline')
        return 1
    print(f'the bundle is self-contained: {checked} links and resources, '
          f'every one of them inside it, and MANIFEST.txt records {version}')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
