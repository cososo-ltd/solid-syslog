"""MkDocs build hook: publish repository-root Markdown as pages of the site.

A few documents have to live at the repository root — GitHub reads SECURITY.md
to drive its vulnerability-reporting UI, and a licence is looked for beside the
code — but they are documentation the site should hold rather than send a reader
away for. Publishing them here keeps one source with two renderings instead of a
root copy and a docs copy drifting apart.

The file is read at build time and handed to MkDocs as generated content, so
nothing is copied into the working tree and nothing needs a symlink (which the
Windows side would not thank us for).

Their links are written to resolve from the repository root, where `docs/…` is
how they reach a documentation page. Inside the site that prefix is wrong, so it
is stripped and the remainder made relative to wherever the page is published.
`mkdocs build --strict` fails on any that does not land.
"""

import os
import re

# Root file → where it is published. Both halves matter: the source path is
# fixed by convention (GitHub looks for these names, in this place), and the
# destination is ours to choose so the page sits with its siblings.
PUBLISHED = {
    "SECURITY.md": "security/policy.md",
    "LICENSE.md": "license.md",
    "SUPPORT.md": "support.md",
    # LICENSE.md chooses between three licences and links the two verbatim
    # texts beside it. Publishing them keeps those links live on the site
    # instead of dead-ending a reader deciding whether they may use the
    # library — which is the question the page exists to answer.
    "LICENSES/PolyForm-Internal-Use-1.0.0.md": "licenses/polyform-internal-use-1.0.0.md",
    "LICENSES/PolyForm-Noncommercial-1.0.0.md": "licenses/polyform-noncommercial-1.0.0.md",
}

# The site's landing page, which is what `docs/` means to a reader at the root.
HOME = "README.md"
# Any link in a root document. Whether it needs re-aiming depends on where it
# points, which _target decides.
LINK = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")


def _target(href):
    """Where this link lands inside the site, or None if it is not ours to move.

    Two kinds re-aim: a path into the documentation tree — `docs/` alone being
    the landing page — and a sibling root document that is also published. An
    external URL is already right.
    """
    if href.startswith("docs/"):
        return href[len("docs/") :] or HOME
    return PUBLISHED.get(href)


def _relink(markdown, published_at, root):
    """Re-aim a root file's links at this page's position in the site.

    A link to a root document we do not publish becomes plain text rather than
    a jump out to the repository host — the same treatment the documentation
    gives a source file it cannot render. Publishing the target instead would
    cascade: CONTRIBUTING.md reaches CLAUDE.md, which is a working agreement
    between maintainers and not part of the product's documentation.
    """
    page_dir = os.path.dirname(published_at)

    def rewrite(match):
        text, href = match.group(1), match.group(2)
        href, _, fragment = href.partition("#")
        target = _target(href)
        if target is None:
            unpublished = "/" not in href and os.path.isfile(os.path.join(root, href))
            return text if unpublished else match.group(0)
        relative = os.path.relpath(target, page_dir) if page_dir else target
        return f"[{text}]({relative}{'#' + fragment if fragment else ''})"

    return LINK.sub(rewrite, markdown)


def on_files(files, config):
    # Imported here rather than at module scope so PUBLISHED can be read without
    # MkDocs installed -- hooks/source_links.py consumes it, and both hooks stay
    # exercisable outside the docs image.
    from mkdocs.structure.files import File

    root = os.path.dirname(config["config_file_path"])
    for source, destination in PUBLISHED.items():
        with open(os.path.join(root, source), encoding="utf-8") as handle:
            markdown = handle.read()
        content = _relink(markdown, destination, root)
        files.append(File.generated(config, destination, content=content))
    return files
