"""Extract the vtable "realised by" graph from the public headers.

The dependency-inversion structure makes every implementer of a vtable role a
public factory that returns ``struct SolidSyslog<Role>*``. Scanning the interface
headers for that return-type signal yields, for each role, the backends that
realise it — with each backend's package (its folder) and whether it is the Null
object — entirely from code, no hand-maintained list.

Run standalone (``python hooks/relationship_data.py``) to print a summary.
"""

import os
import re
import glob
import functools

# struct SolidSyslog<Role> *  SolidSyslog<Impl>_(Create|Get) (
_FACTORY = re.compile(
    r"struct\s+SolidSyslog(\w+)\s*\*\s*SolidSyslog(\w+)_(Create|Get)\s*\(", re.S
)


def _scan_dirs(root):
    dirs = [os.path.join(root, "Core", "Interface")]
    dirs += sorted(glob.glob(os.path.join(root, "Platform", "*", "Interface")))
    return [d for d in dirs if os.path.isdir(d)]


def _package(path, root):
    rel = os.path.relpath(path, root).replace(os.sep, "/")
    parts = rel.split("/")
    if parts[0] == "Platform" and len(parts) > 1:
        return parts[1]
    return "Core"


# A struct DEFINED with a body: mkdoxy renders a structSolidSyslog<Name> compound
# page for it and links every use of the type name there. An opaque forward-decl
# (the SolidSyslog handle, SolidSyslogAddress) has no such page, so its name
# resolves to a file page instead. This is what decides a node's link target.
_STRUCT_DEF = re.compile(r"struct\s+SolidSyslog(\w+)\s*\{")


@functools.lru_cache(maxsize=None)
def _defined_structs(root):
    names = set()
    for directory in _scan_dirs(root):
        for path in glob.glob(os.path.join(directory, "*.h")):
            with open(path, encoding="utf-8") as handle:
                names.update(_STRUCT_DEF.findall(handle.read()))
    return names


def _canonical(name, defined, file_page):
    """The page mkdoxy links a type name to: its compound page when the struct is
    defined, else the given file page. Diagram nodes point here, and each diagram
    is injected here, so a name-click (in prose or a diagram) always lands with it."""
    return "structSolidSyslog" + name if name in defined else file_page


@functools.lru_cache(maxsize=None)
def build_graph(root):
    """Return {role: {"implementers": [ {name, page, package, is_null}, ... ]}}.

    Memoized (pure in root): build_reverse/build_facade/build_consumers and the
    diagram hook each call it, so caching collapses the header rescan to one pass
    per build. Callers must treat the returned graph as read-only."""
    roles = {}
    for directory in _scan_dirs(root):
        for path in sorted(glob.glob(os.path.join(directory, "*.h"))):
            with open(path, encoding="utf-8") as handle:
                text = handle.read()
            page = os.path.basename(path)[:-2] + "_8h"
            for role, impl, _factory in _FACTORY.findall(text):
                roles.setdefault(role, {"implementers": []})
                roles[role]["implementers"].append(
                    {
                        "name": impl,
                        "page": page,
                        "package": _package(path, root),
                        "is_null": impl.startswith("Null"),
                        "header": path,
                    }
                )
    core = os.path.join(root, "Core", "Interface")
    defined = _defined_structs(root)
    for role, info in roles.items():
        # The file page: the dispatch header if there is one (what users browse),
        # else the Definition header (e.g. SecurityPolicy has no plain
        # SolidSyslogSecurityPolicy.h, only the vtable Definition).
        if os.path.exists(os.path.join(core, "SolidSyslog" + role + ".h")):
            info["base_page"] = "SolidSyslog" + role + "_8h"
        else:
            info["base_page"] = "SolidSyslog" + role + "Definition_8h"
        # The link target / injection page: the vtable's compound page where one
        # exists (Stream, Sender, …), else the file page for an opaque role (Address).
        info["link_page"] = _canonical(role, defined, info["base_page"])
        info["implementers"].sort(key=lambda i: (i["is_null"], i["name"]))
    return roles


# struct SolidSyslog<Role> *  (or ** for a list) — a role-typed collaborator slot.
_ROLE_PTR = re.compile(r"struct\s+SolidSyslog(\w+)\s*(\*+)")
_COMMENT = re.compile(r"/\*.*?\*/", re.S)


def _balanced(text, open_char, close_char, at):
    """Return the substring between the delimiters starting at index ``at``."""
    depth, i = 0, at
    while i < len(text):
        if text[i] == open_char:
            depth += 1
        elif text[i] == close_char:
            depth -= 1
            if depth == 0:
                return text[at + 1 : i]
        i += 1
    return ""


# A config struct field: role pointer, member name, ';', then the rest of the
# line — where a trailing /**< @optional ... */ marker (if any) lives.
_CONFIG_FIELD = re.compile(r"struct\s+SolidSyslog(\w+)\s*(\*+)\s*\w+\s*;[ \t]*([^\n]*)")


def _region(text, pattern):
    """The balanced { } or ( ) region a pattern opens, comments intact."""
    match = re.search(pattern, text)
    if not match:
        return ""
    opener = text[match.end() - 1]
    return _balanced(text, opener, "}" if opener == "{" else ")", match.end() - 1)


