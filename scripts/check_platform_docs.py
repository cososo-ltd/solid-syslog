#!/usr/bin/env python3
"""Assert the documentation matches what the code declares.

Two things are declared in the code and enumerated in the docs — the platforms
and the roles — and in both cases the enumeration is written by a person and
checked here, never generated. A missing entry is a decision not yet made, not a
mechanical gap; generating a placeholder would hide it.

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

It also holds three boundaries that hand review kept losing:

* **No platform names another.** A platform describes itself completely; where
  a capability comes from is the capability matrix's job. Naming a sibling
  couples the two, so the eleventh platform means editing ten pages. Applies to
  the platform's whole tree and its docs folder alike.
* **Every class a platform declares carries its token.** The rule is stated in
  docs/NAMING.md, "Platform classes carry their pack's registry token", and had
  nothing asserting it — which is how the naming drifted far enough to need a
  rename.
* **Every header its platform ships is on its page.** The *What it ships*
  manifest is generated from the Interface directory, so this asserts the
  heading the generator writes into is still there.

A role is declared by a Core/Interface/SolidSyslog<Role>Definition.h header, and
listed in three hand-written places: the porting guide, the roles index, and the
mkdocs nav. Each must link the role's generated contract page. The count used to
be stated in prose in six places, so a thirteenth role meant six edits and no
failure; the count is gone and this is what replaces it.

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
    "StdAtomic": [],
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

# A pack wrapping a genuinely distinct second upstream prefixes those classes
# with that upstream instead of its token — docs/NAMING.md, "Platform classes
# carry their pack's registry token". Kept apart from ALIASES above, which is
# prose vocabulary: "POSIX" is how the Posix platform is written in a sentence,
# and must not become a licence to declare SolidSyslogPOSIXFile. An entry is
# agreed when the second upstream is taken on, so widening this is deliberate.
CLASS_PREFIXES = {
    "Windows": ["Winsock"],
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


def interface_headers(directory):
    """The header filenames a platform declares, sorted. Empty if it declares none."""
    interface = os.path.join(ROOT, directory, "Interface")
    if not os.path.isdir(interface):
        return []
    return sorted(name for name in os.listdir(interface) if name.endswith(".h"))


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
        for alias in [token, *ALIASES.get(token, [])]:
            terms.setdefault(alias, set()).add(token)
        for header in interface_headers(directory):
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


DESCRIPTIONS = os.path.join("hooks", "page_descriptions.py")


def described(slug):
    """The meta descriptions a platform's pages publish, with line numbers.

    They are page content — the snippet a search result shows — but they live in
    a hook rather than in the page, so the walk above never reached them. That
    gap is not theoretical: three platform descriptions named a sibling platform
    while this check reported the boundary clean.
    """
    keys = (f'"platforms/{slug}/index.md":', f'"platforms/{slug}/setup.md":')
    inside = False
    for number, line in enumerate(read(DESCRIPTIONS).splitlines(), 1):
        if line.strip().startswith(keys):
            inside = True
        if inside:
            yield number, line
            if line.rstrip().endswith("),"):
                inside = False


def speaks_for(directory, slug):
    """Every line that speaks for one platform, as (source, line number, text).

    Its own tree, its pages, and the descriptions published for those pages.
    """
    for relative in scanned(directory, slug):
        for number, line in enumerate(read(relative).splitlines(), 1):
            yield relative, number, line
    for number, line in described(slug):
        yield DESCRIPTIONS, number, line


def naming_faults(rows, terms):
    """Flag a platform naming another, longest term first so FreeRTOS-Plus-TCP
    is read as itself rather than as FreeRTOS."""
    ordered = sorted(terms, key=len, reverse=True)
    pattern = re.compile(r"(?<![\w-])(" + "|".join(re.escape(t) for t in ordered) + r")(?![\w-])")
    exempt = {(path, term) for path, term, _ in ALLOWED}
    faults = []

    for token, directory in rows:
        for relative, number, line in speaks_for(directory, token.lower()):
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


def declared_roles():
    """[Role] from Core/Interface/SolidSyslog<Role>Definition.h, the declaration."""
    interface = os.path.join(ROOT, "Core", "Interface")
    suffix = "Definition.h"
    return sorted(
        name[len("SolidSyslog") : -len(suffix)]
        for name in os.listdir(interface)
        if name.startswith("SolidSyslog") and name.endswith(suffix)
    )


def role_faults():
    """Every declared role is listed wherever roles are enumerated, and nothing
    is listed that is not declared."""
    faults = []
    # Where roles are enumerated, and how far each sits from the api/ tree.
    listings = {
        os.path.join("docs", "porting.md"): "api/",
        os.path.join("docs", "roles", "index.md"): "../api/",
        "mkdocs.yml": "api/",
    }
    roles = declared_roles()

    for listing, prefix in listings.items():
        text = read(listing)
        for role in roles:
            if f"{prefix}structSolidSyslog{role}.md" not in text:
                faults.append(f"{role}: declared by its Definition.h but not linked from {listing}")
        # A role page that outlived its contract — the rename nobody finished.
        for orphan in re.findall(rf"{re.escape(prefix)}structSolidSyslog(\w+)\.md", text):
            if orphan not in roles:
                faults.append(f"{listing} links {orphan} as a role, but no SolidSyslog{orphan}Definition.h declares it")
    return faults


def prefix_faults(rows):
    """Every header a platform declares begins with SolidSyslog<Token>, or with
    an agreed second-upstream prefix from CLASS_PREFIXES.

    docs/NAMING.md states this for public classes. An *Errors.h takes its name
    from the class it belongs to, so it carries the token by construction and is
    checked alongside the rest — one that does not is an orphan worth catching.
    """
    faults = []
    for token, directory in rows:
        accepted = [token, *CLASS_PREFIXES.get(token, [])]
        for header in interface_headers(directory):
            if not any(header.startswith(f"SolidSyslog{prefix}") for prefix in accepted):
                wanted = " or ".join(f"SolidSyslog{prefix}" for prefix in accepted)
                faults.append(
                    f"{token}: {directory}/Interface/{header} does not begin {wanted} — "
                    "a platform's classes carry its registry token"
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

    faults.extend(prefix_faults(rows))
    faults.extend(unlisted_headers(rows))
    faults.extend(naming_faults(rows, vocabulary(rows)))
    faults.extend(role_faults())
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
    for token, prefixes in sorted(CLASS_PREFIXES.items()):
        spellings = ", ".join(f"SolidSyslog{prefix}*" for prefix in prefixes)
        print(f"allowed: {token} may also declare {spellings} — a second upstream, agreed when it was taken on")
    print(
        f"docs match the code: {len(registered())} platforms, all documented, none naming "
        f"another and each declaring only classes that carry its token; "
        f"{len(declared_roles())} roles, each listed everywhere roles are enumerated"
    )
