"""Tests for what a run reads from the board and its build (nucleo_f767zi.py).

Run:  python -m unittest discover -s Bdd/Targets/Boards/NucleoF767zi/Runner -p 'test_*.py'
"""

import os
import sys
import time
import unittest
import unittest.mock

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import nucleo_f767zi  # noqa: E402
import serial_console  # noqa: E402
from test_serial_console import FakePort  # noqa: E402


BANNER = "\r\nSolidSyslog BDD target on NUCLEO-F767ZI, commit 0123456789\r\n[board] link up\r\nSolidSyslog> "


class BannerTest(unittest.TestCase):
    def test_an_image_built_from_the_commit_under_test_is_accepted(self):
        nucleo_f767zi.check_image_is_of_commit(BANNER, "0123456789abcdef0123456789abcdef01234567")

    def test_an_image_built_from_another_commit_is_refused(self):
        with self.assertRaisesRegex(RuntimeError, "0123456789.*fedcba9876"):
            nucleo_f767zi.check_image_is_of_commit(BANNER, "fedcba9876543210fedcba9876543210fedcba98")

    # The build marks a tree with changes its commit does not hold.
    def test_an_image_built_from_a_changed_tree_is_refused(self):
        dirty = BANNER.replace("0123456789", "0123456789-dirty")

        with self.assertRaisesRegex(RuntimeError, "0123456789-dirty"):
            nucleo_f767zi.check_image_is_of_commit(dirty, "0123456789abcdef0123456789abcdef01234567")

    def test_a_board_that_shows_no_banner_is_refused(self):
        with self.assertRaisesRegex(RuntimeError, "no banner"):
            nucleo_f767zi.check_image_is_of_commit("SolidSyslog> ", "0123456789abcdef0123456789abcdef01234567")


class OutputToPromptTest(unittest.TestCase):
    def setUp(self):
        self.port = FakePort()
        self.console = serial_console.SerialConsole(self.port, lambda: None)
        self.addCleanup(self.console.kill)

    def test_everything_the_board_shows_up_to_its_prompt_is_returned(self):
        self.port.arrived.put(b"banner\r\nSolid")
        self.port.arrived.put(b"Syslog> ")

        self.assertEqual("banner\r\nSolidSyslog> ", nucleo_f767zi.output_to_prompt(self.console, 5))

    def test_a_board_that_never_shows_its_prompt_is_refused_with_what_it_did_show(self):
        self.port.arrived.put(b"[board] link down\r\n")

        with self.assertRaisesRegex(TimeoutError, "link down"):
            nucleo_f767zi.output_to_prompt(self.console, 0.2)


# The ST-LINK keeps what the board printed while nobody had the port open, and
# hands it over as the port opens.
class DiscardTest(unittest.TestCase):
    def test_what_the_port_held_before_the_reset_is_discarded(self):
        port = FakePort()
        port.arrived.put(b"commit 0000000000\r\n")
        port.arrived.put(b"SolidSyslog> ")

        nucleo_f767zi.discard_until_quiet(port, 0.05, 5)

        self.assertTrue(port.arrived.empty())

    def test_a_quiet_port_is_left_once_it_has_been_quiet_for_the_time_given(self):
        port = FakePort()
        started = time.monotonic()

        nucleo_f767zi.discard_until_quiet(port, 0.05, 5)

        self.assertGreaterEqual(time.monotonic() - started, 0.05)

    def test_a_board_that_never_falls_quiet_is_refused(self):
        port = FakePort()
        port.read = lambda: b"tick\r\n"

        with self.assertRaisesRegex(RuntimeError, "not been quiet"):
            nucleo_f767zi.discard_until_quiet(port, 0.05, 0.2)


