"""Tests for the board's console over a serial port (serial_console.py).

Run:  python -m unittest discover -s Bdd/Targets/Boards/NucleoF767zi/Runner -p 'test_*.py'
"""

import os
import queue
import subprocess
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import serial_console  # noqa: E402


# A serial port as the console reads one: read gives what has arrived, or
# nothing once a short wait has passed with none.
class FakePort:
    def __init__(self):
        self.arrived = queue.Queue()
        self.written = b""
        self.closed = False

    def read(self):
        try:
            return self.arrived.get(timeout=0.01)
        except queue.Empty:
            return b""

    def has_arrived(self):
        return not self.arrived.empty()

    def write(self, data):
        self.written += data

    def close(self):
        self.closed = True


class SerialConsoleTest(unittest.TestCase):
    def setUp(self):
        self.port = FakePort()
        self.resets = []
        self.console = serial_console.SerialConsole(self.port, lambda: self.resets.append("reset"))
        self.addCleanup(self.console.kill)

    def test_what_the_board_sends_can_be_read_from_stdout(self):
        self.port.arrived.put(b"SolidSyslog> ")

        self.assertEqual(b"SolidSyslog> ", os.read(self.console.stdout.fileno(), 13))

    def test_what_the_steps_write_to_stdin_reaches_the_board(self):
        self.console.stdin.write("send 1\n")
        self.console.stdin.flush()

        self.assertEqual(b"send 1\n", self.port.written)

    def test_a_console_carries_one_stream_and_no_process(self):
        self.assertIsNone(self.console.stderr)
        self.assertIsNone(self.console.pid)

    def test_a_board_that_is_running_has_no_return_code(self):
        self.assertIsNone(self.console.poll())

    def test_waiting_on_a_board_that_is_running_times_out(self):
        with self.assertRaises(subprocess.TimeoutExpired):
            self.console.wait(timeout=0.05)

    # A kill stands in for a power cut, which on the board is a reset.
    def test_killing_resets_the_board(self):
        self.console.kill()

        self.assertEqual(["reset"], self.resets)

    def test_killing_twice_resets_the_board_once(self):
        self.console.kill()
        self.console.kill()

        self.assertEqual(["reset"], self.resets)

    def test_killing_ends_stdout_and_frees_the_port(self):
        self.console.kill()

        self.assertEqual(b"", os.read(self.console.stdout.fileno(), 1))
        self.assertTrue(self.port.closed)

    def test_a_killed_board_can_be_waited_on(self):
        self.console.kill()

        self.assertEqual(0, self.console.wait(timeout=5))
        self.assertEqual(0, self.console.poll())


if __name__ == "__main__":
    unittest.main()
