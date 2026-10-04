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
        self.stdin = _ConsoleInput(connection)
        self.returncode = None
        threading.Thread(target=self._copy_output, daemon=True).start()

    def poll(self):
        return self.returncode

    # The steps read stdout with os.read, which needs a real file descriptor;
    # a socket has none on Windows, so the output is copied into a pipe. The
    # console ends when the target disconnects - stopping QEMU resets it - and
    # closing the pipe then gives the reader its end of file.
    def _copy_output(self):
        data = _receive(self._connection)
        while data:
            os.write(self._write_end, data)
            data = _receive(self._connection)
        os.close(self._write_end)
        self.returncode = 0


def _receive(connection):
    try:
        return connection.recv(4096)
    except ConnectionResetError:
        return b""


# Text written as the steps write a local target's stdin, sent as it is written.
class _ConsoleInput:
    def __init__(self, connection):
        self._connection = connection

    def write(self, text):
        self._connection.sendall(text.encode())

    def flush(self):
        pass
