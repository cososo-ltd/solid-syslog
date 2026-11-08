"""MkDocs build hook: dress a generated report in the site's header and footer.

The coverage report under ``testing/coverage-report/`` is genhtml's output,
copied into the site untouched. genhtml can paste a static prolog and epilog
onto every page, and nothing more: the prolog cannot know the year, the build
the site came from, or how the rest of the site looks. Those are all rendered
into the Coverage page by the time this hook runs, so it takes them from there.

After the build, for each report this hook is configured with, it reads the
page that introduces the report, lifts its stylesheet links, its ``<body>``
attributes, the rendered ``<header>``, the tabs and the ``<footer>``, and
splices them into every page of the report between the markers the prolog and
epilog leave (``Tests/Coverage/prolog.html``). The header loses what needs the
site's scripts - the search box, the drawer and palette controls - and its
page-title line becomes a link back to the introducing page; the footer's generator
credit names the report's tool instead of MkDocs. Every relative link in the
fragments is re-based from the introducing page to the report page it lands
on, so the tabs, the logo and the footer mark resolve at every depth, in the
served site and in the offline bundle alike.

A build without the report in hand - every local one - has nothing to dress
and does nothing. A report whose introducing page is missing is a
misconfiguration and fails the build, since the report would otherwise publish
in genhtml's own clothes without anyone noticing.
"""

import os
import posixpath
import re

from mkdocs.exceptions import PluginError

# (page that introduces the report, directory holding the report, credit for
# the tool that produced it), all relative to the docs root and the site root.
REPORTS = [
    (
        "testing/coverage",
        "testing/coverage-report",
        'Reported by <a href="https://github.com/linux-test-project/lcov" target="_blank" rel="noopener">LCOV</a>',
    ),
]

STYLES_MARKER = "<!-- site-styles -->"
HEADER = ("<!-- site-header -->", "<!-- /site-header -->")
FOOTER = ("<!-- site-footer -->", "<!-- /site-footer -->")

STYLESHEET = re.compile(r'<link rel="stylesheet" href="[^"]*"[^>]*>')
# The site's own scripts, under javascripts/, such as the one that shrinks the
# logo on scroll. Material's bundle under assets/javascripts/ is not lifted: it
# drives components the report pages do not have.
SITE_SCRIPT = re.compile(r'<script src="[^"]*/javascripts/[^"]*"></script>')
MATERIAL_BUNDLE = "/assets/javascripts/"
BODY_TAG = re.compile(r"<body[^>]*>")
HEADER_TAG = re.compile(r'<header class="md-header".*?</header>', re.DOTALL)
TABS_TAG = re.compile(r'<nav class="md-tabs".*?</nav>', re.DOTALL)
FOOTER_TAG = re.compile(r'<footer class="md-footer">.*?</footer>', re.DOTALL)
SCRIPT = re.compile(r"<script>.*?</script>", re.DOTALL)
PALETTE = re.compile(r'<form class="md-header__option".*?</form>', re.DOTALL)
BUTTON = re.compile(r'<label class="md-header__button md-icon" for="__(?:drawer|search)">.*?</label>', re.DOTALL)
TOPIC = re.compile(
    r'(<div class="md-header__topic" data-md-component="header-topic">\s*<span class="md-ellipsis">\s*)(.*?)(\s*</span>)',
    re.DOTALL,
)
GENERATOR = re.compile(r'Made with\s*<a href="https://squidfunk.github.io/mkdocs-material/"[^>]*>\s*Material for MkDocs\s*</a>')
URL_ATTRIBUTE = re.compile(r'(href|src)="([^"]*)"')
EXTERNAL = ("http://", "https://", "//", "mailto:", "tel:", "data:", "#", "/")


def introducing_page(site_dir, page, directory_urls):
    """Where MkDocs wrote the page, the directory its links are relative to,
    and its address from the site root."""
    if directory_urls:
        return os.path.join(site_dir, page, "index.html"), page + "/", page + "/"
    return os.path.join(site_dir, page + ".html"), posixpath.dirname(page) + "/", page + ".html"


def relocate(fragment, from_dir, to_dir):
    """Re-base every relative URL in `fragment` from one directory to another,
    both given as site-root-relative paths ending in a slash."""

    def rebase(match):
        attribute, url = match.groups()
        if url.startswith(EXTERNAL) or not url:
            return match.group(0)
        path, separator, tail = _split_tail(url)
        target = posixpath.normpath(posixpath.join(from_dir, path))
        target = "" if target == "." else target
        trailing = "/" if path.endswith("/") or target == "" else ""
        rebased = posixpath.relpath(target or ".", to_dir.rstrip("/") or ".")
        rebased = "" if rebased == "." else rebased
        return f'{attribute}="{rebased}{trailing}{separator}{tail}"'

    return URL_ATTRIBUTE.sub(rebase, fragment)


