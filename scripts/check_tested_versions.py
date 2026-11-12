#!/usr/bin/env python3
"""Hold each platform page's "tested against" claim to what CI built.

SUPPORT.md defines supported as built and tested in CI against the version a
pack's page names. Every lane's configure records what it compiled against -
the compiler, and the version each selected pack's upstream states in its own
header, with the tag and commit of the tree where it is a git checkout
(cmake/VersionRecord.cmake) - and uploads the record beside its JUnit results.
This reads those records out of a run's ``junit-*`` artifacts and compares them
with the claim each page under ``docs/platforms/`` makes in a marker:

    <!-- tested-against: 3.6.7 -->

A claim is backed when some lane's record for that pack answers to it: the
header's version, the tree's tag with or without a leading ``v``, or its commit
by at least the first seven characters, which is how a tree with no releases
is named. The rules:

* A pack some lane built against an upstream has a page, and that page
  carries at least one marker, so there is a claim to check.
* Every marker names something a lane built against.
* A marker on a pack no lane recorded an upstream for is a claim CI cannot
  back.
* The page's prose says what the marker says, so the sentence a reader sees
  cannot drift from the value the check reads.

A pack with no upstream - the host packs - needs no marker. A platform is
found by its registry token lower-cased, as the docs folders are named.

Run:  python3 scripts/check_tested_versions.py build/junit

Exit code:
    0  every claim is backed
    1  a rule above failed
    2  no record was found: download a run's junit-* artifacts first
"""

import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import junit_report as model  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MARKER = re.compile(r"<!--\s*tested-against:\s*([^\s>]+)\s*-->")
COMPILER_MARKER = re.compile(r"<!--\s*tested-with:\s*([^>]*?)\s*-->")
COMMENT = re.compile(r"<!--.*?-->", re.DOTALL)


def read_claims(page_path):
    """The marker values on a page, and its prose with the comments removed."""
    with open(page_path, encoding="utf-8") as handle:
        text = handle.read()
    return MARKER.findall(text), COMMENT.sub("", text)


def recorded_compilers(run):
    """Every compiler any lane built each pack with, by pack, each kept once."""
    by_pack = {}
    for lane in run.lanes.values():
        if lane.versions is None:
            continue
        for pack in lane.versions.packs:
            seen = by_pack.setdefault(pack, [])
            if lane.versions.compiler not in seen:
                seen.append(lane.versions.compiler)
    return by_pack


def recorded_upstreams(run):
    """Every upstream any lane recorded, by pack, each kept once."""
    by_pack = {}
    for lane in run.lanes.values():
        if lane.versions is None:
            continue
        for pack, upstreams in lane.versions.upstreams.items():
            seen = by_pack.setdefault(pack, [])
            for upstream in upstreams:
                if upstream not in seen:
                    seen.append(upstream)
    return by_pack


def describe(upstream):
    """One upstream as the failure names it: label, then tag at commit."""
    short = (upstream.commit or "")[: model.SHORT_COMMIT]
    detail = []
    if upstream.tag and upstream.tag != upstream.label:
        detail.append(upstream.tag)
    if short and short != upstream.label:
        detail.append(f"at {short}" if detail else short)
    return f"{upstream.label} ({' '.join(detail)})" if detail else upstream.label


def describe_all(upstreams):
    """Every distinct tree, a bare version standing aside for a tree that has
    the same version and a tag or commit too."""
    detailed = {u.label for u in upstreams if describe(u) != u.label}
    descriptions = []
    for upstream in upstreams:
        text = describe(upstream)
        if text == upstream.label and upstream.label in detailed:
            continue
        if text not in descriptions:
            descriptions.append(text)
    return ", ".join(descriptions)


def check(tokens, recorded, docs_dir):
    """Apply the rules. Returns (failures, summary rows)."""
    failures = []
    rows = []
    for token in tokens:
        page = os.path.join(docs_dir, token.lower(), "index.md")
        relative = f"{token.lower()}/index.md"
        upstreams = recorded.get(token, [])
        if not os.path.exists(page):
            if upstreams:
                failures.append(f"{relative}: CI built {token} against {describe_all(upstreams)}, but there is no such page")
                rows.append((token, "-", describe_all(upstreams)))
            continue
        claims, prose = read_claims(page)
        if upstreams and not claims:
            failures.append(
                f"{relative}: CI built {token} against {describe_all(upstreams)}, "
                "but the page carries no <!-- tested-against: ... --> marker"
            )
        for claim in claims:
            if not upstreams:
                failures.append(f"{relative}: claims {claim}, but no lane records an upstream for {token}")
            elif not any(upstream.answers_to(claim) for upstream in upstreams):
                failures.append(f"{relative}: claims {claim}, but CI built {token} against {describe_all(upstreams)}")
            if claim not in prose:
                failures.append(f"{relative}: the marker says {claim}, but the page's prose does not")
        if upstreams or claims:
            rows.append((token, ", ".join(claims) or "-", describe_all(upstreams) or "-"))
    return failures, rows


def check_compilers(tokens, compilers, docs_dir):
    """The tested-with rules. Returns (failures, summary rows)."""
    failures = []
    rows = []
    for token in tokens:
        page = os.path.join(docs_dir, token.lower(), "index.md")
        if not os.path.exists(page):
            continue
        with open(page, encoding="utf-8") as handle:
            text = handle.read()
        claims = COMPILER_MARKER.findall(text)
        prose = COMMENT.sub("", text)
        built = sorted(compilers.get(token, []))
        for claim in claims:
            if not built:
                failures.append(f"{token.lower()}/index.md: claims {claim}, but no lane built {token}")
            elif claim not in built:
                failures.append(f"{token.lower()}/index.md: claims {claim}, but CI built {token} with {', '.join(built)}")
            if claim not in prose:
                failures.append(f"{token.lower()}/index.md: the marker says {claim}, but the page's prose does not")
        if claims:
            rows.append((token, ", ".join(claims), ", ".join(built) or "-"))
    return failures, rows


def summary(rows):
    lines = ["| Pack | Page claims | CI built against |", "|---|---|---|"]
    lines.extend(f"| {token} | {claimed} | {built} |" for token, claimed, built in rows)
    return "\n".join(lines) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("artifacts", help="a directory of junit-<lane> artifacts, as download-artifact leaves them")
    parser.add_argument("--docs", default=os.path.join(ROOT, "docs", "platforms"), help="the platform pages")
    parser.add_argument("--registry", default=os.path.join(ROOT, "CMakeLists.txt"), help="the CMakeLists.txt holding the platform registry")
    arguments = parser.parse_args(argv)

    with open(arguments.registry, encoding="utf-8") as handle:
        tokens = model.registry_tokens(handle.read())
    run = model.read_run(arguments.artifacts, features_dir=None)
    recorded = recorded_upstreams(run)
    if not any(lane.versions for lane in run.lanes.values()):
        print(f"no solidsyslog-versions.json under {arguments.artifacts}: download a run's junit-* artifacts first")
        return 2

    failures, rows = check(tokens, recorded, arguments.docs)
    compiler_failures, compiler_rows = check_compilers(tokens, recorded_compilers(run), arguments.docs)
    failures += compiler_failures
    rows += compiler_rows
    text = summary(rows)
    print(text)
    step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if step_summary:
        with open(step_summary, "a", encoding="utf-8") as handle:
            handle.write("### Platform pages against CI's version record\n\n" + text)
    for failure in failures:
        print(f"::error::{failure}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
