"""Tests for the remote QEMU target's console (remote_console.py).

Run:  python -m unittest discover -s Bdd/Targets/VxWorks64/Runner -p 'test_*.py'
"""

import os
import socket
import struct
import sys
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
        connection, _ = listener.accept()
        self.console = remote_console.RemoteConsole(connection)

    def test_what_the_target_sends_can_be_read_from_stdout(self):
        self.target.sendall(b"SolidSyslog> ")

        self.assertEqual(b"SolidSyslog> ", os.read(self.console.stdout.fileno(), 13))

    def test_what_the_steps_write_to_stdin_reaches_the_target(self):
        self.console.stdin.write("send 1\n")
        self.console.stdin.flush()

        self.target.settimeout(5)
        self.assertEqual(b"send 1\n", self.target.recv(4096))

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

    def poll_until_exited(self):
        deadline = time.monotonic() + 5
        while self.console.poll() is None and time.monotonic() < deadline:
            time.sleep(0.01)
        return self.console.poll()


if __name__ == "__main__":
    unittest.main()
