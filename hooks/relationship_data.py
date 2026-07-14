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


def build_graph(root):
    """Return {role: {"implementers": [ {name, page, package, is_null}, ... ]}}."""
    roles = {}
    for directory in _scan_dirs(root):
        for path in sorted(glob.glob(os.path.join(directory, "*.h"))):
            with open(path, "r", encoding="utf-8") as handle:
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
                    }
                )
    core = os.path.join(root, "Core", "Interface")
    for role, info in roles.items():
        # Inject on the consumer dispatch header if there is one (what users
        # browse); else on the Definition header (e.g. SecurityPolicy has no
        # plain SolidSyslogSecurityPolicy.h, only the vtable Definition).
        if os.path.exists(os.path.join(core, "SolidSyslog" + role + ".h")):
            info["base_page"] = "SolidSyslog" + role + "_8h"
        else:
            info["base_page"] = "SolidSyslog" + role + "Definition_8h"
        info["struct_page"] = "structSolidSyslog" + role
        info["implementers"].sort(key=lambda i: (i["is_null"], i["name"]))
    return roles


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
    print("\nroles: {}   longest backend name: {} chars".format(len(graph), longest))
