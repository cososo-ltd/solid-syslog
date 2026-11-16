"""A board's serial console, presented to the BDD steps as a process.

The board's console is a serial port on the development machine, and the board
is started by resetting it. SerialConsole wraps both so that the steps can read
and write the console as they do a local target's pipes.

A board has no exit status. A target that ends prints "exit: ok", or
"exit: failed" and the status, on a line of its own and then halts.
SerialConsole reports that line as the exit of a local target.
"""

import os
import re
import subprocess
import threading

_DISCARDED_AT_A_TIME = 4096
_EXIT_LINE = re.compile(rb"\nexit: (?:ok|failed (\d+))\r?\n")
# Longer than any exit line, so that one split across reads is still found.
_EXIT_LINE_KEPT = 64


class SerialConsole:
    # port is the open serial port, whose read returns the bytes that have
    # arrived, or none after a short timeout. reset restarts the board. shown
    # is output already read from the board, which stdout delivers first.
    def __init__(self, port, reset, shown=b""):
        self._port = port
        self._reset = reset
        read_end, self._write_end = os.pipe()
        self.stdout = os.fdopen(read_end, "rb", buffering=0)
        self.stdin = _ConsoleInput(port)
        # A serial console is a single stream, so errors arrive on stdout.
        self.stderr = None
        self.pid = None
        self.returncode = None
        self._ended = threading.Event()
        self._copier = threading.Thread(target=self._copy_output, args=(shown,), daemon=True)
        self._copier.start()

    def poll(self):
        return self.returncode

    # The steps kill a target to end it and to simulate a power cut. On the
    # board both are a reset, which is not a loss of power at the media.
    # Unread output is discarded first, because the copier may be blocked on a
    # full pipe and would not otherwise notice that the console has ended. A
    # board that has reported its exit has already halted and is not reset, so
    # its output can still be read; the next target to open resets it. In
    # both cases kill returns only once the port is closed, because the next
    # target opens the same port.
    def kill(self):
        exited = self._ended.is_set()
        self._ended.set()
        if not exited:
            while os.read(self.stdout.fileno(), _DISCARDED_AT_A_TIME):
                pass
        self._copier.join()
        if not exited:
            self._reset()

    def wait(self, timeout=None):
        self._copier.join(timeout)
        if self._copier.is_alive():
            raise subprocess.TimeoutExpired("serial console", timeout)
        return self.returncode

    # The steps read stdout with os.read, which needs a real file descriptor,
    # so the output is copied into a pipe. Closing the pipe when the console
    # ends gives the reader end of file, and a port failure ends the console
    # as a kill does, so the reader is never left waiting. The output already
    # read is written from this thread as well, because a write to a full pipe
    # blocks and nothing reads it until the console has been handed over. It
    # is scanned for an exit line like the rest.
    def _copy_output(self, shown):
        self._status = 0
        self._latest = b""
        try:
            self._copy(shown)
            while not self._ended.is_set():
                self._copy(self._port.read())
        finally:
            try:
                os.close(self._write_end)
                self._port.close()
            finally:
                self.returncode = self._status

    def _copy(self, arrived):
        self._write_all(arrived)
        self._latest = (self._latest + arrived)[-_EXIT_LINE_KEPT:]
        exited = _EXIT_LINE.search(self._latest)
        if exited:
            self._status = int(exited.group(1) or 0)
            self._ended.set()

    def _write_all(self, data):
        while data:
            data = data[os.write(self._write_end, data):]


# Accepts text as a local target's stdin does.
class _ConsoleInput:
    def __init__(self, port):
        self._port = port

    def write(self, text):
        self._port.write(text.encode())

    def flush(self):
        pass
