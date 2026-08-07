"""MkDocs build hook: the doorways between a platform and its API reference.

Three links, none of them hand-written:

* every generated API page for a header under ``Platform/<Token>/`` gets a chip
  naming the platform it belongs to — the question a reader arriving from a
  search result has, and one the API reference could not previously answer;
* every platform page gets a chip to its Doxygen group, the generated reference
  for everything that platform declares;
* and one to its setup guide, where the platform has one.

They are injected rather than written because they are navigation furniture, not
content: generating them keeps the Markdown clean, keeps the links inside
MkDocs' own resolution and validation (raw HTML in a page is neither rewritten
nor checked), and means adding a platform needs no markup at all.

The platform list is read from ``SOLIDSYSLOG_PLATFORM_REGISTRY`` in the
top-level CMakeLists.txt — the same table ``scripts/check_manifest.py`` treats
as authoritative — so a new platform is picked up by being registered. Its slug
is the registry token lowercased, which is what the docs folder and the Doxygen
group are both named after, and its display name is the platform page's own
heading, so the name is written once.
"""

import os
import re

REGISTRY = re.compile(r"set\(SOLIDSYSLOG_PLATFORM_REGISTRY(.*?)^\)", re.DOTALL | re.MULTILINE)
ROW = re.compile(r'"([^"|]+)\|[^"|]*\|[^"|]*\|[^"|]*\|([^"|]+)\|[^"]*"')

GENERATED_PREFIX = "api/"
PLATFORM_PREFIX = "platforms/"

_CACHE = {}


def _label(root, slug):
    """The platform's own H1 — so the display name is written once, on its page."""
    path = os.path.join(root, "docs", PLATFORM_PREFIX, slug, "index.md")
    with open(path, encoding="utf-8") as page:
        for line in page:
            if line.startswith("# "):
                return line[2:].strip()
    return slug


def _index(config):
    """Return (headers, slugs): header stem -> (label, slug), and the slug set."""
    root = os.path.dirname(config["config_file_path"])
    if root not in _CACHE:
        with open(os.path.join(root, "CMakeLists.txt"), encoding="utf-8") as cmake:
            registry = REGISTRY.search(cmake.read())
        headers, slugs = {}, {}
        for token, directory in ROW.findall(registry.group(1)) if registry else []:
            interface = os.path.join(root, directory, "Interface")
            if not os.path.isdir(interface):
                continue
            slug = token.lower()
            pack = (_label(root, slug), slug)
            slugs[slug] = os.path.isfile(os.path.join(root, "docs", PLATFORM_PREFIX, slug, "setup.md"))
            for name in os.listdir(interface):
                if name.endswith(".h"):
                    headers[name[: -len(".h")]] = pack
        _CACHE[root] = (headers, slugs)
    return _CACHE[root]


def _stem(src_uri):
    """api/SolidSyslogMbedTlsStream_8h.md -> SolidSyslogMbedTlsStream."""
    leaf = src_uri[len(GENERATED_PREFIX) : -len(".md")]
    return leaf[: -len("_8h")] if leaf.endswith("_8h") else None


def on_page_markdown(markdown, page, config, files, **kwargs):
    src_uri = getattr(page.file, "src_uri", None) or page.file.src_path.replace(os.sep, "/")
    headers, slugs = _index(config)

    if src_uri.startswith(GENERATED_PREFIX):
        stem = _stem(src_uri)
        pack = headers.get(stem) if stem else None
        if pack is None:
            return markdown
        label, slug = pack
        chip = f"[{label} platform](../{PLATFORM_PREFIX}{slug}/index.md){{ .ss-platform-chip }}"
        return f"{chip}\n\n{markdown}"

    if src_uri.startswith(PLATFORM_PREFIX) and src_uri.endswith("/index.md"):
        slug = src_uri[len(PLATFORM_PREFIX) : -len("/index.md")]
        if slug not in slugs:
            return markdown
        chips = [f"[API reference](../../api/group__platform__{slug}.md){{ .ss-chip .ss-chip--api }}"]
        if slugs[slug]:
            chips.append("[Setup](setup.md){ .ss-chip .ss-chip--setup }")
        # One line, so the inline-flex chips sit side by side; placed under the
        # title rather than above it, so the page still opens with its name.
        title, _, body = markdown.partition("\n")
        return f"{title}\n\n{' '.join(chips)}\n\n{body.lstrip()}"

    return markdown
