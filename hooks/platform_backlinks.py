"""MkDocs build hook: link every generated platform API page to its platform.

The API reference answers "what does this header declare". It never answered
"which platform pack is this, and what does that pack need from my build" —
which is the question a reader arriving from a search result actually has.

Every generated page for a header under ``Platform/<Token>/`` gets one line at
the top linking to that pack's hand-written page. The platform's own page links
the other way, to the pack's Doxygen group, so the pair closes the loop.

The platform list is read from ``SOLIDSYSLOG_PLATFORM_REGISTRY`` in the
top-level CMakeLists.txt — the same table ``scripts/check_manifest.py`` treats
as authoritative — so a new pack is picked up by being registered, with no edit
here. The slug is the registry token lowercased, which is the convention the
docs folder and the Doxygen group both follow.
"""

import os
import re

REGISTRY = re.compile(r"set\(SOLIDSYSLOG_PLATFORM_REGISTRY(.*?)^\)", re.DOTALL | re.MULTILINE)
ROW = re.compile(r'"([^"|]+)\|[^"|]*\|[^"|]*\|[^"|]*\|([^"|]+)\|[^"]*"')

# Generated page stems are the header path with non-alphanumerics escaped;
# the leaf name is enough to identify the header, since every public header in
# the tree is uniquely named (docs/NAMING.md).
GENERATED_PREFIX = "api/"

_CACHE = {}


def _label(root, slug):
    """The platform's own H1 — so the name is written once, on its page."""
    path = os.path.join(root, "docs", "platforms", slug, "index.md")
    with open(path, encoding="utf-8") as page:
        for line in page:
            if line.startswith("# "):
                return line[2:].strip()
    return slug


def _packs(config):
    """Return {header stem: (label, slug)} for every public platform header."""
    root = os.path.dirname(config["config_file_path"])
    if root not in _CACHE:
        with open(os.path.join(root, "CMakeLists.txt"), encoding="utf-8") as cmake:
            registry = REGISTRY.search(cmake.read())
        headers = {}
        for token, directory in ROW.findall(registry.group(1)) if registry else []:
            interface = os.path.join(root, directory, "Interface")
            if not os.path.isdir(interface):
                continue
            pack = (_label(root, token.lower()), token.lower())
            for name in os.listdir(interface):
                if name.endswith(".h"):
                    headers[name[: -len(".h")]] = pack
        _CACHE[root] = headers
    return _CACHE[root]


def _stem(src_uri):
    """api/SolidSyslogMbedTlsStream_8h.md -> SolidSyslogMbedTlsStream."""
    leaf = src_uri[len(GENERATED_PREFIX) : -len(".md")]
    return leaf[: -len("_8h")] if leaf.endswith("_8h") else None


def on_page_markdown(markdown, page, config, files, **kwargs):
    src_uri = getattr(page.file, "src_uri", None) or page.file.src_path.replace(os.sep, "/")
    if not src_uri.startswith(GENERATED_PREFIX):
        return markdown
    stem = _stem(src_uri)
    pack = _packs(config).get(stem) if stem else None
    if pack is None:
        return markdown
    label, slug = pack
    banner = (
        f"!!! info \"Part of the [{label}](../platforms/{slug}/index.md) platform\"\n"
        f"    What the pack ships, what your build must provide, and the\n"
        f"    obligations it leaves to you.\n\n"
    )
    return banner + markdown
