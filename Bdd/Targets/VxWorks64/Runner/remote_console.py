"""The console of the remote QEMU target, as the BDD steps drive a process.

QEMU on the build machine connects its serial port out to the development
machine. RemoteConsole wraps that connection so the steps can read and write it
as they do a local target's pipes.
"""

import os
import re
import subprocess
import threading
import time


# Starts the target, which connects its console to listener, and returns that
# console. start and stop begin and end the target - in practice the runner's
# qemu-start and qemu-stop jobs. A target that has not connected within
# accept_timeout seconds is stopped rather than left running. line_gap_seconds
# and sleep pace the console's input, as for RemoteConsole.
def open_remote_target(listener, start, stop, accept_timeout=None, line_gap_seconds=0.0, sleep=time.sleep):
    start()
    listener.settimeout(accept_timeout)
    try:
        connection, _ = listener.accept()
    except TimeoutError:
        stop()
        raise
    connection.settimeout(None)
    return RemoteConsole(connection, stop, line_gap_seconds, sleep)


class RemoteConsole:
    # stop ends the target - in practice the runner's qemu-stop job. Each line
    # written to stdin is followed by line_gap_seconds, slept with sleep.
    def __init__(self, connection, stop, line_gap_seconds=0.0, sleep=time.sleep):
        self._connection = connection
        self._stop = stop
        # Where the target reached this machine, so where its syslog should go.
        self.collector_address = connection.getsockname()[0]
        read_end, self._write_end = os.pipe()
        self.stdout = os.fdopen(read_end, "rb", buffering=0)
        self.stdin = _ConsoleInput(connection, line_gap_seconds, sleep)
        # A serial console carries one stream; errors arrive on stdout with the rest.
        self.stderr = None
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
        exit_status = _ExitMarker()
        data = _receive(self._connection)
        while data:
            os.write(self._write_end, data)
            if exit_status.seen_in(data):
                self._stop()
            data = _receive(self._connection)
        os.close(self._write_end)
        self._connection.close()
        self.returncode = exit_status.code


# The target cannot hand QEMU an exit status, so a target that ends the run
# prints one as "[EXIT n]" and the console stops it. Output arrives in pieces of
# any size, so the end of each piece is kept to find a marker split across two.
class _ExitMarker:
    _PATTERN = re.compile(rb"\[EXIT (\d+)\]")
    _LONGEST = 16

    def __init__(self):
        self.code = 0
        self._found = False
        self._tail = b""

    def seen_in(self, data):
        newly_found = False
        if not self._found:
            text = self._tail + data
            match = self._PATTERN.search(text)
            if match:
                self.code = int(match.group(1))
                self._found = True
                newly_found = True
            self._tail = text[-self._LONGEST:]
        return newly_found


def _receive(connection):
    try:
        return connection.recv(4096)
    except ConnectionResetError:
        return b""


# Text written as the steps write a local target's stdin, sent a line at a time
# with a gap after each - the pace of a serial line rather than of a socket.
class _ConsoleInput:
    def __init__(self, connection, line_gap_seconds, sleep):
        self._connection = connection
        self._line_gap_seconds = line_gap_seconds
        self._sleep = sleep

    def write(self, text):
        for line in text.splitlines(keepends=True):
            self._connection.sendall(line.encode())
            self._sleep(self._line_gap_seconds)

    def flush(self):
        pass
