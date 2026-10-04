"""The console of the remote QEMU target, as the BDD steps drive a process.

QEMU on the build machine connects its serial port out to the development
machine. RemoteConsole wraps that connection so the steps can read and write it
as they do a local target's pipes.
"""

import os
import subprocess
import threading


# Starts the target, which connects its console to listener, and returns that
# console. start and stop begin and end the target - in practice the runner's
# qemu-start and qemu-stop jobs.
def open_remote_target(listener, start, stop):
    start()
    connection, _ = listener.accept()
    return RemoteConsole(connection, stop)


class RemoteConsole:
    # stop ends the target - in practice the runner's qemu-stop job.
    def __init__(self, connection, stop):
        self._connection = connection
        self._stop = stop
        # Where the target reached this machine, so where its syslog should go.
        self.collector_address = connection.getsockname()[0]
        read_end, self._write_end = os.pipe()
        self.stdout = os.fdopen(read_end, "rb", buffering=0)
        self.stdin = _ConsoleInput(connection)
        # The target runs on another machine, so there is no local process.
        self.pid = None
        self.returncode = None
        self._copier = threading.Thread(target=self._copy_output, daemon=True)
        self._copier.start()

    def poll(self):
        return self.returncode

    def kill(self):
        self._stop()

    def wait(self, timeout=None):
        self._copier.join(timeout)
        if self._copier.is_alive():
            raise subprocess.TimeoutExpired("remote console", timeout)
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
