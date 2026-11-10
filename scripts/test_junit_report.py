"""Tests for junit_report.py: reading CI's JUnit artifacts into one model."""

import os
import tempfile
import textwrap
import unittest

import junit_report

CPPUTEST = textwrap.dedent(
    """\
    <?xml version="1.0" encoding="UTF-8" ?>
    <testsuite errors="0" failures="1" hostname="localhost" name="AtomicCounterContract" tests="3" time="0.002" timestamp="2026-11-08T13:34:36">
    <properties>
    </properties>
    <testcase classname="SolidSyslogTests.AtomicCounterContract" name="FirstIncrementReturnsOne" assertions="1" time="0.001" file="/__w/solid-syslog/solid-syslog/Tests/SolidSyslogAtomicCounterContractTest.cpp" line="30">
    </testcase>
    <testcase classname="SolidSyslogTests.AtomicCounterContract" name="IncrementAtMaxWrapsToOne" assertions="1" time="0.000" file="/__w/solid-syslog/solid-syslog/Tests/SolidSyslogAtomicCounterContractTest.cpp" line="91">
    <failure message="/__w/solid-syslog/solid-syslog/Tests/SolidSyslogAtomicCounterContractTest.cpp:93: expected &lt;1&gt; but was &lt;2&gt;" type="AssertionFailedError">
    </failure>
    </testcase>
    <testcase classname="SolidSyslogTests.AtomicCounterContract" name="Pending" assertions="0" time="0.001" file="/__w/solid-syslog/solid-syslog/Tests/SolidSyslogAtomicCounterContractTest.cpp" line="40">
    <skipped />
    </testcase>
    </testsuite>
    """
)

CPPUTEST_UNPACKAGED_WINDOWS = textwrap.dedent(
    """\
    <?xml version="1.0" encoding="UTF-8" ?>
    <testsuite errors="0" failures="0" hostname="localhost" name="PosixClock" tests="1" time="0.000" timestamp="2026-11-08T13:34:36">
    <testcase classname="PosixClock" name="ReadsTheWallClock" assertions="1" time="0.000" file="D:\\a\\solid-syslog\\solid-syslog\\Tests\\SolidSyslogPosixClockTest.cpp" line="12">
    </testcase>
    </testsuite>
    """
)

BEHAVE = textwrap.dedent(
    """\
    <testsuite name="tls_matrix.TLS cipher matrix" tests="3" errors="0" failures="1" skipped="1" time="1.5" timestamp="2026-11-08T13:35:44" hostname="x"><testcase classname="tls_matrix.TLS cipher matrix" name="Server accepts the default cipher" status="passed" time="0.5"><system-out>
    <![CDATA[
    @scenario.begin
      Scenario: Server accepts the default cipher
        Given a thing ... passed in 0.001s
        When it runs ... passed in 0.250s

    @scenario.end
    ]]>
    </system-out></testcase><testcase classname="tls_matrix.TLS cipher matrix" name="Server rejects a weak cipher -- @1.2 " status="skipped" time="0"><skipped /><system-out>
    <![CDATA[
    @scenario.begin
      Scenario Outline: Server rejects a weak cipher -- @1.2
        Given a RC4 ... skipped in 0.000s

    @scenario.end
    ]]>
    </system-out></testcase><testcase classname="tls_matrix.TLS cipher matrix" name="Handshake completes" status="failed" time="1.0"><failure type="AssertionError" message="Assertion Failed: expected 1 message">
    <![CDATA[
    Traceback (most recent call last):
      File "steps.py", line 10, in step_impl
    ]]>
    </failure></testcase></testsuite>
    """
)

FEATURE = textwrap.dedent(
    """\
    @tls @tcp
    Feature: TLS cipher matrix
      Prose about the feature.

      Scenario: Server accepts the default cipher
        Given a thing

      @tls13 @requires_message_size_1500
      Scenario Outline: Server rejects a weak cipher
        Given a <cipher>

        Examples:
          | cipher |
          | RC4    |

      Scenario: Handshake completes
        Given a thing
    """
)