# The banner is the first thing a reset draws, so a board that has not shown
# one soon after a reset has not started.
class ResetTest(unittest.TestCase):
    def setUp(self):
        self.port = FakePort()
        self.resets = 0
        self.reports = []
        self.starts_from_reset = 1
        self.shown_at_a_reset = [b"\r\nSolidSyslog BDD target on NUCLEO"]

    def reset(self):
        self.resets += 1
        if self.resets >= self.starts_from_reset:
            for shown in self.shown_at_a_reset:
                self.port.arrived.put(shown)
        return "MCU Reset"

    def reset_until_banner(self):
        return nucleo_f767zi.reset_until_banner(self.port, self.reset, 0.05, 3, self.reports.append)

    def test_a_board_that_starts_is_reset_once_and_nothing_is_reported(self):
        self.reset_until_banner()

        self.assertEqual(1, self.resets)
        self.assertEqual([], self.reports)

    def test_what_the_board_showed_from_its_banner_on_is_returned(self):
        self.shown_at_a_reset = [b"\r\nSolidSyslog BDD tar", b"get on NUCLEO\r\n[board] link down\r\n"]

        self.assertEqual(b"SolidSyslog BDD target on NUCLEO\r\n[board] link down\r\n", self.reset_until_banner())

    # The boot before this one can still be printing as the reset is asked
    # for, and its prompt would be taken for this boot's.
    def test_what_the_boot_before_showed_ahead_of_the_banner_is_discarded(self):
        self.shown_at_a_reset = [b"[board] link up\r\nSolidSyslog> ", b"\r\nSolidSyslog BDD target on NUCLEO"]

        self.assertEqual(b"SolidSyslog BDD target on NUCLEO", self.reset_until_banner())

    def test_a_board_that_shows_no_banner_is_reset_again_and_reported(self):
        self.starts_from_reset = 2

        self.reset_until_banner()

        self.assertEqual(2, self.resets)
        self.assertEqual(1, len(self.reports))
        self.assertRegex(self.reports[0], "showed no banner.*reset 1.*MCU Reset")

    def test_the_report_holds_what_the_board_showed_instead(self):
        self.port.arrived.put(b"SolidSyslog> ")
        self.starts_from_reset = 2

        self.reset_until_banner()

        self.assertRegex(self.reports[0], "It showed b'SolidSyslog> '")

    def test_a_board_that_never_shows_a_banner_is_refused(self):
        self.starts_from_reset = 99

        with self.assertRaisesRegex(RuntimeError, "3 resets"):
            self.reset_until_banner()

        self.assertEqual(3, self.resets)


class ConsoleAfterResetTest(unittest.TestCase):
    def setUp(self):
        self.port = FakePort()
        self.shown_at_reset = b"SolidSyslog BDD target on NUCLEO\r\n"
        waits = unittest.mock.patch.multiple(
            nucleo_f767zi, _QUIET_SECONDS=0.05, _BANNER_WAIT_SECONDS=0.05, _RESET_ATTEMPTS=1
        )
        waits.start()
        self.addCleanup(waits.stop)

    def reset(self):
        self.port.arrived.put(self.shown_at_reset)
        return "MCU Reset"

    def test_the_console_starts_with_the_banner_the_reset_drew(self):
        console = nucleo_f767zi.console_after_reset(self.port, self.reset, None)
        self.addCleanup(console.kill)

        self.assertEqual(self.shown_at_reset, os.read(console.stdout.fileno(), len(self.shown_at_reset)))

    # A port left open is one no later scenario can open.
    def test_a_board_that_does_not_start_leaves_the_port_closed(self):
        self.shown_at_reset = b""

        with self.assertRaises(RuntimeError):
            nucleo_f767zi.console_after_reset(self.port, self.reset, None)

        self.assertTrue(self.port.closed)


class BoardAddressTest(unittest.TestCase):
    def test_the_address_is_the_one_the_image_was_configured_with(self):
        cache = "CMAKE_BUILD_TYPE:STRING=Debug\nSOLIDSYSLOG_BDD_BOARD_IP:STRING=192.0.2.2\nSOLIDSYSLOG_BDD_BOARD_NETMASK:STRING=255.255.255.0\n"

        self.assertEqual("192.0.2.2", nucleo_f767zi.board_address_in(cache))

    def test_a_cache_with_no_address_is_refused(self):
        with self.assertRaisesRegex(RuntimeError, "SOLIDSYSLOG_BDD_BOARD_IP"):
            nucleo_f767zi.board_address_in("CMAKE_BUILD_TYPE:STRING=Debug\n")


if __name__ == "__main__":
    unittest.main()
