"""MkDocs build hook: hide the table of contents on mkdoxy's member indexes.

mkdoxy buckets a member index by each symbol's initial letter. Every public
function is ``SolidSyslog*`` and every public macro ``SOLIDSYSLOG_*``, so those
two indexes collapse to a single bucket and their table of contents renders a
lone entry — furniture implying a navigation it cannot offer.

Only those two are affected: the variable and class-member indexes bucket on
unprefixed struct-member names and stay genuinely multi-letter, and the Headers
and Data-structures indexes emit no headings to index at all.
"""

import os

# Generated pages whose letter-bucket index collapses to a single entry.
NO_TOC_PAGES = frozenset(("api/functions.md", "api/macros.md"))


def on_page_markdown(markdown, page, config, files, **kwargs):
    src = getattr(page.file, "src_uri", None) or page.file.src_path.replace(os.sep, "/")
    if src in NO_TOC_PAGES:
        # Material reads page.meta["hide"] when it renders the secondary sidebar,
        # which happens after this hook, so setting it here is honoured.
        hidden = list(page.meta.get("hide", []))
        if "toc" not in hidden:
            hidden.append("toc")
        page.meta["hide"] = hidden
    return markdown