def _split_tail(url):
    """A URL's path, and the query or fragment that follows it."""
    for i, char in enumerate(url):
        if char in "?#":
            return url[:i], url[i], url[i + 1 :]
    return url, "", ""


def strip_controls(header):
    """The header without what needs the site's scripts to work."""
    header = SCRIPT.sub("", header)
    header = PALETTE.sub("", header)
    header = BUTTON.sub("", header)
    return _without_element(header, '<div class="md-search"')


def _without_element(html, start_tag):
    """`html` with the element opening at `start_tag` removed, nested
    elements of the same name included."""
    start = html.find(start_tag)
    if start < 0:
        return html
    name = start_tag[1:].split(" ", 1)[0]
    depth = 0
    for match in re.finditer(rf"<{name}\b|</{name}>", html[start:]):
        depth += 1 if match.group(0).startswith(f"<{name}") else -1
        if depth == 0:
            return html[:start] + html[start + match.end() :]
    return html


def link_topic(header, href):
    """The header's page-title line, as a link to `href`, and shown.

    Material shows the site name until the reader scrolls past the page
    heading, then slides the page title in; the swap is the site's script
    marking the title active. The report pages carry no script, so the title
    is marked active here and the header reads logo, then the linked title.
    """
    header = header.replace('class="md-header__title"', 'class="md-header__title md-header__title--active"', 1)
    return TOPIC.sub(lambda m: f'{m.group(1)}<a href="{href}">{m.group(2).strip()}</a>{m.group(3)}', header, count=1)


def credit(footer, text):
    return GENERATOR.sub(text, footer)


class Chrome:
    """What one introducing page lends to the report pages beneath it."""

    def __init__(self, html, from_dir, url, generator):
        self.from_dir = from_dir
        self.url = url
        self.styles = "\n".join(STYLESHEET.findall(html))
        self.scripts = "\n".join(tag for tag in SITE_SCRIPT.findall(html) if MATERIAL_BUNDLE not in tag)
        self.body = _first(BODY_TAG, html, "body tag")
        self.header = strip_controls(_first(HEADER_TAG, html, "header"))
        self.tabs = _first(TABS_TAG, html, "tabs")
        self.footer = credit(_first(FOOTER_TAG, html, "footer"), generator)

    def dress(self, page, page_dir):
        """`page` with the fragments spliced in, every link re-based to `page_dir`."""
        # The title leads back to the introducing page, one level above the
        # report's own root. It is written for this page, so it is added after
        # the fragments lifted from the introducing page have been re-based.
        back = posixpath.relpath(self.url, page_dir.rstrip("/")) + ("/" if self.url.endswith("/") else "")
        header = link_topic(relocate(self.header + self.tabs, self.from_dir, page_dir), back)
        footer = relocate(self.footer + "\n" + self.scripts, self.from_dir, page_dir)
        page = page.replace(STYLES_MARKER, relocate(self.styles, self.from_dir, page_dir), 1)
        page = BODY_TAG.sub(self.body, page, count=1)
        page = _replace_between(page, HEADER, header)
        return _replace_between(page, FOOTER, footer)


def _first(pattern, html, what):
    match = pattern.search(html)
    if not match:
        raise PluginError(f"hooks/report_chrome.py: the introducing page has no {what} to lend")
    return match.group(0)


def _replace_between(html, markers, replacement):
    start, end = markers
    opening = html.find(start)
    closing = html.find(end, opening)
    if opening < 0 or closing < 0:
        return html
    return html[:opening] + replacement + html[closing + len(end) :]


def dress_report(site_dir, page, report, generator, directory_urls):
    report_path = os.path.join(site_dir, report)
    if not os.path.isdir(report_path):
        return 0
    template, from_dir, url = introducing_page(site_dir, page, directory_urls)
    if not os.path.exists(template):
        raise PluginError(f"hooks/report_chrome.py: {report} is in the site but {page} is not, so nothing can dress it")
    with open(template, encoding="utf-8") as handle:
        chrome = Chrome(handle.read(), from_dir, url, generator)
    dressed = 0
    for root, _, names in os.walk(report_path):
        for name in names:
            if not name.endswith(".html"):
                continue
            path = os.path.join(root, name)
            page_dir = os.path.relpath(root, site_dir).replace(os.sep, "/") + "/"
            with open(path, encoding="utf-8") as handle:
                html = handle.read()
            with open(path, "w", encoding="utf-8") as handle:
                handle.write(chrome.dress(html, page_dir))
            dressed += 1
    return dressed


def on_post_build(config, **kwargs):
    for page, report, generator in REPORTS:
        dress_report(config["site_dir"], page, report, generator, config["use_directory_urls"])
