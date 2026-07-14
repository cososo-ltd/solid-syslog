"""MkDocs build hook: inject generated relationship diagrams onto API pages.

For every vtable role, the graph is extracted from the headers
(``relationship_data``) and a post-it "realised by" diagram is rendered
(``relationship_render``) and prepended, inline, onto the role's base API page —
its consumer dispatch header where there is one, else the vtable Definition.

The SVG is inline (not a Markdown image or <object>) so its per-backend links
stay clickable; its styles are id-scoped so nothing leaks into the page. Nothing
is committed or written to disk — the diagrams are built from code each run.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import relationship_data  # noqa: E402
import relationship_render  # noqa: E402

_CACHE = {}


def _by_base_page(config):
    """Return {base_page_stem: (role, info)} for this repo, built once."""
    root = os.path.dirname(config["config_file_path"])
    if root not in _CACHE:
        graph = relationship_data.build_graph(root)
        _CACHE[root] = {info["base_page"]: (role, info) for role, info in graph.items()}
    return _CACHE[root]


def on_page_markdown(markdown, page, config, files, **kwargs):
    src = getattr(page.file, "src_uri", None) or page.file.src_path.replace(os.sep, "/")
    if not (src.startswith("api/") and src.endswith("_8h.md")):
        return markdown
    stem = src[len("api/"):-len(".md")]
    match = _by_base_page(config).get(stem)
    if match is None:
        return markdown
    role, info = match
    return relationship_render.render_svg(role, info) + "\n\n" + markdown
