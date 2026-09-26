"""MkDocs build hook: inject generated relationship diagrams onto API pages.

Three diagram types, all extracted from the headers (``relationship_data``) and
rendered as inline post-it SVG (``relationship_render``), prepended to the page:

* a role's base page gets the "realised by" view — its concrete backends;
* each concrete implementer's page gets the reverse view — the role it realises
  and the roles it is wired to use;
* the SolidSyslogConfig page gets the facade view — the composition root and the
  roles it wires together.

The SVG is inline (not a Markdown image or <object>) so its per-node links stay
clickable; its styles are id-scoped so nothing leaks into the page. Nothing is
committed or written to disk — the diagrams are built from code each run.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import relationship_data  # noqa: E402
import relationship_render  # noqa: E402

_CACHE = {}


def on_config(config, **kwargs):
    """Tell the renderer how this build addresses a page.

    The diagrams' links are raw SVG that MkDocs never rewrites, so they have to
    be written in the shape the build being configured uses.
    """
    relationship_render.use_directory_urls(config["use_directory_urls"])
    return config


def _cache_key(config):
    """What makes two builds' diagrams the same.

    The rendered SVG has the page links baked into it, so a build that
    addresses pages differently needs its own entry. One process can see both:
    `mkdocs serve` reloads the configuration, and the offline bundle is built
    from a second configuration.
    """
    return (os.path.dirname(config["config_file_path"]), bool(config["use_directory_urls"]))


def _diagrams(config):
    """Return {api_page_stem: rendered_svg} for this repo, built once per shape."""
    root = os.path.dirname(config["config_file_path"])
    key = _cache_key(config)
    if key not in _CACHE:
        out = {}
        consumers = relationship_data.build_consumers(root)
        for role, info in relationship_data.build_graph(root).items():
            clients = consumers.get(role, [])
            out[info["link_page"]] = relationship_render.render_role_combined(role, info, clients)
        for page, entry in relationship_data.build_reverse(root).items():
            svg = relationship_render.render_reverse(entry)
            out[page] = svg
            if entry["config_page"]:
                out[entry["config_page"]] = svg
        facade = relationship_data.build_facade(root)
        if facade is not None:
            out[facade["pages"][0]] = relationship_render.render_facade_class(facade)
            out[facade["pages"][1]] = relationship_render.render_facade_stacked(facade)
        _CACHE[key] = out
    return _CACHE[key]


def on_page_markdown(markdown, page, config, files, **kwargs):
    src = getattr(page.file, "src_uri", None) or page.file.src_path.replace(os.sep, "/")
    if not (src.startswith("api/") and src.endswith(".md")):
        return markdown
    # Keyed by canonical page stem — a role's compound page, an implementer's file
    # page, a config compound page. Unmatched stems (source listings, etc.) skip.
    svg = _diagrams(config).get(src[len("api/"):-len(".md")])
    return svg + "\n\n" + markdown if svg else markdown
