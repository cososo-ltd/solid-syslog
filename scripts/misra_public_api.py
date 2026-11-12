#!/usr/bin/env python3
"""Drop the rule 8.7 findings that deviation D.016 covers.

Reads the cppcheck-misra finding lines and returns those still uncovered.
"""

import glob
import os
import re

FINDING = re.compile(r"^(?P<file>[^:\s]+):(?P<line>\d+):(?P<column>\d+): .*\[misra-c2012-8\.7\]")
IDENTIFIER = re.compile(r"[A-Za-z_]\w*")


def uncovered(findings, root):
    public = public_declarations(root)
    return [finding for finding in findings if not is_public_api(finding, public, root)]


def public_declarations(root):
    headers = glob.glob(os.path.join(root, "Core", "Interface", "*.h"))
    headers += glob.glob(os.path.join(root, "Platform", "*", "Interface", "*.h"))
    names = set()
    for header in headers:
        with open(header, encoding="utf-8") as handle:
            names.update(IDENTIFIER.findall(handle.read()))
    return names


def is_public_api(finding, public, root):
    match = FINDING.match(finding)
    if not match:
        return False
    with open(os.path.join(root, match["file"]), encoding="utf-8") as handle:
        source = handle.read().split("\n")[int(match["line"]) - 1]
    named = IDENTIFIER.match(source, int(match["column"]) - 1)
    return bool(named) and named.group(0) in public
