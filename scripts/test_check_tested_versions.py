"""Tests for check_tested_versions.py: the platform pages' claims against CI's record."""

import contextlib
import io
import json
import os
import tempfile
import textwrap
import unittest

import check_tested_versions

REGISTRY = textwrap.dedent(
    """\
    set(SOLIDSYSLOG_PLATFORM_REGISTRY
        "Posix|SOLIDSYSLOG_POSIX|x|probe|Platform/Posix|network"
        "MbedTls|SOLIDSYSLOG_MBEDTLS|x|upstream|Platform/MbedTls|tls"
        "LittleFs|SOLIDSYSLOG_LITTLEFS|x|upstream|Platform/LittleFs|file"
        "PlusFat|SOLIDSYSLOG_PLUSFAT|x|upstream|Platform/PlusFat|file"
        "FatFs|SOLIDSYSLOG_FATFS|x|upstream|Platform/FatFs|file"
    )
    """
)


def record(**upstreams):
    return json.dumps(
        {
            "compiler": {"id": "GNU", "version": "12.2.0", "executable": "gcc"},
            "system": {"name": "Linux", "processor": "x86_64", "crossCompiling": False},
            "packs": list(upstreams),
            "bddTarget": None,
            "upstreams": upstreams,
        }
    )


def host_record(compiler_id, compiler_version, packs):
    return json.dumps(
        {
            "compiler": {"id": compiler_id, "version": compiler_version, "executable": "cc"},
            "system": {"name": "Linux", "processor": "x86_64", "crossCompiling": False},
            "packs": packs,
            "bddTarget": None,
            "upstreams": {},
        }
    )


MBEDTLS = {"tree": "MBEDTLS_DIR", "version": "3.6.7", "tag": "mbedtls-3.6.7", "commit": "068ff080b369adfac81509f9b57b2afabaf82dc5"}
LITTLEFS = {"tree": "LITTLEFS_PATH", "version": "2.11", "tag": "v2.11.3", "commit": "6cb4e86540eca0d9ba62500a298385c9d863c8be"}
PLUSFAT = {"tree": "FREERTOS_PLUS_FAT_PATH", "commit": "9ca465c3f2426c301b6b660286f3f964a2abdce5"}


class ATree:
    def __init__(self):
        self.root = tempfile.TemporaryDirectory()
        self.artifacts = os.path.join(self.root.name, "junit")
        self.docs = os.path.join(self.root.name, "docs", "platforms")
        self.registry = os.path.join(self.root.name, "CMakeLists.txt")
        self.write(self.registry, REGISTRY)
        self.page("posix", "A pack over the host's sockets. Nothing upstream.\n")

    def write(self, path, text):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(text)

    def lane(self, name, text):
        self.write(os.path.join(self.artifacts, f"junit-{name}", "build", "debug", "solidsyslog-versions.json"), text)

    def page(self, slug, text):
        self.write(os.path.join(self.docs, slug, "index.md"), text)

    def cleanup(self):
        self.root.cleanup()


