"""Tests for bump_image_digest.py: moving every reference to an image to a new digest."""

import os
import tempfile
import unittest

import bump_image_digest

OLD = "a" * 64
NEW = "b" * 64


class BumpImageDigestTest(unittest.TestCase):
    def setUp(self):
        self.root = tempfile.TemporaryDirectory()
        self.addCleanup(self.root.cleanup)

    def write(self, path, text):
        full = os.path.join(self.root.name, path)
        os.makedirs(os.path.dirname(full), exist_ok=True)
        with open(full, "w", encoding="utf-8") as f:
            f.write(text)

    def read(self, path):
        with open(os.path.join(self.root.name, path), encoding="utf-8") as f:
            return f.read()

    def test_moves_a_trailing_comment_reference_to_the_new_digest_and_tag(self):
        self.write(
            ".devcontainer/docker-compose.yml",
            "    image: ghcr.io/cososo-ltd/cpputest@sha256:" + OLD + " # sha-1111111\n",
        )
        bump_image_digest.bump(self.root.name, "ghcr.io/cososo-ltd/cpputest", "sha-2222222", NEW)
        self.assertEqual(
            "    image: ghcr.io/cososo-ltd/cpputest@sha256:" + NEW + " # sha-2222222\n",
            self.read(".devcontainer/docker-compose.yml"),
        )


if __name__ == "__main__":
    unittest.main()
