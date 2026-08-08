#!/usr/bin/env python3
"""Assert every registered platform is documented, and nothing else claims to be.

A platform is declared once, in SOLIDSYSLOG_PLATFORM_REGISTRY in the top-level
CMakeLists.txt. Everything else follows from its token:

    token       LwipRaw
    directory   Platform/LwipRaw/
    docs        docs/platforms/lwipraw/{index,setup}.md
    nav         an entry in mkdocs.yml
    description an entry in hooks/page_descriptions.py

This checks that each of those exists for each row, and the reverse — a docs
folder or a group with no row behind it. Registering a platform is then the one
edit that cannot be forgotten, because forgetting anything else fails the build.

It also holds two boundaries that hand review kept losing:

* **No platform names another.** A platform describes itself completely; where
  a capability comes from is the capability matrix's job. Naming a sibling
  couples the two, so the eleventh platform means editing ten pages. Applies to
  the platform's whole tree and its docs folder alike.
* **Every header its platform ships is on its page.** The *What it ships*
  manifest is generated from the Interface directory, so this asserts the
  heading the generator writes into is still there.

Run:  python3 scripts/check_platform_docs.py
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

REGISTRY = re.compile(r"set\(SOLIDSYSLOG_PLATFORM_REGISTRY(.*?)^\)", re.DOTALL | re.MULTILINE)
ROW = re.compile(r'"([^"|]+)\|[^"|]*\|[^"|]*\|[^"|]*\|([^"|]+)\|[^"]*"')

# What each platform is called in prose, where that differs from its registry
# token. Tokens and class names are derived, so this is the only hand-kept part
# of the vocabulary — and every registered token must appear, so adding a
# platform forces the decision rather than silently widening the gap.
ALIASES = {
    "Atomics": [],
    "FatFs": ["FatFs", "ChaN"],
    "FreeRtos": ["FreeRTOS"],
    "LwipRaw": ["lwIP"],
    "MbedTls": ["Mbed TLS", "mbedTLS", "mbedtls"],
    "OpenSsl": ["OpenSSL"],
    # Plus-FAT and Plus-TCP are FreeRTOS-Plus-* products, so "FreeRTOS" is part
    # of their own identity as much as it is the kernel pack's — each may name
    # the kernel it sits on. The pack's token stays the pack's, so a page that
    # points at the FreeRtos platform is still caught.
    "PlusFat": ["FreeRTOS-Plus-FAT", "FreeRTOS"],
    "PlusTcp": ["FreeRTOS-Plus-TCP", "FreeRTOS"],
    "Posix": ["POSIX"],
    "Windows": ["Winsock", "Win32"],
}

# Deliberate exemptions, each with the reason it is not a boundary breach.
# Printed on every run: an exemption nobody sees is an exemption nobody
# revisits.
ALLOWED = [
    (
        "Platform/Windows/Source/SolidSyslogWindowsFile.c",
        "POSIX",
        "names the OS standard the MSVC call is measured against, not the Posix platform",
    ),
    (
        "Platform/Windows/Source/SolidSyslogWinsockTcpStream.c",
        "POSIX",
        "names the OS standard the MSVC call is measured against, not the Posix platform",
    ),
]

SCANNED_SUFFIXES = (".c", ".h", ".md")

# An #include names a header the compiler must find, not a platform the prose
# is describing. The boundary is editorial; what a translation unit depends on
# is the build's business and is governed there.
INCLUDE = re.compile(r"^\s*#\s*include")


def read(*parts):
    with open(os.path.join(ROOT, *parts), encoding="utf-8") as handle:
        return handle.read()


def registered():
    """[(token, directory)] from the registry, which is the single declaration."""
    found = REGISTRY.search(read("CMakeLists.txt"))
    if found is None:
        sys.exit("SOLIDSYSLOG_PLATFORM_REGISTRY not found in CMakeLists.txt")
    return ROW.findall(found.group(1))


def documented():
    """Slugs that have a docs folder, whether or not anything registered them."""
    platforms = os.path.join(ROOT, "docs", "platforms")
    return {
        name
        for name in os.listdir(platforms)
        if os.path.isdir(os.path.join(platforms, name))
    }


def vocabulary(rows):
    """Every term that names a platform, mapped to the tokens that may use it.

    Three sources, only one of them hand-kept: the registry token itself, the
    prose aliases above, and the class names taken from the platform's own
    Interface headers — in both their full and bare spellings, since prose says
    PosixTcpStream where code says SolidSyslogPosixTcpStream.

    A term can have more than one owner: "FreeRTOS" belongs to the kernel pack
    and to the two FreeRTOS-Plus-* platforms built on it alike.
    """
    terms = {}
    for token, directory in rows:
        for alias in [token] + ALIASES.get(token, []):
            terms.setdefault(alias, set()).add(token)
        interface = os.path.join(ROOT, directory, "Interface")
        for header in sorted(os.listdir(interface)) if os.path.isdir(interface) else []:
            if header.endswith(".h"):
                stem = header[: -len(".h")]
                terms.setdefault(stem, set()).add(token)
                terms.setdefault(stem[len("SolidSyslog") :], set()).add(token)
    return terms


def scanned(directory, slug):
    """Everything that speaks for one platform: its own tree, and its pages."""
    for base in (os.path.join(ROOT, directory), os.path.join(ROOT, "docs", "platforms", slug)):
        for path, _, names in os.walk(base):
            for name in sorted(names):
                if name.endswith(SCANNED_SUFFIXES):
                    yield os.path.relpath(os.path.join(path, name), ROOT)


def naming_faults(rows, terms):
    """Flag a platform naming another, longest term first so FreeRTOS-Plus-TCP
    is read as itself rather than as FreeRTOS."""
    ordered = sorted(terms, key=len, reverse=True)
    pattern = re.compile(r"(?<![\w-])(" + "|".join(re.escape(t) for t in ordered) + r")(?![\w-])")
    exempt = {(path, term) for path, term, _ in ALLOWED}
    faults = []

    for token, directory in rows:
        for relative in scanned(directory, token.lower()):
            for number, line in enumerate(read(relative).splitlines(), 1):
                if INCLUDE.match(line):
                    continue
                for term in {m.group(1) for m in pattern.finditer(line)}:
                    owners = terms[term]
                    if token not in owners and (relative, term) not in exempt:
                        named = "/".join(sorted(owners))
                        faults.append(
                            f"{token}: {relative}:{number} names {named} "
                            f'("{term}") — a platform describes only itself'
                        )
    return faults


def unlisted_headers(rows):
    """The *What it ships* manifest is generated by hooks/platform_backlinks.py
    from the platform's own Interface directory, so every header is listed by
    construction. What can still go wrong is the heading disappearing, which
    would silently take the whole manifest with it."""
    faults = []
    for token, _ in rows:
        page = os.path.join("docs", "platforms", token.lower(), "index.md")
        if os.path.isfile(os.path.join(ROOT, page)) and "## What it ships" not in read(page):
            faults.append(f"{token}: {page} has no '## What it ships' heading for the manifest")
    return faults


def check():
    faults = []
    nav = read("mkdocs.yml")
    descriptions = read("hooks", "page_descriptions.py")
    matrix = read("docs", "platforms", "index.md")
    slugs = set()
    rows = registered()

    for token, _ in rows:
        if token not in ALIASES:
            faults.append(f"{token}: no ALIASES entry — add its prose spellings, or [] if the token is the only one")

    for token, directory in rows:
        slug = token.lower()
        slugs.add(slug)
        docs = os.path.join("docs", "platforms", slug)

        for page in ("index.md", "setup.md"):
            if not os.path.isfile(os.path.join(ROOT, docs, page)):
                faults.append(f"{token}: {docs}/{page} is missing")

        if f"platforms/{slug}/index.md" not in nav:
            faults.append(f"{token}: no entry in the mkdocs.yml nav")
        for page in ("index.md", "setup.md"):
            if f'"platforms/{slug}/{page}"' not in descriptions:
                faults.append(f"{token}: no meta description for platforms/{slug}/{page}")

        if f"]({slug}/index.md)" not in matrix:
            faults.append(f"{token}: not a row in the docs/platforms/index.md matrix")

    for slug in sorted(documented() - slugs):
        faults.append(f"docs/platforms/{slug}/ documents a platform that is not registered")

    faults.extend(unlisted_headers(rows))
    faults.extend(naming_faults(rows, vocabulary(rows)))
    return faults


if __name__ == "__main__":
    problems = check()
    for problem in problems:
        print(f"error: {problem}", file=sys.stderr)
    if problems:
        print(
            f"\n{len(problems)} problem(s). A platform is declared in "
            "SOLIDSYSLOG_PLATFORM_REGISTRY; everything above follows from its token.",
            file=sys.stderr,
        )
        sys.exit(1)
    for path, term, reason in ALLOWED:
        print(f"allowed: {path} may say {term} — {reason}")
    print(f"platform docs: {len(registered())} platforms, all documented, none naming another")
