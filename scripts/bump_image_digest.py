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


def bump(root, image, tag, digest):
    short_name = image.rsplit("/", 1)[-1]
    for relative in PINNED_FILES:
        path = os.path.join(root, relative)
        with open(path, encoding="utf-8") as f:
            text = f.read()
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
        text = re.sub(
            r"(# Image digest below is " + re.escape(short_name) + r" )\S+",
            r"\g<1>" + tag,
            text,
        )
        with open(path, "w", encoding="utf-8") as f:
            f.write(text)
