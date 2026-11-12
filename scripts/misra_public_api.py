#!/usr/bin/env python3
"""Report the cppcheck findings no suppression or deviation covers.

misra_suppressions.txt covers findings line by line. Deviation D.016 in
docs/misra-deviations.md covers one more set by what it is rather than where:
a rule 8.7 finding on a function or object a public header declares - one
under Core/Interface/ or Platform/*/Interface/ - is the library's API, external
so an integrator can call it. Any other 8.7 finding, every finding of any other
rule, and any finding that is not MISRA's at all - an addon that failed to run
among them - is printed. This is the step's whole verdict, so cppcheck runs
without --error-exitcode, which would count the covered findings too.

Run:  python3 scripts/misra_public_api.py build/cppcheck-misra/cppcheck-misra.log

Exit code:
    0  every finding in the log is covered
    1  a finding is printed above
"""

import argparse
import glob
import os
import re
import sys

FINDING = re.compile(r"^(?P<file>[^:\s]+):(?P<line>\d+):(?P<column>\d+): .*\[misra-c2012-8\.7\]")
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ANY_FINDING = re.compile(r"^\S+:\d+:\d+: \w+: .*\[[\w.-]+\]$")
IDENTIFIER = re.compile(r"[A-Za-z_]\w*")
COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
EXTERN_OBJECT = re.compile(r"\s*extern\b")
EXTERN_C = re.compile(r'\bextern\s*"C"\s*\{')
TYPEDEF = re.compile(r"\s*typedef\b")
LITERAL = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'')
PREPROCESSOR = re.compile(r"^[ \t]*#(?:[^\n]*\\\n)*[^\n]*", re.MULTILINE)


def uncovered(findings, root):
    public = public_declarations(root)
    return [finding for finding in findings if not is_public_api(finding, public, root)]


def public_declarations(root):
    headers = glob.glob(os.path.join(root, "Core", "Interface", "*.h"))
    headers += glob.glob(os.path.join(root, "Platform", "*", "Interface", "*.h"))
    names = set()
    for header in headers:
        with open(header, encoding="utf-8") as handle:
            names.update(declared_name(statement) for statement in statements(handle.read()))
    names.discard(None)
    return names


def statements(header):
    """The header's statements, with nothing left in them that is not C: no comments, preprocessor lines or literals."""
    text = COMMENT.sub(" ", header)
    text = PREPROCESSOR.sub(" ", text)
    text = EXTERN_C.sub(" ", text)
    text = LITERAL.sub(" ", text)
    return text.split(";")


def declared_name(statement):
    """The function or extern object a statement declares, or None for anything else."""
    if TYPEDEF.match(statement):
        return None
    before_parameters, parenthesis, _ = statement.partition("(")
    if parenthesis:
        return last_identifier(before_parameters)
    if EXTERN_OBJECT.match(statement):
        return last_identifier(re.split(r"[\[=]", statement, maxsplit=1)[0])
    return None


def last_identifier(text):
    identifiers = IDENTIFIER.findall(text)
    return identifiers[-1] if identifiers else None


def is_public_api(finding, public, root):
    match = FINDING.match(finding)
    if not match:
        return False
    with open(os.path.join(root, match["file"]), encoding="utf-8") as handle:
        source = handle.read().split("\n")[int(match["line"]) - 1]
    named = IDENTIFIER.match(source, int(match["column"]) - 1)
    return bool(named) and named.group(0) in public


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("log", help="the cppcheck-misra log")
    parser.add_argument("--root", default=ROOT, help="the source tree the log's paths are relative to")
    arguments = parser.parse_args(argv)
    with open(arguments.log, encoding="utf-8") as handle:
        findings = [line.rstrip("\n") for line in handle if ANY_FINDING.match(line.rstrip("\n"))]
    remaining = uncovered(findings, arguments.root)
    for finding in remaining:
        print(finding)
    return 1 if remaining else 0


if __name__ == "__main__":
    sys.exit(main())
