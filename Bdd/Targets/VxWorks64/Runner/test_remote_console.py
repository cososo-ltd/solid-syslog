"""Tests for the remote QEMU target's console (remote_console.py).

Run:  python -m unittest discover -s Bdd/Targets/VxWorks64/Runner -p 'test_*.py'
"""

import os
import socket
import struct
import subprocess
import sys
import threading
import time
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import remote_console  # noqa: E402


class RemoteConsoleTest(unittest.TestCase):
    def setUp(self):
        listener = socket.create_server(("127.0.0.1", 0))
        self.addCleanup(listener.close)
        self.target = socket.create_connection(listener.getsockname())
        self.addCleanup(self.target.close)
        self.connection, _ = listener.accept()
        self.stops = []
        self.console = remote_console.RemoteConsole(self.connection, lambda: self.stops.append("stopped"))

    def test_what_the_target_sends_can_be_read_from_stdout(self):
        self.target.sendall(b"SolidSyslog> ")

        self.assertEqual(b"SolidSyslog> ", os.read(self.console.stdout.fileno(), 13))

    def test_what_the_steps_write_to_stdin_reaches_the_target(self):
        self.console.stdin.write("send 1\n")
        self.console.stdin.flush()

        self.target.settimeout(5)
        self.assertEqual(b"send 1\n", self.target.recv(4096))

    # A byte on a serial line is ten bits: a start bit, eight data bits, a stop bit.
    def test_input_takes_the_time_a_serial_line_at_the_baud_rate_would(self):
        console, _, sleeps = self.paced_console(38400)

        console.stdin.write("set host x\nset port 5514\n")

        self.assertAlmostEqual(25 * 10 / 38400, sum(sleeps))

    # Bytes between two sleeps leave in one burst, so none is longer than the line
    # would carry in one tick of the host's sleep.
    def test_no_burst_is_longer_than_the_line_carries_in_a_tick(self):
        console, _, sleeps = self.paced_console(38400)

        console.stdin.write("set msg " + "X" * 375 + "\n")

        self.assertAlmostEqual(384 * 10 / 38400, sum(sleeps))
        for sleep in sleeps:
            self.assertLessEqual(sleep, remote_console.HOST_TICK_SECONDS)

    def test_paced_input_reaches_the_target_whole(self):
        console, target, _ = self.paced_console(38400)
        line = "set msg " + "X" * 375 + "\n"

        console.stdin.write(line)

        target.settimeout(5)
        received = b""
        while len(received) < len(line):
            received += target.recv(4096)
        self.assertEqual(line.encode(), received)

    def test_with_no_baud_rate_input_is_not_paced(self):
        sleeps = []
        console = remote_console.RemoteConsole(self.connection, lambda: None, sleep=sleeps.append)

        console.stdin.write("set host x\n")

        self.assertEqual([], sleeps)

    # A console of its own, paced at baud, with its target end and the sleeps it asked for.
    def paced_console(self, baud):
        sleeps = []
        listener = socket.create_server(("127.0.0.1", 0))
        self.addCleanup(listener.close)
        target = socket.create_connection(listener.getsockname())
        self.addCleanup(target.close)
        connection, _ = listener.accept()
        return remote_console.RemoteConsole(connection, lambda: None, baud, sleeps.append), target, sleeps

    def test_it_is_running_while_the_target_is_connected(self):
        self.assertIsNone(self.console.poll())

    def test_it_has_exited_once_the_target_disconnects(self):
        self.target.close()

        self.assertEqual(0, self.poll_until_exited())

    # Stopping QEMU kills it, so its end of the console is reset, not closed.
    def test_it_has_exited_once_the_target_resets_the_connection(self):
        self.target.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
        self.target.close()

        self.assertEqual(0, self.poll_until_exited())

    # A target cannot hand QEMU an exit status, so it prints one instead.
    def test_an_exit_marker_from_the_target_becomes_its_exit_code(self):
        self.target.sendall(b"store full\r\n[EXIT 2]\r\n")
        self.target.close()

        self.assertEqual(2, self.poll_until_exited())

    def test_an_exit_marker_stops_the_target(self):
        self.target.sendall(b"[EXIT 2]\r\n")
        self.target.close()
        self.poll_until_exited()

        self.assertEqual(["stopped"], self.stops)

    def test_an_exit_marker_split_across_reads_is_still_seen(self):
        self.target.sendall(b"[EXI")
        time.sleep(0.1)
        self.target.sendall(b"T 2]\r\n")
        self.target.close()

        self.assertEqual(2, self.poll_until_exited())

    def test_the_exit_marker_still_reaches_stdout(self):
        self.target.sendall(b"[EXIT 2]")

        self.assertEqual(b"[EXIT 2]", os.read(self.console.stdout.fileno(), 8))

    def test_killing_it_stops_the_target(self):
        self.console.kill()

        self.assertEqual(["stopped"], self.stops)

    def test_waiting_after_killing_it_returns_though_the_disconnect_never_arrives(self):
        self.console.kill()

        self.console.wait(timeout=5)

    def test_waiting_returns_the_exit_code_once_the_target_disconnects(self):
        self.target.close()

        self.assertEqual(0, self.console.wait(timeout=5))

    def test_waiting_while_the_target_is_connected_times_out_as_a_process_would(self):
        with self.assertRaises(subprocess.TimeoutExpired):
            self.console.wait(timeout=0.1)

    def test_the_collector_address_is_the_one_the_target_reached_this_machine_at(self):
        self.assertEqual("127.0.0.1", self.console.collector_address)

    def test_it_has_no_process_id_on_this_machine(self):
        self.assertIsNone(self.console.pid)

    def test_it_has_no_separate_error_stream(self):
        self.assertIsNone(self.console.stderr)

    def test_its_connection_is_closed_once_the_target_disconnects(self):
        self.target.close()
        self.poll_until_exited()

        self.assertEqual(-1, self.connection.fileno())

    def poll_until_exited(self):
        deadline = time.monotonic() + 5
        while self.console.poll() is None and time.monotonic() < deadline:
            time.sleep(0.01)
        return self.console.poll()


