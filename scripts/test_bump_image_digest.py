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
        for relative in bump_image_digest.PINNED_FILES + (bump_image_digest.CONTAINERS_DOC,):
            self.write(relative, "")

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

    def test_moves_every_pinned_file_and_leaves_other_images_alone(self):
        self.write(".github/workflows/ci.yml", "      image: ghcr.io/cososo-ltd/cpputest@sha256:" + OLD + " # sha-1111111\n")
        self.write(".github/workflows/docs-bundle.yml", "      image: ghcr.io/cososo-ltd/cpputest-freertos@sha256:" + OLD + " # sha-1111111\n")
        self.write(".github/workflows/lint-workflows.yml", "ACTIONLINT_IMAGE: rhysd/actionlint@sha256:" + OLD + " # 1.7.7\n")
        self.write(".devcontainer/docker-compose.yml", "    image: ghcr.io/cososo-ltd/cpputest@sha256:" + OLD + " # sha-1111111\n")
        self.write("ci/docker-compose.bdd.yml", "    image: ghcr.io/cososo-ltd/cpputest@sha256:" + OLD + " # sha-1111111\n")
        bump_image_digest.bump(self.root.name, "ghcr.io/cososo-ltd/cpputest", "sha-2222222", NEW)
        self.assertIn(NEW + " # sha-2222222", self.read(".github/workflows/ci.yml"))
        self.assertIn(NEW + " # sha-2222222", self.read("ci/docker-compose.bdd.yml"))
        self.assertIn("cpputest-freertos@sha256:" + OLD + " # sha-1111111", self.read(".github/workflows/docs-bundle.yml"))
        self.assertIn("actionlint@sha256:" + OLD + " # 1.7.7", self.read(".github/workflows/lint-workflows.yml"))

    def test_moves_a_reference_whose_tag_comment_sits_on_the_line_above(self):
        self.write(
            ".github/workflows/ci.yml",
            "      # Image digest below is mkdocs-mkdoxy sha-1111111 (see docs/containers.md).\n"
            "      - name: Test docs build hooks\n"
            "        run: |\n"
            "          docker run --rm -v \"$PWD:/docs\" \\\n"
            "            ghcr.io/cososo-ltd/mkdocs-mkdoxy@sha256:" + OLD + " \\\n"
            "            python3 -m unittest discover -s /docs/hooks -p 'test_*.py'\n",
        )
        bump_image_digest.bump(self.root.name, "ghcr.io/cososo-ltd/mkdocs-mkdoxy", "sha-2222222", NEW)
        self.assertEqual(
            "      # Image digest below is mkdocs-mkdoxy sha-2222222 (see docs/containers.md).\n"
            "      - name: Test docs build hooks\n"
            "        run: |\n"
            "          docker run --rm -v \"$PWD:/docs\" \\\n"
            "            ghcr.io/cososo-ltd/mkdocs-mkdoxy@sha256:" + NEW + " \\\n"
            "            python3 -m unittest discover -s /docs/hooks -p 'test_*.py'\n",
            self.read(".github/workflows/ci.yml"),
        )

    def test_moves_the_tag_in_the_containers_table(self):
        self.write(".devcontainer/docker-compose.yml", "    image: ghcr.io/cososo-ltd/cpputest@sha256:" + OLD + " # sha-1111111\n")
        self.write(
            "docs/containers.md",
            "| Image | Tag | What it is for |\n"
            "|---|---|---|\n"
            "| `ghcr.io/cososo-ltd/cpputest` | `sha-1111111` | The GCC base |\n"
            "| `ghcr.io/cososo-ltd/cpputest-clang` | `sha-1111111` | The Clang counterpart |\n",
        )
        bump_image_digest.bump(self.root.name, "ghcr.io/cososo-ltd/cpputest", "sha-2222222", NEW)
        self.assertEqual(
            "| Image | Tag | What it is for |\n"
            "|---|---|---|\n"
            "| `ghcr.io/cososo-ltd/cpputest` | `sha-2222222` | The GCC base |\n"
            "| `ghcr.io/cososo-ltd/cpputest-clang` | `sha-1111111` | The Clang counterpart |\n",
            self.read("docs/containers.md"),
        )

    def test_refuses_when_the_references_do_not_already_agree(self):
        self.write(".devcontainer/docker-compose.yml", "    image: ghcr.io/cososo-ltd/cpputest@sha256:" + OLD + " # sha-1111111\n")
        self.write("ci/docker-compose.bdd.yml", "    image: ghcr.io/cososo-ltd/cpputest@sha256:" + "c" * 64 + " # sha-3333333\n")
        with self.assertRaises(bump_image_digest.BumpError):
            bump_image_digest.bump(self.root.name, "ghcr.io/cososo-ltd/cpputest", "sha-2222222", NEW)
        self.assertIn(OLD, self.read(".devcontainer/docker-compose.yml"))

    def test_refuses_an_image_nothing_references(self):
        self.write(".devcontainer/docker-compose.yml", "    image: ghcr.io/cososo-ltd/cpputest@sha256:" + OLD + " # sha-1111111\n")
        with self.assertRaises(bump_image_digest.BumpError):
            bump_image_digest.bump(self.root.name, "ghcr.io/cososo-ltd/behave", "sha-2222222", NEW)


if __name__ == "__main__":
    unittest.main()