class Checking(unittest.TestCase):
    def setUp(self):
        self.tree = ATree()
        self.tree.lane("unit-linux-gcc", record(MbedTls=[MBEDTLS], LittleFs=[LITTLEFS]))
        self.tree.lane("bdd-freertos", record(PlusFat=[PLUSFAT]))

    def tearDown(self):
        self.tree.cleanup()

    def run_check(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = check_tested_versions.main([self.tree.artifacts, "--docs", self.tree.docs, "--registry", self.tree.registry])
        return code, out.getvalue()

    def all_pages_right(self):
        self.tree.page("mbedtls", "The pack is tested against 3.6.7 in CI.\n<!-- tested-against: 3.6.7 -->\n")
        self.tree.page("littlefs", "<!-- tested-against: 2.11.3 -->\nThe pack is tested against 2.11.3.\n")
        self.tree.page("plusfat", "Tested against commit 9ca465c, Plus-FAT having no tagged releases.\n<!-- tested-against: 9ca465c -->\n")

    def test_every_claim_ci_built_against_passes(self):
        self.all_pages_right()
        code, out = self.run_check()
        self.assertEqual(code, 0, out)
        self.assertIn("| MbedTls | 3.6.7 | 3.6.7 (mbedtls-3.6.7 at 068ff08) |", out)

    def test_a_claim_ci_did_not_build_against_fails_naming_what_it_did(self):
        self.all_pages_right()
        self.tree.page("mbedtls", "The pack is tested against 3.6.2 in CI.\n<!-- tested-against: 3.6.2 -->\n")
        code, out = self.run_check()
        self.assertEqual(code, 1)
        self.assertIn("::error::mbedtls/index.md: claims 3.6.2, but CI built MbedTls against 3.6.7 (mbedtls-3.6.7 at 068ff08)", out)

    def test_a_pack_ci_builds_against_an_upstream_must_carry_a_marker(self):
        self.all_pages_right()
        self.tree.page("littlefs", "A LittleFS checkout.\n")
        code, out = self.run_check()
        self.assertEqual(code, 1)
        self.assertIn("::error::littlefs/index.md: CI built LittleFs against 2.11 (v2.11.3 at 6cb4e86), but the page carries no <!-- tested-against: ... --> marker", out)

    def test_a_marker_on_a_pack_no_lane_recorded_fails(self):
        self.all_pages_right()
        self.tree.page("fatfs", "Tested against R0.16.\n<!-- tested-against: R0.16 -->\n")
        code, out = self.run_check()
        self.assertEqual(code, 1)
        self.assertIn("::error::fatfs/index.md: claims R0.16, but no lane records an upstream for FatFs", out)

    def test_the_prose_must_say_what_the_marker_says(self):
        self.all_pages_right()
        self.tree.page("mbedtls", "The pack is tested against the current 3.6 release in CI.\n<!-- tested-against: 3.6.7 -->\n")
        code, out = self.run_check()
        self.assertEqual(code, 1)
        self.assertIn("::error::mbedtls/index.md: the marker says 3.6.7, but the page's prose does not", out)

    def test_a_tree_known_only_by_its_version_defers_to_one_that_also_has_a_tag(self):
        installed = check_tested_versions.model.Upstream("WOLFSSL_DIR", version="5.9.4")
        source = check_tested_versions.model.Upstream("WOLFSSL_SOURCE_DIR", version="5.9.4", tag="v5.9.4-stable", commit="3c5eead44904df64e6a5a1f4ebdce377d35a849a")
        self.assertEqual(check_tested_versions.describe_all([installed, source]), "5.9.4 (v5.9.4-stable at 3c5eead)")
        self.assertEqual(check_tested_versions.describe_all([installed]), "5.9.4")

    def test_a_pack_ci_builds_against_an_upstream_must_have_a_page(self):
        self.all_pages_right()
        os.remove(os.path.join(self.tree.docs, "littlefs", "index.md"))
        code, out = self.run_check()
        self.assertEqual(code, 1)
        self.assertIn("::error::littlefs/index.md: CI built LittleFs against 2.11 (v2.11.3 at 6cb4e86), but there is no such page", out)

    def test_a_page_without_upstreams_needs_no_marker(self):
        self.all_pages_right()
        code, out = self.run_check()
        self.assertEqual(code, 0, out)
        self.assertNotIn("posix", out)

    def test_a_compiler_a_lane_built_the_pack_with_passes(self):
        self.all_pages_right()
        self.tree.lane("unit-linux-clang", host_record("Clang", "19.1.7", ["Posix"]))
        self.tree.page("posix", "Tested with Clang 19.1.7.\n<!-- tested-with: Clang 19.1.7 -->\n")
        code, out = self.run_check()
        self.assertEqual(code, 0, out)
        self.assertIn("| Posix | Clang 19.1.7 | Clang 19.1.7 |", out)

    def test_a_compiler_no_lane_built_the_pack_with_fails_naming_those_that_did(self):
        self.all_pages_right()
        self.tree.lane("unit-linux-gcc-posix", host_record("GNU", "14.2.0", ["Posix"]))
        self.tree.lane("unit-linux-clang", host_record("Clang", "19.1.7", ["Posix"]))
        self.tree.page("posix", "Tested with GNU 12.2.0.\n<!-- tested-with: GNU 12.2.0 -->\n")
        code, out = self.run_check()
        self.assertEqual(code, 1)
        self.assertIn("::error::posix/index.md: claims GNU 12.2.0, but CI built Posix with Clang 19.1.7, GNU 14.2.0", out)

    def test_the_prose_must_say_what_the_tested_with_marker_says(self):
        self.all_pages_right()
        self.tree.lane("unit-linux-clang", host_record("Clang", "19.1.7", ["Posix"]))
        self.tree.page("posix", "Tested with Clang 19.\n<!-- tested-with: Clang 19.1.7 -->\n")
        code, out = self.run_check()
        self.assertEqual(code, 1)
        self.assertIn("::error::posix/index.md: the marker says Clang 19.1.7, but the page's prose does not", out)

    def test_a_tested_with_marker_on_a_pack_no_lane_built_fails(self):
        self.all_pages_right()
        self.tree.page("posix", "Tested with GNU 14.2.0.\n<!-- tested-with: GNU 14.2.0 -->\n")
        code, out = self.run_check()
        self.assertEqual(code, 1)
        self.assertIn("::error::posix/index.md: claims GNU 14.2.0, but no lane built Posix", out)

    def test_no_record_at_all_is_a_missing_input_not_a_stale_page(self):
        self.all_pages_right()
        empty = os.path.join(self.tree.root.name, "nothing")
        os.makedirs(empty)
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = check_tested_versions.main([empty, "--docs", self.tree.docs, "--registry", self.tree.registry])
        self.assertEqual(code, 2)
        self.assertIn("no solidsyslog-versions.json", out.getvalue())


if __name__ == "__main__":
    unittest.main()