VERSIONS = textwrap.dedent(
    """\
    {
      "compiler": {"id": "GNU", "version": "12.2.1", "executable": "arm-none-eabi-gcc"},
      "system": {"name": "Generic", "processor": "arm", "crossCompiling": true},
      "packs": ["FreeRtos", "MbedTls", "PlusFat"],
      "bddTarget": "FREERTOS_PLUSTCP",
      "upstreams": {
        "MbedTls": [{"tree": "MBEDTLS_DIR", "path": "/opt/mbedtls", "version": "3.6.7", "tag": "mbedtls-3.6.7", "commit": "068ff080b369adfac81509f9b57b2afabaf82dc5"}],
        "FreeRtos": [{"tree": "FREERTOS_KERNEL_PATH", "path": "/opt/freertos/kernel", "version": "V11.3.3", "tag": "V11.3.3", "commit": "89592801dd8b73329a7583719b7e3567b50910e5"}],
        "PlusFat": [{"tree": "FREERTOS_PLUS_FAT_PATH", "path": "/opt/freertos/plus-fat", "commit": "9ca465c3f2426c301b6b660286f3f964a2abdce5"}]
      }
    }
    """
)

REGISTRY = textwrap.dedent(
    """\
    set(SOLIDSYSLOG_PLATFORM_REGISTRY
        "Posix|platform|Posix|x|Platform/Posix|y"
        "LwipRaw|platform|lwIP (Raw API)|x|Platform/LwipRaw|y"
        "LwipSocket|platform|lwIP (Sockets API)|x|Platform/LwipSocket|y"
        "MbedTls|platform|Mbed TLS|x|Platform/MbedTls|y"
    )
    """
)


class ATree:
    """A temporary artifact tree: junit-<lane>/<files>, plus a features dir."""

    def __init__(self):
        self.root = tempfile.TemporaryDirectory()

    def write(self, relative, text):
        path = os.path.join(self.root.name, relative)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(text)
        return path

    def cleanup(self):
        self.root.cleanup()


class ReadingCppUTest(unittest.TestCase):
    def setUp(self):
        self.tree = ATree()
        self.tree.write("junit-unit-linux-gcc/cpputest_SolidSyslogTests_AtomicCounterContract.xml", CPPUTEST)
        self.run = junit_report.read_run(self.tree.root.name, features_dir=None)
        self.lane = self.run.lanes["unit-linux-gcc"]

    def tearDown(self):
        self.tree.cleanup()

    def test_the_lane_takes_its_name_and_kind_from_the_artifact(self):
        self.assertEqual(self.lane.name, "unit-linux-gcc")
        self.assertEqual(self.lane.kind, "unit")

    def test_each_case_has_status_time_file_and_line(self):
        first = self.lane.tests[0]
        self.assertEqual((first.suite, first.name, first.status, first.time), ("AtomicCounterContract", "FirstIncrementReturnsOne", "passed", 0.001))
        self.assertEqual((first.file, first.line), ("Tests/SolidSyslogAtomicCounterContractTest.cpp", 30))

    def test_the_executable_is_the_classname_package(self):
        self.assertEqual({t.executable for t in self.lane.tests}, {"SolidSyslogTests"})

    def test_a_failure_carries_its_message(self):
        failed = [t for t in self.lane.tests if t.status == "failed"]
        self.assertEqual(len(failed), 1)
        self.assertEqual(failed[0].name, "IncrementAtMaxWrapsToOne")
        self.assertIn("expected <1> but was <2>", failed[0].message)

    def test_a_skip_is_a_skip(self):
        self.assertEqual([t.name for t in self.lane.tests if t.status == "skipped"], ["Pending"])

    def test_counts_sum_the_lane(self):
        self.assertEqual((self.lane.counts.total, self.lane.counts.passed, self.lane.counts.failed, self.lane.counts.skipped), (3, 1, 1, 1))
        self.assertAlmostEqual(self.lane.counts.time, 0.002)


