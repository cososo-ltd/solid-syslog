"""MkDocs build hook: a platform's page, its manifest, and the way back to it.

Three jobs, none of them hand-written:

* every generated API page for a header under ``Platform/<Token>/`` gets a chip
  naming the platform it belongs to — the question a reader arriving from a
  search result has, and one the API reference could not previously answer;
* a platform page gets a chip to its setup guide and its setup guide one back,
  and its *What it ships* section is filled with the headers it publishes;
* and the Doxygen group pages are dropped from the build.

The groups are dropped because a platform had two pages answering "what is this
platform", and the group page's only unique content was its Files table — which
is now the manifest, on the platform page, where the prose is hand-editable, the
links are validated, and the URL is the good one. ``@defgroup`` and ``@ingroup``
stay in the source: they cost nothing, and an integrator running Doxygen over
the headers they ship with still gets the grouping.

Chips are injected rather than written because they are navigation furniture,
not content: generating them keeps the Markdown clean, keeps the links inside
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
# The @file block down to its first blank comment line, and the first sentence
# of it — Doxygen's own implicit brief, so the page and the generated reference
# say the same thing because they read the same source.
FILE_BLOCK = re.compile(r"/\*\*\s*@file\s*\n(.*?)(?:\n[ \t]*\*[ \t]*\n|\*/)", re.DOTALL)
SENTENCE_END = re.compile(r"(?<=[a-z0-9)\]])\.(?:\s|$)")
# The section the manifest replaces. Everything to the next heading is
# generated, so the page source carries the heading alone.
SHIPS = re.compile(r"^(## What it ships[ \t]*\n)(.*?)(?=^## |\Z)", re.MULTILINE | re.DOTALL)
# A definition, not a forward declaration — the brace is what distinguishes
# `struct SolidSyslogMbedTlsStreamConfig {` from `struct SolidSyslogStream;`.
STRUCT_DEF = re.compile(r"struct\s+(SolidSyslog\w+)\s*\{", re.MULTILINE)

GENERATED_PREFIX = "api/"
PLATFORM_PREFIX = "platforms/"
# The group pages and the index that lists them. Dropped from the build: their
# content lives on the platform pages now, and leaving them would publish two
# answers to the same question.
DROPPED = re.compile(r"^api/(group__platform__\w+|modules)\.md$")
# mkdoxy's index-of-indexes lists Modules among them, so dropping the page
# without dropping the entry leaves a dead link the strict build rejects.
MODULES_ENTRY = re.compile(r"^[ \t]*-[ \t]*\[Modules\]\(modules\.md\)[ \t]*\n", re.MULTILINE)

_CACHE = {}


def _label(root, slug):
    """The platform's own H1 — so the display name is written once, on its page."""
    path = os.path.join(root, "docs", PLATFORM_PREFIX, slug, "index.md")
    with open(path, encoding="utf-8") as page:
        for line in page:
            if line.startswith("# "):
                return line[2:].strip()
    return slug


def _brief(text):
    """A header's implicit brief — the first sentence of its @file block."""
    block = FILE_BLOCK.search(text)
    if block is None:
        return ""
    body = " ".join(line.strip().lstrip("*").strip() for line in block.group(1).splitlines())
    split = SENTENCE_END.search(body)
    return (body[: split.end()] if split else body).strip()


def _index(config):
    """Return (headers, slugs, labels, manifest) built from the platform registry.

    ``headers`` maps both a header stem and a struct name to its platform, so
    every generated page for a platform — file or data structure — can be
    labelled. Core's structs are absent by construction: only the platform
    Interface directories are scanned.

    ``manifest`` is the ordered (stem, brief) list per platform, which becomes
    the *What it ships* table. Generated rather than written, because a list of
    filenames and their own briefs is mechanical: hand-keeping one per platform
    is clerical work that drifts the moment a header is added.
    """
    root = os.path.dirname(config["config_file_path"])
    if root not in _CACHE:
        with open(os.path.join(root, "CMakeLists.txt"), encoding="utf-8") as cmake:
            registry = REGISTRY.search(cmake.read())
        headers, slugs, labels, manifest = {}, {}, {}, {}
        for token, directory in ROW.findall(registry.group(1)) if registry else []:
            interface = os.path.join(root, directory, "Interface")
            if not os.path.isdir(interface):
                continue
            slug = token.lower()
            labels[slug] = _label(root, slug)
            pack = (labels[slug], slug)
            slugs[slug] = os.path.isfile(os.path.join(root, "docs", PLATFORM_PREFIX, slug, "setup.md"))
            manifest[slug] = []
            for name in sorted(os.listdir(interface)):
                if not name.endswith(".h"):
                    continue
                stem = name[: -len(".h")]
                headers[stem] = pack
                with open(os.path.join(interface, name), encoding="utf-8") as header:
                    text = header.read()
                manifest[slug].append((stem, _brief(text)))
                for struct in STRUCT_DEF.findall(text):
                    headers[struct] = pack
        _CACHE[root] = (headers, slugs, labels, manifest)
    return _CACHE[root]