class PromptTest(unittest.TestCase):
    """Given its prompt, the console types the next line only once the target
    has shown the prompt after the last - as a person at the terminal would."""

    PROMPT = b"SolidSyslog> "

    def setUp(self):
        listener = socket.create_server(("127.0.0.1", 0))
        self.addCleanup(listener.close)
        self.target = socket.create_connection(listener.getsockname())
        self.addCleanup(self.target.close)
        self.target.settimeout(5)
        connection, _ = listener.accept()
        self.console = remote_console.RemoteConsole(
            connection, lambda: None, prompt=self.PROMPT, prompt_timeout_seconds=5
        )

    def test_the_first_line_goes_at_once(self):
        self.console.stdin.write("set a\n")

        self.assertEqual(b"set a\n", self.target.recv(4096))

    def test_a_line_waits_for_the_prompt_after_the_one_before(self):
        self.console.stdin.write("set a\n")
        self.target.recv(4096)

        writer = self.write_in_background("set b\n")

        self.assertFalse(self.arrives_within(0.3))
        self.target.sendall(b"set a=1\r\n" + self.PROMPT)
        self.assertEqual(b"set b\n", self.target.recv(4096))
        writer.join(5)

    def test_lines_written_together_still_wait_for_each_prompt(self):
        writer = self.write_in_background("set a\nset b\n")

        self.assertEqual(b"set a\n", self.target.recv(4096))
        self.assertFalse(self.arrives_within(0.3))
        self.target.sendall(self.PROMPT)
        self.assertEqual(b"set b\n", self.target.recv(4096))
        writer.join(5)

    def test_a_prompt_split_across_reads_still_counts(self):
        self.console.stdin.write("set a\n")
        self.target.recv(4096)
        writer = self.write_in_background("set b\n")

        self.target.sendall(self.PROMPT[:5])
        time.sleep(0.1)
        self.target.sendall(self.PROMPT[5:])

        self.assertEqual(b"set b\n", self.target.recv(4096))
        writer.join(5)

    def test_a_line_goes_anyway_when_the_prompt_never_comes(self):
        listener = socket.create_server(("127.0.0.1", 0))
        self.addCleanup(listener.close)
        target = socket.create_connection(listener.getsockname())
        self.addCleanup(target.close)
        target.settimeout(5)
        connection, _ = listener.accept()
        console = remote_console.RemoteConsole(
            connection, lambda: None, prompt=self.PROMPT, prompt_timeout_seconds=0.2
        )
        console.stdin.write("set a\n")
        target.recv(4096)

        console.stdin.write("set b\n")

        self.assertEqual(b"set b\n", target.recv(4096))

    def write_in_background(self, text):
        writer = threading.Thread(target=self.console.stdin.write, args=(text,), daemon=True)
        writer.start()
        return writer

    def arrives_within(self, seconds):
        self.target.settimeout(seconds)
        try:
            return bool(self.target.recv(4096))
        except TimeoutError:
            return False
        finally:
            self.target.settimeout(5)


class OpenTest(unittest.TestCase):
    def setUp(self):
        self.listener = socket.create_server(("127.0.0.1", 0))
        self.addCleanup(self.listener.close)

    # QEMU connects its console out once started, as the start job begins it.
    def start_a_target_that_connects(self):
        target = socket.create_connection(self.listener.getsockname())
        self.addCleanup(target.close)
        target.sendall(b"booting")

    def test_opening_starts_the_target_and_returns_its_console(self):
        console = remote_console.open_remote_target(self.listener, self.start_a_target_that_connects, lambda: None)

        self.assertEqual(b"booting", os.read(console.stdout.fileno(), 7))

    def test_opening_gives_the_console_the_baud_rate(self):
        sleeps = []
        console = remote_console.open_remote_target(
            self.listener, self.start_a_target_that_connects, lambda: None, None, 38400, sleeps.append
        )

        console.stdin.write("set host x\n")

        self.assertAlmostEqual(11 * 10 / 38400, sum(sleeps))

    def test_a_target_that_never_connects_times_out_and_is_stopped(self):
        stops = []

        with self.assertRaises(TimeoutError):
            remote_console.open_remote_target(self.listener, lambda: None, lambda: stops.append("stopped"), 0.1)
        self.assertEqual(["stopped"], stops)


if __name__ == "__main__":
    unittest.main()
