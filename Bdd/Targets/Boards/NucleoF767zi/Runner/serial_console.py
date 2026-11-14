"""The console of a board on a serial port, as the BDD steps drive a process.

The board's console is a serial port on the development machine, and the board
is started by resetting it. SerialConsole wraps the two so the steps can read
and write the console as they do a local target's pipes.
"""

import os
import subprocess
import threading

_DISCARDED_AT_A_TIME = 4096


class SerialConsole:
    # port is the open serial port: read gives the bytes that have arrived, or
    # none once a short wait has passed without any. reset restarts the board.
    # shown is what the board has shown already, which stdout gives first.
    def __init__(self, port, reset, shown=b""):
        self._port = port
        self._reset = reset
        read_end, self._write_end = os.pipe()
        self.stdout = os.fdopen(read_end, "rb", buffering=0)
        self.stdin = _ConsoleInput(port)
        # A serial console carries one stream; errors arrive on stdout with the rest.
        self.stderr = None
        # The target is a board, so there is no local process.
        self.pid = None
        self.returncode = None
        self._ended = threading.Event()
        self._copier = threading.Thread(target=self._copy_output, args=(shown,), daemon=True)
        self._copier.start()

    def poll(self):
        return self.returncode

    # The steps kill a target to end it, and to stand in for a power cut. On
    # the board both are a reset, which is not a loss of power at the media.
    # Output still unread is discarded on the way: the copy may be waiting on
    # a full pipe for a reader, and would otherwise never see that it has ended.
    def kill(self):
        if not self._ended.is_set():
            self._ended.set()
            while os.read(self.stdout.fileno(), _DISCARDED_AT_A_TIME):
                pass
            self._copier.join()
            self._reset()

    def wait(self, timeout=None):
        self._copier.join(timeout)
        if self._copier.is_alive():
            raise subprocess.TimeoutExpired("serial console", timeout)
        return self.returncode

    # The steps read stdout with os.read, which needs a real file descriptor,
    # so the output is copied into a pipe. Closing the pipe as the console ends
    # gives the reader its end of file, and a port that fails ends the console
    # as a kill does, so the reader is not left waiting on it. What the board
    # had shown already goes into the pipe from here too: a pipe that is full
    # makes its writer wait for a reader, and there is none until the console
    # has been handed over.
    def _copy_output(self, shown):
        try:
            self._write_all(shown)
            while not self._ended.is_set():
                self._write_all(self._port.read())
        finally:
            os.close(self._write_end)
            self._port.close()
            self.returncode = 0

    def _write_all(self, data):
        while data:
            data = data[os.write(self._write_end, data):]


# Text written as the steps write a local target's stdin.
class _ConsoleInput:
    def __init__(self, port):
        self._port = port

    def write(self, text):
        self._port.write(text.encode())

    def flush(self):
        pass