def _ships(entries):
    """The manifest table: every header the platform publishes, by filename."""
    rows = ["| Header | What it is |", "|---|---|"]
    for stem, brief in entries:
        rows.append(f"| [`{stem}.h`](../../api/{stem}_8h.md) | {brief.replace('|', r'\|')} |")
    return "\n".join(rows)


def _stem(src_uri):
    """The header or struct a generated page documents, else None.

    api/SolidSyslogMbedTlsStream_8h.md          -> SolidSyslogMbedTlsStream
    api/structSolidSyslogMbedTlsStreamConfig.md -> SolidSyslogMbedTlsStreamConfig
    """
    leaf = src_uri[len(GENERATED_PREFIX) : -len(".md")]
    if leaf.endswith("_8h"):
        return leaf[: -len("_8h")]
    if leaf.startswith("struct"):
        return leaf[len("struct") :]
    return None


def on_files(files, config):
    """Drop the group pages and the index listing them.

    mkdoxy generates a page per @defgroup whether anything links to it or not.
    Left in, they publish a second answer to "what is this platform" — and a
    worse one, since the platform page owns the prose, the good URL and the
    meta description. Removing them here rather than removing @defgroup from
    the source keeps the grouping for anyone running Doxygen themselves.
    """
    kept = [f for f in files if not DROPPED.match(f.src_uri.replace(os.sep, "/"))]
    # Rebuild the collection MkDocs handed us — it is a Files object with its
    # own API downstream, not a plain list.
    return type(files)(kept)


def on_page_markdown(markdown, page, config, files, **kwargs):
    src_uri = getattr(page.file, "src_uri", None) or page.file.src_path.replace(os.sep, "/")
    headers, slugs, labels, manifest = _index(config)

    if src_uri.startswith(GENERATED_PREFIX):
        if src_uri == f"{GENERATED_PREFIX}links.md":
            return MODULES_ENTRY.sub("", markdown, count=1)

        stem = _stem(src_uri)
        pack = headers.get(stem) if stem else None
        if pack is None:
            return markdown
        label, slug = pack
        chip = f"[{label} platform](../{PLATFORM_PREFIX}{slug}/index.md){{ .ss-chip .ss-chip--platform }}"
        return f"{chip}\n\n{markdown}"

    if src_uri.startswith(PLATFORM_PREFIX) and src_uri.endswith("/setup.md"):
        slug = src_uri[len(PLATFORM_PREFIX) : -len("/setup.md")]
        if slug not in slugs:
            return markdown
        chip = f"[{labels[slug]} platform](index.md){{ .ss-chip .ss-chip--platform }}"
        title, _, body = markdown.partition("\n")
        return f"{title}\n\n{chip}\n\n{body.lstrip()}"

    if src_uri.startswith(PLATFORM_PREFIX) and src_uri.endswith("/index.md"):
        slug = src_uri[len(PLATFORM_PREFIX) : -len("/index.md")]
        if slug not in slugs:
            return markdown
        if slugs[slug]:
            chip = "[Setup](setup.md){ .ss-chip .ss-chip--setup }"
            # Placed under the title rather than above it, so the page still
            # opens with its name.
            title, _, body = markdown.partition("\n")
            markdown = f"{title}\n\n{chip}\n\n{body.lstrip()}"
        return SHIPS.sub(lambda m: f"{m.group(1)}\n{_ships(manifest[slug])}\n\n", markdown, count=1)

    return markdown
