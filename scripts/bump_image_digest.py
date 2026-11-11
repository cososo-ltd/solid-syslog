#!/usr/bin/env python3
"""Move every reference to a container image onto a new digest.

Run:  python3 scripts/bump_image_digest.py <image> <tag> <digest>
"""

import os
import re

PINNED_FILES = (
    ".github/workflows/ci.yml",
    ".github/workflows/docs-bundle.yml",
    ".github/workflows/lint-workflows.yml",
    ".devcontainer/docker-compose.yml",
    "ci/docker-compose.bdd.yml",
)

CONTAINERS_DOC = "docs/containers.md"


class BumpError(Exception):
    """The tree is not in a state the bump can be applied to."""


def bump(root, image, tag, digest):
    texts = {relative: _read(root, relative) for relative in PINNED_FILES + (CONTAINERS_DOC,)}
    _check_references_agree(image, texts)
    short_name = image.rsplit("/", 1)[-1]
    for relative in PINNED_FILES:
        texts[relative] = _bump_pinned(texts[relative], image, short_name, tag, digest)
    texts[CONTAINERS_DOC] = _bump_table(texts[CONTAINERS_DOC], image, tag)
    for relative, text in texts.items():
        _write(root, relative, text)


def _check_references_agree(image, texts):
    digests = set()
    for text in texts.values():
        digests.update(re.findall(re.escape(image) + r"@sha256:([0-9a-f]{64})", text))
    if not digests:
        raise BumpError("nothing references " + image)
    if len(digests) > 1:
        raise BumpError(image + " is pinned at more than one digest: " + ", ".join(sorted(digests)))


def _bump_pinned(text, image, short_name, tag, digest):
    text = re.sub(
        re.escape(image) + r"@sha256:[0-9a-f]{64}( # )\S+",
        image + "@sha256:" + digest + r"\g<1>" + tag,
        text,
    )
    text = re.sub(
        re.escape(image) + r"@sha256:[0-9a-f]{64}",
        image + "@sha256:" + digest,
        text,
    )
    return re.sub(
        r"(# Image digest below is " + re.escape(short_name) + r" )\S+",
        r"\g<1>" + tag,
        text,
    )


def _bump_table(text, image, tag):
    return re.sub(
        r"(\| `" + re.escape(image) + r"` \| `)[^`]*`",
        r"\g<1>" + tag + "`",
        text,
    )


def _read(root, relative):
    with open(os.path.join(root, relative), encoding="utf-8") as f:
        return f.read()


def _write(root, relative, text):
    with open(os.path.join(root, relative), "w", encoding="utf-8") as f:
        f.write(text)