def _collaborators(header, impl, roles):
    """Role-typed slots this implementer is wired with — its ``<X>Config`` struct
    fields and its ``_Create`` parameters, in that order, filtered to known roles.
    A ``**`` slot is a list (upper bound ``*``); a config field whose trailing
    doc-comment carries the ``@optional`` marker has lower bound 0 (else 1, the
    header's "required" contract). ``_Create`` params are injected directly and are
    never optional today."""
    with open(header, encoding="utf-8") as handle:
        text = handle.read()
    out, seen = [], set()
    config = _region(text, r"struct\s+SolidSyslog" + impl + r"Config\s*\{")
    for role, stars, rest in _CONFIG_FIELD.findall(config):
        if role in roles and role not in seen:
            seen.add(role)
            out.append({"role": role, "is_list": len(stars) >= 2, "is_optional": "@optional" in rest})
    params = _COMMENT.sub("", _region(text, r"SolidSyslog" + impl + r"_Create\s*\("))
    for role, stars in _ROLE_PTR.findall(params):
        if role in roles and role not in seen:
            seen.add(role)
            out.append({"role": role, "is_list": len(stars) >= 2, "is_optional": False})
    return out


@functools.lru_cache(maxsize=None)
def build_reverse(root):
    """Return {impl_page: {name, package, is_null, base_role, base_page, config_page,
    collaborators:[{role, base_page, is_list, is_optional}]}} — the per-implementer
    view: the role it realises (upward) and the roles it is wired to use (outward).
    base_page here is a *link target* (canonical page), not necessarily a file page.
    config_page is the implementer's config compound page, if it has one."""
    graph = build_graph(root)
    defined = _defined_structs(root)
    role_link = {role: info["link_page"] for role, info in graph.items()}
    reverse = {}
    for role, info in graph.items():
        for impl in info["implementers"]:
            collaborators = _collaborators(impl["header"], impl["name"], role_link)
            config = impl["name"] + "Config"
            reverse[impl["page"]] = {
                "name": impl["name"],
                "package": impl["package"],
                "is_null": impl["is_null"],
                "base_role": role,
                "base_page": role_link[role],
                "config_page": "structSolidSyslog" + config if config in defined else None,
                "collaborators": [
                    {"role": c["role"], "base_page": role_link[c["role"]],
                     "is_list": c["is_list"], "is_optional": c["is_optional"]}
                    for c in collaborators
                ],
            }
    return reverse


# The composition root: the one _Create that returns the bare `struct SolidSyslog*`
# handle (no role suffix), taking a `SolidSyslog<...>Config`. This is the structural
# signal that distinguishes the facade from a role implementer — no hand-kept list.
_FACADE_FACTORY = re.compile(
    r"struct\s+SolidSyslog\s*\*\s*SolidSyslog_Create\s*\(\s*const\s+struct\s+(SolidSyslog\w*Config)\b"
)


@functools.lru_cache(maxsize=None)
def build_facade(root):
    """The composition-root view: the SolidSyslog facade and the roles its config
    wires it to (uses-only — the facade realises no role). Collaborators are
    scraped from the config struct exactly as for an implementer. Returns None if
    the facade factory is not found."""
    role_link = {role: info["link_page"] for role, info in build_graph(root).items()}
    header = os.path.join(root, "Core", "Interface", "SolidSyslogConfig.h")
    if not os.path.exists(header):
        return None
    with open(header, encoding="utf-8") as handle:
        text = handle.read()
    match = _FACADE_FACTORY.search(text)
    if not match:
        return None
    config = _region(text, r"struct\s+" + match.group(1) + r"\s*\{")
    collaborators, seen = [], set()
    for role, stars, rest in _CONFIG_FIELD.findall(config):
        if role in role_link and role not in seen:
            seen.add(role)
            collaborators.append({"role": role, "base_page": role_link[role],
                                  "is_list": len(stars) >= 2, "is_optional": "@optional" in rest})
    # pages[0] = the class view's page: the opaque facade handle has no compound
    # page, so its name resolves to the SolidSyslog file page. pages[1] = the config
    # view's page: SolidSyslogConfig has a compound page, where its name links.
    return {"name": "SolidSyslog", "pages": ["SolidSyslog_8h", "structSolidSyslogConfig"],
            "collaborators": collaborators}


def build_consumers(root):
    """Return {role: [ {name, page, package}, ... ]} — who *uses* each role: the
    transpose of the collaborator map (every implementer's ``uses`` edges), plus
    the SolidSyslog facade. Completes the navigation graph — from a role page you
    can reach both its implementers and its clients."""
    consumers = {}
    for page, entry in build_reverse(root).items():
        for collab in entry["collaborators"]:
            consumers.setdefault(collab["role"], []).append(
                {"name": entry["name"], "page": page, "package": entry["package"]})
    facade = build_facade(root)
    if facade is not None:
        for collab in facade["collaborators"]:
            consumers.setdefault(collab["role"], []).append(
                {"name": facade["name"], "page": facade["pages"][0], "package": "Core"})
    for role in consumers:
        consumers[role].sort(key=lambda c: c["name"])
    return consumers


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.dirname(here)
    graph = build_graph(repo)
    longest = 0
    for role in sorted(graph, key=lambda r: -len(graph[r]["implementers"])):
        impls = graph[role]["implementers"]
        print("\n{}  ({} backends)  base={}".format(role, len(impls), graph[role]["base_page"]))
        for i in impls:
            longest = max(longest, len(i["name"]))
            flag = " [null]" if i["is_null"] else ""
            print("    {:22} {:10}{}".format(i["name"], i["package"], flag))
    print(f"\nroles: {len(graph)}   longest backend name: {longest} chars")