class ReadingUnpackagedAndWindows(unittest.TestCase):
    def setUp(self):
        self.tree = ATree()
        self.tree.write("junit-unit-windows-msvc/cpputest_PosixClock.xml", CPPUTEST_UNPACKAGED_WINDOWS)
        self.tree.write("junit-unit-linux-gcc/vxworks64/cpputest_PosixClock.xml", CPPUTEST_UNPACKAGED_WINDOWS)
        self.run = junit_report.read_run(self.tree.root.name, features_dir=None)

    def tearDown(self):
        self.tree.cleanup()

    def test_a_windows_path_is_made_repo_relative_with_forward_slashes(self):
        test = self.run.lanes["unit-windows-msvc"].tests[0]
        self.assertEqual(test.file, "Tests/SolidSyslogPosixClockTest.cpp")

    def test_a_case_without_a_package_is_attributed_to_its_artifact_subdirectory(self):
        self.assertEqual(self.run.lanes["unit-windows-msvc"].tests[0].executable, "unnamed executable")
        self.assertEqual(self.run.lanes["unit-linux-gcc"].tests[0].executable, "unnamed executable (vxworks64)")


class ReadingBehave(unittest.TestCase):
    def setUp(self):
        self.tree = ATree()
        self.tree.write("junit-bdd-linux-openssl-syslogng/TESTS-tls_matrix.xml", BEHAVE)
        self.tree.write("features/tls_matrix.feature", FEATURE)
        self.run = junit_report.read_run(self.tree.root.name, features_dir=os.path.join(self.tree.root.name, "features"))
        self.lane = self.run.lanes["bdd-linux-openssl-syslogng"]

    def tearDown(self):
        self.tree.cleanup()

    def test_the_feature_is_the_suite_and_scenarios_are_the_tests(self):
        self.assertEqual(self.lane.kind, "bdd")
        self.assertEqual({t.suite for t in self.lane.tests}, {"TLS cipher matrix"})
        self.assertEqual([t.name for t in self.lane.tests][0], "Server accepts the default cipher")

    def test_a_scenario_carries_the_feature_tags_and_its_own(self):
        by_name = {t.name: t for t in self.lane.tests}
        self.assertEqual(by_name["Server accepts the default cipher"].tags, ("tcp", "tls"))
        self.assertEqual(by_name["Server rejects a weak cipher -- @1.2 "].tags, ("requires_message_size_1500", "tcp", "tls", "tls13"))

    def test_a_scenario_knows_its_feature_file(self):
        self.assertEqual(self.lane.tests[0].file, "Bdd/features/tls_matrix.feature")

    def test_a_scenario_carries_its_steps_with_their_outcome_and_time(self):
        first = self.lane.tests[0]
        self.assertEqual([(s.keyword, s.text, s.status, s.time) for s in first.steps], [("Given", "a thing", "passed", 0.001), ("When", "it runs", "passed", 0.25)])
        self.assertEqual([s.status for s in self.lane.tests[1].steps], ["skipped"])

    def test_the_run_knows_each_features_name_and_prose(self):
        feature = self.run.features["tls_matrix"]
        self.assertEqual(feature.name, "TLS cipher matrix")
        self.assertEqual(feature.description, "Prose about the feature.")
        self.assertEqual(feature.feature_tags, ("tcp", "tls"))
        self.assertEqual(self.lane.tests[0].feature_stem, "tls_matrix")

    def test_statuses_follow_the_markup(self):
        self.assertEqual([t.status for t in self.lane.tests], ["passed", "skipped", "failed"])
        self.assertIn("expected 1 message", [t for t in self.lane.tests if t.status == "failed"][0].message)


class Packs(unittest.TestCase):
    def setUp(self):
        self.tokens = junit_report.registry_tokens(REGISTRY)

    def test_tokens_come_from_the_registry_in_order(self):
        self.assertEqual(self.tokens, ["Posix", "LwipRaw", "LwipSocket", "MbedTls"])

    def test_a_root_test_file_named_for_a_pack_belongs_to_it(self):
        self.assertEqual(junit_report.pack_of("Tests/SolidSyslogPosixClockTest.cpp", self.tokens), "Posix")

    def test_the_longest_token_wins_over_a_prefix_of_it(self):
        self.assertEqual(junit_report.pack_of("Tests/Lwip/SolidSyslogLwipRawDatagramTest.cpp", self.tokens), "LwipRaw")
        self.assertEqual(junit_report.pack_of("Tests/LwipSocket/SolidSyslogLwipSocketTcpStreamTest.cpp", self.tokens), "LwipSocket")

    def test_a_directory_named_for_a_pack_claims_what_its_files_do_not(self):
        self.assertEqual(junit_report.pack_of("Tests/MbedTlsIntegration/SocketStreamContractTest.cpp", self.tokens), "MbedTls")

    def test_a_fake_test_is_test_support_and_the_rest_is_core(self):
        self.assertEqual(junit_report.pack_of("Tests/BufferFakeTest.cpp", self.tokens), "Test support")
        self.assertEqual(junit_report.pack_of("Tests/SolidSyslogFormatterTest.cpp", self.tokens), "Core")

    def test_the_bdd_target_tests_and_scenarios_are_their_own_group(self):
        self.assertEqual(junit_report.pack_of("Tests/Bdd/Targets/BddTargetAppNameTest.cpp", self.tokens), "BDD target")
        self.assertEqual(junit_report.pack_of("Bdd/features/tls_matrix.feature", self.tokens), "BDD scenarios")


