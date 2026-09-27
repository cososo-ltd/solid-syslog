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
* **A list claiming to name every platform names every platform.** A region
  marked `<!-- platforms: <scope> -->` must name each platform the scope covers,
  where the scope is a registry field: `all`, `kind=probe` or `roles=tls`. The
  enumeration stays a sentence or a table a person wrote - what changes is that
  a fourteenth pack fails the build everywhere that claims to list them all.
  Four separate lists had drifted the same way by the time this was written.

A role is declared by a Core/Interface/SolidSyslog<Role>Definition.h header, and
listed in three hand-written places: the porting guide, the roles index, and the
mkdocs nav. Each must link the role's generated contract page. The count used to
be stated in prose in six places, so a thirteenth role meant six edits and no
failure; the count is gone and this is what replaces it.

One role is declared per platform instead: the TLS credentials vtable, which
each TLS pack declares for itself because what Install configures is
backend-typed. Those are held to the roles index alone. They are not Core's, so
they do not belong in a nav section listing Core's, and the porting guide covers
them in its TLS material rather than in the role table.

Run:  python3 scripts/check_platform_docs.py
"""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

REGISTRY = re.compile(r"set\(SOLIDSYSLOG_PLATFORM_REGISTRY(.*?)^\)", re.DOTALL | re.MULTILINE)
ROW = re.compile(r'"([^"|]+)\|[^"|]*\|[^"|]*\|([^"|]*)\|([^"|]+)\|([^"]*)"')

# What each platform is called in prose, where that differs from its registry
# token. Tokens and class names are derived, so this is the only hand-kept part
# of the vocabulary — and every registered token must appear, so adding a
# platform forces the decision rather than silently widening the gap.
ALIASES = {
    # The pack is the language feature, and prose says the feature.
    "StdAtomic": ["C11 atomics"],
    # The pack is named for the API it targets, and prose says the API's own
    # name. No kernel is an alias: the adapter is not written against one.
    "CmsisRtos": ["CMSIS-RTOS2", "CMSIS"],
    "FatFs": ["FatFs", "ChaN"],
    # "FreeRTOS" alone belongs to the kernel pack and to both FreeRTOS-Plus-*
    # packs, so in a list naming all three it identifies none of them. The
    # kernel pack needs a spelling of its own for that case.
    "FreeRtos": ["FreeRTOS", "FreeRTOS kernel"],
    "LittleFs": ["LittleFS", "littlefs"],
    # Two packs target the same stack at different API tiers, so "lwIP" is part
    # of each one's own identity rather than the other's name, the way FreeRTOS
    # is for the Plus-* packs below. The tier-qualified spellings stay exclusive,
    # so a page reaching across to the other tier is still caught.
    "LwipRaw": ["lwIP", "lwIP (Raw API)", "Raw API"],
    "LwipSocket": ["lwIP", "lwIP (Sockets API)", "Sockets API"],
    "MbedTls": ["Mbed TLS", "mbedTLS", "mbedtls"],
    "OpenSsl": ["OpenSSL"],
    # Plus-FAT and Plus-TCP are FreeRTOS-Plus-* products, so "FreeRTOS" is part
    # of their own identity as much as it is the kernel pack's — each may name
    # the kernel it sits on. The pack's token stays the pack's, so a page that
    # points at the FreeRtos platform is still caught.
    "PlusFat": ["FreeRTOS-Plus-FAT", "Plus-FAT", "FreeRTOS"],
    "PlusTcp": ["FreeRTOS-Plus-TCP", "Plus-TCP", "FreeRTOS"],
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

# A spelt-out count of the roles, which prose must not carry: the enumeration
# checks below are what a thirteenth role trips, and a number beside them just
# rots. Spelt-out only -- a bare digit would match version numbers and sizes.
ROLE_COUNT_IN_PROSE = re.compile(
    r"\b(?:eleven|twelve|thirteen|fourteen)\b(?=[^.]{0,40}?\b(?:roles?|contracts?)\b)",
    re.IGNORECASE,
)

# A region of a page that claims to name every platform in some scope, and the
# end of one. A comment rather than a fence so it renders as nothing: the reader
# sees the sentence or the table, and only the check sees the claim.
PLATFORM_LIST_OPEN = re.compile(r"<!--\s*platforms:\s*([^>]*?)\s*-->")
PLATFORM_LIST_CLOSE = "<!-- /platforms -->"

# A comment shaped like a marker, which means the keyword is the first thing in
# it. Every one of these must be an opening the strict pattern above parses or a
# closing marker; one it cannot read - a misspelt keyword, an opener missing its
# `-->` - would otherwise be skipped in silence, and a skipped marker is the one
# way a list escapes the check rather than failing it.
#
# Matching any comment that merely mentions a platform would be the wrong trade.
# That is prose, and failing the build on it would block an edit that is correct.
PLATFORM_LIST_LOOKALIKE = re.compile(r"<!--\s*/?\s*platforms?\b", re.IGNORECASE)

# An #include names a header the compiler must find, not a platform the prose
# is describing. The boundary is editorial; what a translation unit depends on
# is the build's business and is governed there.
INCLUDE = re.compile(r"^\s*#\s*include")


def read(*parts):
    with open(os.path.join(ROOT, *parts), encoding="utf-8") as handle:
        return handle.read()


def registry_rows():
    """[(token, kind, directory, [roles])] - every field the checks below read.

    A row is `token|option|default|kind|directory|roles`, as the macro that
    reads it in CMakeLists.txt states. The kind and the roles are what let a
    scoped platform list be checked against the declaration rather than by
    hand: "the platforms decided by a probe" and "the platforms filling the TLS
    role" are both already written down here.
    """
    found = REGISTRY.search(read("CMakeLists.txt"))
    if found is None:
        sys.exit("SOLIDSYSLOG_PLATFORM_REGISTRY not found in CMakeLists.txt")
    return [
        (token, kind, directory, roles.split())
        for token, kind, directory, roles in ROW.findall(found.group(1))
    ]


def registered():
    """[(token, directory)] from the registry, which is the single declaration."""
    return [(token, directory) for token, _, directory, _ in registry_rows()]


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


ROLES_INDEX = os.path.join("docs", "roles", "index.md")


def pack_roles():
    """[(Role, Pack)] from Platform/<Pack>/Interface/SolidSyslog<Role>Definition.h.

    A role a platform declares for itself rather than Core. There is one today,
    TLS credentials, declared twice because the material it installs is typed by
    the backend. Core's own check cannot see these, which is how the thirteenth
    role stayed off the roles index unnoticed.
    """
    found = []
    platform = os.path.join(ROOT, "Platform")
    suffix = "Definition.h"
    for pack in sorted(os.listdir(platform)):
        interface = os.path.join(platform, pack, "Interface")
        if not os.path.isdir(interface):
            continue
        for name in sorted(os.listdir(interface)):
            if name.startswith("SolidSyslog") and name.endswith(suffix):
                found.append((name[len("SolidSyslog") : -len(suffix)], pack))
    return found


def pack_role_faults():
    """Every per-platform role is linked from the roles index."""
    faults = []
    listing = ROLES_INDEX
    text = read(listing)
    for role, pack in pack_roles():
        if f"../api/structSolidSyslog{role}.md" not in text:
            faults.append(
                f"{role}: declared by {pack}'s Definition.h but not linked from {listing}"
            )
    return faults


def role_faults():
    """Every declared role is listed wherever roles are enumerated, and nothing
    is listed that is not declared."""
    faults = []
    # Where roles are enumerated, and how far each sits from the api/ tree.
    listings = {
        os.path.join("docs", "porting.md"): "api/",
        ROLES_INDEX: "../api/",
        "mkdocs.yml": "api/",
    }
    roles = declared_roles()

    for listing, prefix in listings.items():
        text = read(listing)
        for role in roles:
            if f"{prefix}structSolidSyslog{role}.md" not in text:
                faults.append(f"{role}: declared by its Definition.h but not linked from {listing}")
        # A role page that outlived its contract — the rename nobody finished.
        # A per-platform role is legitimate on the roles index and nowhere else,
        # so the other two listings still reject it as an unknown role. Widening
        # the set for all three would have let one into the Core nav unnoticed.
        known = set(roles)
        if listing == ROLES_INDEX:
            known |= {role for role, _ in pack_roles()}
        per_platform = {role: pack for role, pack in pack_roles()}
        for orphan in re.findall(rf"{re.escape(prefix)}structSolidSyslog(\w+)\.md", text):
            if orphan in known:
                continue
            if orphan in per_platform:
                faults.append(
                    f"{listing} links {orphan} as a role, but {per_platform[orphan]} "
                    f"declares it rather than Core - a per-platform role belongs on "
                    f"{ROLES_INDEX} alone"
                )
            else:
                faults.append(f"{listing} links {orphan} as a role, but no SolidSyslog{orphan}Definition.h declares it")
        # A count in prose is the thing this check replaced. It cannot be
        # asserted, so a thirteenth role would leave it quietly wrong.
        for spelt in re.findall(ROLE_COUNT_IN_PROSE, text):
            faults.append(
                f"{listing} states the number of roles in prose ('{spelt}') — "
                "the listing above is what keeps the set honest, so leave the count out"
            )
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


def markdown_pages():
    """Every page a platform list can appear on: docs/ and the root documents.

    The root documents are in scope because README.md carried one of the stale
    lists. hooks/page_descriptions.py is not: a search snippet has no room for
    thirteen of anything, so a list there would be wrong rather than incomplete.
    """
    for name in sorted(os.listdir(ROOT)):
        if name.endswith(".md"):
            yield name
    for path, _, names in os.walk(os.path.join(ROOT, "docs")):
        for name in sorted(names):
            if name.endswith(".md"):
                yield os.path.relpath(os.path.join(path, name), ROOT)


def marker_pairs(text):
    """[(opening, closing position or None)] for one page, in order.

    A later list's closer does not close this one, so the search is bounded at
    the next opening. Without the bound an unclosed list borrows the next one's
    closer, its body swallows that list, and it passes on names it never said -
    green, and wrong twice over.
    """
    openings = list(PLATFORM_LIST_OPEN.finditer(text))
    pairs = []
    for index, opening in enumerate(openings):
        limit = openings[index + 1].start() if index + 1 < len(openings) else len(text)
        end = text.find(PLATFORM_LIST_CLOSE, opening.end(), limit)
        pairs.append((opening, None if end == -1 else end))
    return pairs


def marked_lists():
    """(page, line number, scope, body) for every marked platform list.

    A body of None is an opening marker with no closer, which the caller
    reports rather than silently reading to the end of the page.
    """
    for page in markdown_pages():
        text = read(page)
        for opening, end in marker_pairs(text):
            number = text.count("\n", 0, opening.start()) + 1
            body = None if end is None else text[opening.end() : end]
            yield page, number, opening.group(1), body


def marker_faults():
    """Every platform-list marker parses as one, and closes what it claims to.

    A list that fails the completeness check says so. A marker the strict pattern
    cannot read says nothing at all, and a closing marker with nothing above it
    parses perfectly and still means nothing - so this is what keeps the rest of
    the mechanism honest.
    """
    faults = []
    for page in markdown_pages():
        text = read(page)
        pairs = marker_pairs(text)
        parsed = {opening.start() for opening, _ in pairs}
        closing = {end for _, end in pairs if end is not None}
        for hit in PLATFORM_LIST_LOOKALIKE.finditer(text):
            if hit.start() in parsed or hit.start() in closing:
                continue
            number = text.count("\n", 0, hit.start()) + 1
            if text.startswith(PLATFORM_LIST_CLOSE, hit.start()):
                faults.append(
                    f"{page}:{number} closes a platform list that was never opened - "
                    f"open one above it with `<!-- platforms: <scope> -->`, or take it out"
                )
            else:
                faults.append(
                    f"{page}:{number} reads as a platform-list marker but does not parse as "
                    f"one - an opening is `<!-- platforms: <scope> -->`, a closing is "
                    f"`{PLATFORM_LIST_CLOSE}`"
                )
    return faults


def list_scope(scope, rows):
    """The tokens a marked region must name, or None if the scope is unreadable.

    Three forms, each of them a registry field: `all`, `kind=<probe|upstream>`
    and `roles=<role>`. Nothing is hand-kept - the registry already states a
    row's selection kind and the roles it fills, so a scoped list is held to the
    same declaration as an exhaustive one.
    """
    if scope == "all":
        return [token for token, _, _, _ in rows]
    if scope.startswith("kind="):
        wanted = scope[len("kind=") :]
        return [token for token, kind, _, _ in rows if kind == wanted]
    if scope.startswith("roles="):
        wanted = scope[len("roles=") :]
        return [token for token, _, _, roles in rows if wanted in roles]
    return None


def names_term(text, term):
    """Whether a region says a term, on the word boundaries the vocabulary uses."""
    return re.search(r"(?<![\w-])" + re.escape(term) + r"(?![\w-])", text) is not None


def list_faults(rows, terms):
    """Every marked region names every platform in the scope it claims.

    A term two platforms in the same scope share names neither of them: "lwIP"
    stopped identifying a pack the day the second lwIP pack shipped, and this
    was written because a table had it standing for both. So a platform counts
    as named only by a term that is unambiguous within the scope - which is why
    the suggestion in the message is worth reading rather than guessing at.
    """
    faults = []
    for page, number, scope, body in marked_lists():
        if body is None:
            faults.append(
                f"{page}:{number} opens a platform list that is never closed - "
                f"put {PLATFORM_LIST_CLOSE} after the last entry"
            )
            continue
        wanted = list_scope(scope, rows)
        if wanted is None:
            faults.append(
                f'{page}:{number} claims the scope "{scope}", which is not one of '
                "all, kind=<probe|upstream> or roles=<role>"
            )
            continue
        if not wanted:
            faults.append(
                f'{page}:{number} claims the scope "{scope}", which no registry row '
                "matches - check it against SOLIDSYSLOG_PLATFORM_REGISTRY"
            )
            continue
        scoped = set(wanted)
        for token in wanted:
            unambiguous = {term for term, owners in terms.items() if owners & scoped == {token}}
            if any(names_term(body, term) for term in unambiguous):
                continue
            prose = [t for t in [token, *ALIASES.get(token, [])] if t in unambiguous]
            faults.append(
                f'{page}:{number} lists platforms ("{scope}") but does not name '
                f"{token} - say {' or '.join(prose)}"
            )
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
    faults.extend(pack_role_faults())
    faults.extend(marker_faults())
    faults.extend(list_faults(registry_rows(), vocabulary(rows)))
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
        f"{len(declared_roles())} roles, each listed everywhere roles are enumerated, "
        f"and {len(pack_roles())} declared per platform, each linked from the roles index; "
        f"{sum(1 for _ in marked_lists())} marked platform lists, each complete for its scope"
    )
