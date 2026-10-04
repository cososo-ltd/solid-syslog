"""Tests for the remote QEMU target's console (remote_console.py).

Run:  python -m unittest discover -s Bdd/Targets/VxWorks64/Runner -p 'test_*.py'
"""

import os
import socket
import sys
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


if __name__ == "__main__":
    unittest.main()
