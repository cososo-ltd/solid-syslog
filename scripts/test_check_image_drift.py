"""Tests for check_image_drift.py: a pin that lags its image repository with no bump pull request."""

import contextlib
import io
import os
import tempfile
import unittest

import check_image_drift

HEAD = "1234567" + "8" * 33
OTHER = "abcdef0" + "1" * 33


def table(cpputest="sha-1234567", freertos="sha-1234567", cross="sha-1234567"):
    return (
        "| Image | Tag | Use |\n"
        "|---|---|---|\n"
        "| `ghcr.io/cososo-ltd/cpputest` | `" + cpputest + "` | The GCC base |\n"
        "| `ghcr.io/cososo-ltd/cpputest-clang` | `sha-5905aea` | The Clang counterpart |\n"
        "| `ghcr.io/cososo-ltd/cpputest-freertos` | `" + freertos + "` | Host-TDD |\n"
        "| `ghcr.io/cososo-ltd/cpputest-freertos-cross` | `" + cross + "` | Cross |\n"
    )


ALL_AT_HEAD = {"cososo-ltd/CppUTestDocker": HEAD, "cososo-ltd/CppUTestFreertosDocker": HEAD}


class CheckImageDriftTest(unittest.TestCase):
    def test_a_pin_at_its_repository_head_is_current(self):
        self.assertEqual([], check_image_drift.check(table(), ALL_AT_HEAD, []))

    def test_a_pin_behind_its_repository_head_with_no_pull_request_lags(self):
        heads = dict(ALL_AT_HEAD, **{"cososo-ltd/CppUTestDocker": OTHER})
        self.assertEqual(
            [
                "ghcr.io/cososo-ltd/cpputest is pinned at sha-1234567, but cososo-ltd/CppUTestDocker main"
                " publishes sha-abcdef0 and no open bump pull request carries it"
            ],
            check_image_drift.check(table(), heads, []),
        )

    def test_a_lagging_pin_carried_by_an_open_bump_pull_request_is_not_drift(self):
        heads = dict(ALL_AT_HEAD, **{"cososo-ltd/CppUTestDocker": OTHER})
        branches = ["chore/bump-image-sha-abcdef0-cpputest"]
        self.assertEqual([], check_image_drift.check(table(), heads, branches))

    def test_a_bump_pull_request_for_an_older_tag_does_not_carry_the_head(self):
        heads = dict(ALL_AT_HEAD, **{"cososo-ltd/CppUTestDocker": OTHER})
        branches = ["chore/bump-image-sha-7777777-cpputest"]
        self.assertEqual(1, len(check_image_drift.check(table(), heads, branches)))

    def test_one_repository_publishing_two_images_is_checked_for_each(self):
        heads = dict(ALL_AT_HEAD, **{"cososo-ltd/CppUTestFreertosDocker": OTHER})
        problems = check_image_drift.check(table(), heads, [])
        self.assertEqual(2, len(problems))
        self.assertIn("ghcr.io/cososo-ltd/cpputest-freertos is pinned", problems[0])
        self.assertIn("ghcr.io/cososo-ltd/cpputest-freertos-cross is pinned", problems[1])

    def test_an_image_whose_repository_does_not_send_the_event_is_not_checked(self):
        self.assertNotIn("ghcr.io/cososo-ltd/cpputest-clang", check_image_drift.SOURCES)

    def test_an_image_missing_from_the_table_is_reported(self):
        text = table().replace("| `ghcr.io/cososo-ltd/cpputest` | `sha-1234567` | The GCC base |\n", "")
        self.assertEqual(
            ["ghcr.io/cososo-ltd/cpputest has no row in docs/containers.md"],
            check_image_drift.check(text, ALL_AT_HEAD, []),
        )

    def test_a_repository_whose_head_was_not_given_is_reported(self):
        heads = {"cososo-ltd/CppUTestDocker": HEAD}
        problems = check_image_drift.check(table(), heads, [])
        self.assertEqual(
            ["no main head given for cososo-ltd/CppUTestFreertosDocker"] * 2,
            problems,
        )


class MainTest(unittest.TestCase):
    def setUp(self):
        self.root = tempfile.TemporaryDirectory()
        self.addCleanup(self.root.cleanup)
        os.makedirs(os.path.join(self.root.name, "docs"))

    def write_table(self, text):
        with open(os.path.join(self.root.name, check_image_drift.CONTAINERS_DOC), "w", encoding="utf-8") as f:
            f.write(text)

    def run_main(self, argv):
        out = io.StringIO()
        err = io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            status = check_image_drift.main(argv + ["--root", self.root.name])
        return status, out.getvalue(), err.getvalue()

    def test_lists_the_repositories_whose_heads_it_needs(self):
        status, out, _ = self.run_main(["--repositories"])
        self.assertEqual(0, status)
        self.assertEqual("cososo-ltd/CppUTestDocker\ncososo-ltd/CppUTestFreertosDocker\n", out)

    def test_passes_when_every_pin_is_current(self):
        self.write_table(table())
        status, out, _ = self.run_main(
            ["--head", "cososo-ltd/CppUTestDocker=" + HEAD, "--head", "cososo-ltd/CppUTestFreertosDocker=" + HEAD]
        )
        self.assertEqual(0, status)
        self.assertIn("ghcr.io/cososo-ltd/cpputest sha-1234567", out)

    def test_fails_and_says_why_when_a_pin_lags(self):
        self.write_table(table())
        status, _, err = self.run_main(
            [
                "--head",
                "cososo-ltd/CppUTestDocker=" + OTHER,
                "--head",
                "cososo-ltd/CppUTestFreertosDocker=" + HEAD,
                "--open-branch",
                "release-please--branches--main",
            ]
        )
        self.assertEqual(1, status)
        self.assertIn("check_image_drift: ghcr.io/cososo-ltd/cpputest is pinned at sha-1234567", err)


if __name__ == "__main__":
    unittest.main()
