"""MkDocs build hook: drop mkdoxy's member summary tables.

Every generated API page lists each member twice. A summary table near the top
gives the member's type and the first sentence of its comment, and a
Documentation section below repeats the name with the full signature, the whole
comment, and the anchor the table of contents already points at. The summary is
the weaker of the two everywhere it appears, and for a vtable it is actively
wrong: Doxygen's XML splits a function-pointer member so that its ``type`` is
the literal text ``void(*`` and the parameter list lives in ``argsstring``,
which the summary template drops. Every role's vtable therefore rendered its
members as ``void(*``, ``bool(*``, ``size_t(*``.

Repairing that cell would have kept a table whose every row duplicates the
section beneath it, so the tables go instead. Checked before removing, across
the 89 generated pages: 247 summary sections, and not one entry that the
matching Documentation section did not also carry — so no page loses a member,
a signature or a description.

Only summaries with a Documentation counterpart are dropped. ``Classes``,
``Files``, ``Directories`` and ``Detailed Description`` have none: they are the
page's own content rather than a preview of it, and they stay.
"""

import os
import re
import textwrap

# The page's own brief, and the link to a section that now sits directly below
# it. Doxygen splits a comment's first sentence into the brief and keeps the
# rest for the detailed description, so the brief is moved into that section
# rather than dropped — deleting it would lose the sentence.
BRIEF = re.compile(r"^_(?P<brief>.+?)_[ \t]*(?:\[\*\*More\.\.\.\*\*\]|\[More\.\.\.\])\([^)]*\)[ \t]*$", re.MULTILINE)
DETAIL_HEADING = re.compile(r"^## Detailed Description[ \t]*$", re.MULTILINE)
# The header a generated page was built from, as the page itself states it.
INCLUDE = re.compile(r"^\*[ \t]*`#include <(?P<header>[\w.]+)>`[ \t]*$", re.MULTILINE)
STRUCT_TITLE = re.compile(r"^# Struct (?P<name>\w+)[ \t]*$", re.MULTILINE)
# Three or more blank lines, which mkdoxy emits freely and removing a section
# leaves more of.
BLANK_RUN = re.compile(r"\n{4,}")

# Where a header named by an #include line can live. Doxygen's input is the
# public surface, so these two trees are the whole search space.
HEADER_ROOTS = ("Core/Interface", "Platform")

# Summary heading -> the Documentation heading that carries the same members.
# A summary is only ever dropped when its counterpart is present on the page,
# so a future mkdoxy template that stops emitting one leaves the other intact.
SUMMARY_SECTIONS = {
    "Public Functions": "Public Functions Documentation",
    "Public Attributes": "Public Attributes Documentation",
    "Public Types": "Public Types Documentation",
    "Macros": "Macro Definition Documentation",
}

GENERATED_PREFIX = "api/"


def _section_span(markdown, heading):
    """The (start, end) of a ``## heading`` section, or None when absent.

    The span runs to the next ``##`` heading of any level-2 kind, or to the end
    of the page for the last section.
    """
    match = re.search(r"^##[ \t]+" + re.escape(heading) + r"[ \t]*$", markdown, re.MULTILINE)
    if match is None:
        return None
    following = re.search(r"^## ", markdown[match.end() :], re.MULTILINE)
    end = len(markdown) if following is None else match.end() + following.start()
    return (match.start(), end)


def _drop_summaries(markdown):
    """Remove each summary section whose Documentation counterpart is present."""
    for summary, documentation in SUMMARY_SECTIONS.items():
        if _section_span(markdown, documentation) is None:
            continue
        span = _section_span(markdown, summary)
        if span is not None:
            markdown = markdown[: span[0]] + markdown[span[1] :]
    return markdown


def _merge_brief(markdown):
    """Fold the brief into Detailed Description, so one block describes the page.

    Left alone when there is no such section: the brief is then the only
    description the page has.
    """
    brief = BRIEF.search(markdown)
    heading = DETAIL_HEADING.search(markdown)
    if brief is None or heading is None:
        return markdown
    sentence = brief.group("brief").strip()
    markdown = markdown[: brief.start()] + markdown[brief.end() :]
    heading = DETAIL_HEADING.search(markdown)
    return markdown[: heading.end()] + "\n\n" + sentence + markdown[heading.end() :]


def _strip_comments(source):
    """The declaration without its comments — the shape, not the prose.

    Doxygen already renders each member's comment below; repeating them inside
    the code block would make it a wall rather than a summary of the shape.

    Dedented too: every public struct sits inside SOLIDSYSLOG_EXTERN_C_BEGIN, so
    the header's indentation is relative to a wrapper the block does not show.
    """
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    lines = [line.rstrip() for line in source.splitlines() if line.strip()]
    return textwrap.dedent("\n".join(lines))


def _find_header(root, name):
    for base in HEADER_ROOTS:
        for directory, _dirs, files in os.walk(os.path.join(root, base)):
            if name in files:
                return os.path.join(directory, name)
    return None


def _declaration(root, header, name):
    """The `struct <name> { ... };` block from the header, comments removed."""
    path = _find_header(root, header)
    if path is None:
        return None
    with open(path, encoding="utf-8") as handle:
        source = handle.read()
    start = re.search(r"struct[ \t]+" + re.escape(name) + r"\b[^;{]*\{", source)
    if start is None:
        return None
    # From the start of that line, not from the keyword: these structs are
    # indented inside SOLIDSYSLOG_EXTERN_C_BEGIN, and dedenting needs the first
    # line to carry the same indentation as the rest.
    line_start = source.rfind("\n", 0, start.start()) + 1
    depth = 0
    for index in range(start.end() - 1, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return _strip_comments(source[line_start : index + 1] + ";")
    return None


def _inject_declaration(markdown, root):
    """Put the struct's own declaration under the #include line."""
    title = STRUCT_TITLE.search(markdown)
    include = INCLUDE.search(markdown)
    if title is None or include is None:
        return markdown
    declaration = _declaration(root, include.group("header"), title.group("name"))
    if declaration is None:
        return markdown
    block = "\n\n```c\n" + declaration + "\n```"
    return markdown[: include.end()] + block + markdown[include.end() :]


def on_page_markdown(markdown, page, config, files, **kwargs):
    src = getattr(page.file, "src_uri", None) or page.file.src_path.replace(os.sep, "/")
    if not src.startswith(GENERATED_PREFIX):
        return markdown
    markdown = _drop_summaries(markdown)
    markdown = _merge_brief(markdown)
    root = os.path.dirname(config["config_file_path"]) if config else "."
    markdown = _inject_declaration(markdown, root)
    return BLANK_RUN.sub("\n\n\n", markdown)