class ReadingTheVersionRecord(unittest.TestCase):
    def setUp(self):
        self.tree = ATree()
        self.tree.write("junit-bdd-freertos/build/freertos-cross/solidsyslog-versions.json", VERSIONS)
        self.tree.write("junit-bdd-freertos/Bdd/junit/TESTS-tls.xml", BEHAVE)
        self.tree.write("junit-unit-linux-gcc/cpputest_SolidSyslogTests_AtomicCounterContract.xml", CPPUTEST)
        self.run = junit_report.read_run(self.tree.root.name, features_dir=None)
        self.versions = self.run.lanes["bdd-freertos"].versions

    def tearDown(self):
        self.tree.cleanup()

    def test_the_record_is_found_wherever_the_artifact_holds_it(self):
        self.assertEqual((self.versions.compiler_id, self.versions.compiler_version, self.versions.compiler_executable), ("GNU", "12.2.1", "arm-none-eabi-gcc"))
        self.assertEqual((self.versions.system, self.versions.cross_compiling), ("Generic arm", True))
        self.assertEqual(self.versions.packs, ("FreeRtos", "MbedTls", "PlusFat"))
        self.assertEqual(self.versions.bdd_target, "FREERTOS_PLUSTCP")

    def test_each_upstream_carries_what_the_tree_said_about_itself(self):
        mbedtls = self.versions.upstreams["MbedTls"][0]
        self.assertEqual((mbedtls.tree, mbedtls.version, mbedtls.tag), ("MBEDTLS_DIR", "3.6.7", "mbedtls-3.6.7"))
        self.assertEqual(mbedtls.commit, "068ff080b369adfac81509f9b57b2afabaf82dc5")
        self.assertIsNone(self.versions.upstreams["PlusFat"][0].version)

    def test_an_upstream_is_labelled_by_its_version_then_its_tag_then_its_commit(self):
        self.assertEqual(self.versions.upstreams["MbedTls"][0].label, "3.6.7")
        self.assertEqual(junit_report.Upstream("FATFS_PATH", tag="R0.16", commit="30ca13c62615df0d2e9104ab41256985b96590c1").label, "R0.16")
        self.assertEqual(self.versions.upstreams["PlusFat"][0].label, "9ca465c")

    def test_an_upstream_answers_to_its_version_its_tag_with_or_without_the_v_and_its_commit(self):
        upstream = junit_report.Upstream("LITTLEFS_PATH", version="2.11", tag="v2.11.3", commit="6cb4e86540eca0d9ba62500a298385c9d863c8be")
        for claim in ["2.11", "v2.11.3", "2.11.3", "6cb4e86", "6cb4e86540eca0d9ba62500a298385c9d863c8be"]:
            self.assertTrue(upstream.answers_to(claim), claim)
        for claim in ["2.11.4", "6cb4e8", "V2.11.3", ""]:
            self.assertFalse(upstream.answers_to(claim), claim)

    def test_a_lane_without_a_record_has_no_versions(self):
        self.assertIsNone(self.run.lanes["unit-linux-gcc"].versions)


class Kinds(unittest.TestCase):
    def test_the_kind_is_the_lane_names_first_word(self):
        for lane, kind in [("unit-linux-gcc", "unit"), ("integration-linux-mbedtls", "integration"), ("bdd-windows-openssl-otel", "bdd"), ("build-linux-wolfssl", "build")]:
            self.assertEqual(junit_report.kind_of(lane), kind)


if __name__ == "__main__":
    unittest.main()
