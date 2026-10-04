"""The console of the remote QEMU target, as the BDD steps drive a process.

QEMU on the build machine connects its serial port out to the development
machine. RemoteConsole wraps that connection so the steps can read and write it
as they do a local target's pipes.
"""

import os
import threading


class RemoteConsole:
    def __init__(self, connection):
        self._connection = connection
        read_end, self._write_end = os.pipe()
        self.stdout = os.fdopen(read_end, "rb", buffering=0)
        threading.Thread(target=self._copy_output, daemon=True).start()

    # The steps read stdout with os.read, which needs a real file descriptor;
    # a socket has none on Windows, so the output is copied into a pipe.
    def _copy_output(self):
        data = self._connection.recv(4096)
        while data:
            os.write(self._write_end, data)
            data = self._connection.recv(4096)
