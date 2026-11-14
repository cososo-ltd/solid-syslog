"""Tests for the board's console over a serial port (serial_console.py).

Run:  python -m unittest discover -s Bdd/Targets/Boards/NucleoF767zi/Runner -p 'test_*.py'
"""

import os
import queue
import subprocess
import sys
import threading
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
        self.fails = False
        self.fails_to_close = False

    def read(self):
        if self.fails:
            raise OSError("the port has gone")
        try:
            return self.arrived.get(timeout=0.01)
        except queue.Empty:
            return b""

    def write(self, data):
        self.written += data

    def close(self):
        self.closed = True
        if self.fails_to_close:
            raise OSError("the port would not close")


class SerialConsoleTest(unittest.TestCase):
    def setUp(self):
        self.port = FakePort()
        self.resets = []
        self.console = serial_console.SerialConsole(self.port, lambda: self.resets.append("reset"))
        self.addCleanup(self.console.kill)

    def read_from(self, console, count):
        read = b""
        while len(read) < count:
            read += os.read(console.stdout.fileno(), count - len(read))
        return read

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

    def test_a_port_that_fails_to_close_still_ends_the_console(self):
        self.addCleanup(setattr, threading, "excepthook", threading.excepthook)
        threading.excepthook = lambda args: None
        self.port.fails_to_close = True

        self.console.kill()

        self.assertEqual(0, self.console.poll())

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

    def test_what_the_board_had_shown_already_is_read_first(self):
        port = FakePort()
        port.arrived.put(b"link up")
        console = serial_console.SerialConsole(port, lambda: None, b"banner, ")
        self.addCleanup(console.kill)

        self.assertEqual(b"banner, link up", self.read_from(console, 15))

    # A pipe holds only so much before a write to it waits for a reader, and
    # nobody can read a console that has not been handed over yet.
    def test_more_shown_already_than_a_pipe_holds_does_not_keep_the_console_from_opening(self):
        shown = b"x" * 1000000
        opened = []
        opener = threading.Thread(
            target=lambda: opened.append(serial_console.SerialConsole(FakePort(), lambda: None, shown)), daemon=True
        )

        opener.start()
        opener.join(5)

        self.assertFalse(opener.is_alive())
        self.addCleanup(opened[0].kill)
        self.assertEqual(shown, self.read_from(opened[0], len(shown)))

    # The steps stop reading at the prompt, so a board that goes on talking
    # fills the pipe, and the kill must not wait for a reader that is not coming.
    def test_a_board_whose_output_nobody_reads_can_still_be_killed(self):
        self.port.arrived.put(b"x" * 1000000)
        killer = threading.Thread(target=self.console.kill, daemon=True)

        killer.start()
        killer.join(5)

        self.assertFalse(killer.is_alive())
        self.assertEqual(["reset"], self.resets)
        self.assertTrue(self.port.closed)

    # A probe unplugged mid-scenario: the steps must see the console end
    # rather than wait out their timeout on a pipe nobody will write.
    def test_a_port_that_fails_ends_stdout_and_frees_the_port(self):
        self.port.fails = True

        self.assertEqual(b"", os.read(self.console.stdout.fileno(), 1))
        self.assertEqual(0, self.console.wait(timeout=5))
        self.assertTrue(self.port.closed)

    def test_a_killed_board_can_be_waited_on(self):
        self.console.kill()

        self.assertEqual(0, self.console.wait(timeout=5))
        self.assertEqual(0, self.console.poll())


if __name__ == "__main__":
    unittest.main()
