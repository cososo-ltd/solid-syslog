"""The console of the remote QEMU target, as the BDD steps drive a process.

QEMU on the build machine connects its serial port out to the development
machine. RemoteConsole wraps that connection so the steps can read and write it
as they do a local target's pipes.
"""

import os
import re
import socket
import subprocess
import threading
import time


# The shortest sleep the host keeps to: Windows wakes a sleeping thread on its
# timer tick, about 15.6 ms. Bytes sent between two sleeps leave together.
HOST_TICK_SECONDS = 0.015

# A byte on a serial line: a start bit, eight data bits and a stop bit.
_BITS_PER_BYTE = 10


# Starts the target, which connects its console to listener, and returns that
# console. start and stop begin and end the target - in practice the runner's
# qemu-start and qemu-stop jobs. A target that has not connected within
# accept_timeout seconds is stopped rather than left running. baud, sleep and
# prompt pace the console's input, as for RemoteConsole.
def open_remote_target(listener, start, stop, accept_timeout=None, baud=None, sleep=time.sleep, prompt=None):
    start()
    listener.settimeout(accept_timeout)
    try:
        connection, _ = listener.accept()
    except TimeoutError:
        stop()
        raise
    connection.settimeout(None)
    return RemoteConsole(connection, stop, baud, sleep, prompt)


class RemoteConsole:
    # stop ends the target - in practice the runner's qemu-stop job. Input
    # written to stdin goes no faster than a serial line at baud would carry it,
    # slept with sleep; with no baud it goes as fast as the connection takes it.
    # Given its prompt, each line after the first waits until the target has
    # shown the prompt again - for prompt_timeout_seconds at most, after which it
    # goes anyway rather than hang the run.
    def __init__(self, connection, stop, baud=None, sleep=time.sleep, prompt=None, prompt_timeout_seconds=30):
        self._connection = connection
        self._stop = stop
        # Where the target reached this machine, so where its syslog should go.
        self.collector_address = connection.getsockname()[0]
        read_end, self._write_end = os.pipe()
        self.stdout = os.fdopen(read_end, "rb", buffering=0)
        self._prompts = _PromptCount(prompt)
        self.stdin = _ConsoleInput(connection, baud, sleep, self._prompts, prompt_timeout_seconds)
        # A serial console carries one stream; errors arrive on stdout with the rest.
        self.stderr = None
        # The target runs on another machine, so there is no local process.
        self.pid = None
        self.returncode = None
        self._copier = threading.Thread(target=self._copy_output, daemon=True)
        self._copier.start()

    def poll(self):
        return self.returncode

    # Stopping QEMU resets the connection, but a reset lost on the way would
    # leave the output copier waiting for good, so the console ends it here:
    # shutdown wakes its receive on Linux, and close does on Windows.
    def kill(self):
        self._stop()
        try:
            self._connection.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        self._connection.close()

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
            self._prompts.count_in(data)
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


# How many times the target has shown its prompt, for input to wait on. As with
# the exit marker, the end of each piece of output is kept to find a prompt split
# across two. With no prompt there is nothing to count, and nothing waits.
class _PromptCount:
    def __init__(self, prompt):
        self._prompt = prompt
        self._tail = b""
        self._count = 0
        self._changed = threading.Condition()

    def count_in(self, data):
        if self._prompt:
            text = self._tail + data
            found = text.count(self._prompt)
            self._tail = text[-(len(self._prompt) - 1):] if len(self._prompt) > 1 else b""
            if found:
                with self._changed:
                    self._count += found
                    self._changed.notify_all()

    def now(self):
        with self._changed:
            return self._count

    # Waits until the count has passed seen, or timeout_seconds go by.
    def wait_beyond(self, seen, timeout_seconds):
        if self._prompt:
            with self._changed:
                self._changed.wait_for(lambda: self._count > seen, timeout_seconds)


def _receive(connection):
    try:
        return connection.recv(4096)
    except OSError:
        return b""


# Text written as the steps write a local target's stdin, typed as a person at a
# serial terminal would: a line at a time, each after the prompt the last one
# drew, and no faster than the line carries it. Input arriving faster than that
# is lost, or resets the target. Each piece is what the line carries in one host
# tick, followed by the time it takes on the wire.
class _ConsoleInput:
    def __init__(self, connection, baud, sleep, prompts, prompt_timeout_seconds):
        self._connection = connection
        self._baud = baud
        self._sleep = sleep
        self._prompts = prompts
        self._prompt_timeout_seconds = prompt_timeout_seconds
        self._prompts_before_last_line = None
        self._piece_bytes = None
        if baud:
            self._piece_bytes = max(1, int(baud * HOST_TICK_SECONDS / _BITS_PER_BYTE))

    def write(self, text):
        for line in text.splitlines(keepends=True):
            if self._prompts_before_last_line is not None:
                self._prompts.wait_beyond(self._prompts_before_last_line, self._prompt_timeout_seconds)
            self._prompts_before_last_line = self._prompts.now()
            self._send(line.encode())

    def _send(self, data):
        if not self._piece_bytes:
            self._connection.sendall(data)
        else:
            for start in range(0, len(data), self._piece_bytes):
                piece = data[start:start + self._piece_bytes]
                self._connection.sendall(piece)
                self._sleep(len(piece) * _BITS_PER_BYTE / self._baud)

    def flush(self):
        pass
