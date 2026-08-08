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
MODULES_CRUMB = re.compile(r"\[\*\*Modules\*\*\]\([^)]*\)")
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
CANONICAL = "https://docs.cososo.co.uk/solid-syslog/"

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


def _group_slug(src_uri):
    """api/group__platform__atomics.md -> atomics, else None."""
    leaf = src_uri[len(GENERATED_PREFIX) : -len(".md")]
    prefix = "group__platform__"
    return leaf[len(prefix) :] if leaf.startswith(prefix) else None


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


def _relativise(markdown, slug, label):
    """Point the group's own back-link at this build, not at the live site.

    The .dox block carries the canonical docs URL so that someone reading the
    source tree can find the page. Doxygen auto-links it, which on any build
    that is not production — a local preview, a pull-request artefact — sends
    the reader to the published site instead of the one in front of them.
    """
    url = re.escape(f"{CANONICAL}{PLATFORM_PREFIX}{slug}/")
    linked = re.compile(rf"\[{url}\]\({url}\)|{url}")
    return linked.sub(f"[{label}](../{PLATFORM_PREFIX}{slug}/index.md)", markdown)


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


def on_page_markdown(markdown, page, config, files, **kwargs):
    src_uri = getattr(page.file, "src_uri", None) or page.file.src_path.replace(os.sep, "/")
    headers, slugs, labels, manifest = _index(config)

    if src_uri.startswith(GENERATED_PREFIX):
        # A group page arrives titled in Doxygen's vocabulary — "Group
        # platform_atomics", under a breadcrumb reading "Modules" — where the
        # reader has been shown "C11 atomics" everywhere else. Group and Module
        # are terms this documentation does not otherwise use, and the group's
        # own @defgroup title is ignored by the template, so rewrite all three
        # to the platform page's heading. The doubled underscores of the refid
        # (group__platform__atomics) do not contain the single-underscore group
        # name, so the bare-name replacement cannot damage a link target.
        group = _group_slug(src_uri)
        if group is not None and group in slugs:
            title = f"{labels[group]} platform"
            # mkdoxy backslash-escapes the underscore so Markdown does not read
            # it as emphasis, so the name appears as platform\_atomics.
            name = r"platform\\?_" + re.escape(group)
            markdown = re.sub(rf"#\s+Group\s+{name}", f"# {title}", markdown, count=1)
            markdown = MODULES_CRUMB.sub(f"[**Platforms**](../{PLATFORM_PREFIX}index.md)", markdown, count=1)
            markdown = _relativise(markdown, group, labels[group])
            return re.sub(name, title, markdown)

        if src_uri == f"{GENERATED_PREFIX}modules.md":
            return markdown.replace("# Modules", "# Platform API reference", 1)

        stem = _stem(src_uri)
        pack = headers.get(stem) if stem else None
        if pack is None:
            return markdown
        label, slug = pack
        chip = f"[{label} platform](../{PLATFORM_PREFIX}{slug}/index.md){{ .ss-chip .ss-chip--platform }}"
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
        markdown = f"{title}\n\n{' '.join(chips)}\n\n{body.lstrip()}"
        return SHIPS.sub(lambda m: f"{m.group(1)}\n{_ships(manifest[slug])}\n\n", markdown, count=1)

    return markdown
