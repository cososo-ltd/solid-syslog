"""The NUCLEO-F767ZI as a BDD run drives it: its console on the ST-LINK's
virtual COM port, its reset through the ST-LINK's debug probe, and what the
run reads from the image's banner and from the build that made the image.
"""

import os
import re
import subprocess
import sys
import threading
import time

from serial_console import SerialConsole

CONSOLE_BAUD = 115200
PROMPT = b"SolidSyslog> "
# How long a read of the port waits for a byte before it gives none.
_READ_WAIT_SECONDS = 0.05
_PROBE_TIMEOUT_SECONDS = 30
_PROBE_ATTEMPTS = 3
# Three attempts one after another have all failed inside a fifth of a second,
# and a reset asked for a second later was done.
_PROBE_RECOVERY_SECONDS = 1
_QUIET_SECONDS = 0.5
# A boot prints for a few seconds and then waits at its prompt.
_NEVER_QUIET_SECONDS = 30
# The banner follows a reset at once; this is far longer than it has ever taken.
_BANNER_WAIT_SECONDS = 5
_RESET_ATTEMPTS = 3

_BANNER_STARTS = b"SolidSyslog BDD target"
_BANNER_COMMIT = re.compile(r"NUCLEO-F767ZI, commit (\S+)")
_CACHED_ADDRESS = re.compile(r"^SOLIDSYSLOG_BDD_BOARD_IP:\w+=(\S+)$", re.MULTILINE)


# Resets the board and returns its console, opened first so that the banner
# the reset draws is on it. port_name is the ST-LINK's virtual COM port, and
# programmer is STM32_Programmer_CLI, by path or as PATH finds it.
def open_board(port_name, programmer):
    # Imported here so that only a run against the board needs pyserial.
    import serial

    def report(said):
        print(said, file=sys.__stderr__, flush=True)

    # The probe's exit code, none where it gave no answer, and what it said.
    def ask_probe():
        try:
            done = subprocess.run(
                [programmer, "-c", "port=SWD", "-hardRst"],
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=_PROBE_TIMEOUT_SECONDS,
            )
        except subprocess.TimeoutExpired:
            return None, f"nothing within {_PROBE_TIMEOUT_SECONDS} seconds"
        said = done.stdout.decode(errors="replace").splitlines()
        return done.returncode, " | ".join(line.strip() for line in said if line.strip())

    def reset():
        return reset_through_probe(ask_probe, _PROBE_ATTEMPTS, report, time.sleep)

    port = _ArrivedBytes(serial.Serial(port_name, CONSOLE_BAUD, timeout=_READ_WAIT_SECONDS))
    record_path = os.environ.get("NUCLEO_F767ZI_CONSOLE_RECORD")
    if record_path:
        record = _timed_lines_in(record_path)
        record(f"port {port_name} opened")
        port = RecordedPort(port, record)
        reset = recorded_reset(reset, record)
    return console_after_reset(port, reset, report)


# Appends each line to the file at path as it is given, with the time of day,
# so the record holds whatever was known when a run stopped.
def _timed_lines_in(path):
    lock = threading.Lock()

    def record(line):
        now = time.time()
        stamp = time.strftime("%H:%M:%S", time.localtime(now)) + f".{int(now * 1000) % 1000:03d}"
        with lock, open(path, "a", encoding="utf-8") as lines:
            lines.write(f"{stamp} {line}{chr(10)}")

    return record


# A port that records what crosses it. The record is taken here, before the
# pipe and the threads a run reads the console through, so it says what the
# board sent whatever became of it afterwards.
class RecordedPort:
    def __init__(self, port, record):
        self._port = port
        self._record = record

    def read(self):
        data = self._port.read()
        if data:
            self._record(f"from the board: {data!r}")
        return data

    def write(self, data):
        self._record(f"to the board: {data!r}")
        self._port.write(data)

    def close(self):
        self._record("port closed")
        self._port.close()


# reset, with when it was asked for and what the probe said of it recorded.
def recorded_reset(reset, record):
    def recorded():
        record("reset asked of the probe")
        said = reset()
        record(f"reset done, the probe said: {said}")
        return said

    return recorded


# Has the probe reset the board, and returns what it said, for a report of a
# reset that did not take. The probe has been seen to fail once in some
# thousands of resets, so it is asked again, and each failure is reported, with
# what it said, rather than passed over. What fails it outlasts an attempt, so
# wait is given the seconds to let pass before the next.
def reset_through_probe(ask_probe, attempts, report, wait):
    code = None
    attempt = 0
    while (code != 0) and (attempt < attempts):
        attempt += 1
        code, said = ask_probe()
        if (code != 0) and (attempt < attempts):
            report(f"NUCLEO-F767ZI: the probe did not reset the board at attempt {attempt}, and is asked again. It said: {said}")
            wait(_PROBE_RECOVERY_SECONDS)
    if code != 0:
        raise RuntimeError(f"The probe did not reset the board in any of {attempts} attempts. It last said: {said}")
    return said


# The console of a board fresh from reset, on a port just opened. A board that
# does not start leaves the port closed, for the next to open it.
def console_after_reset(port, reset, report):
    try:
        discard_until_quiet(port, _QUIET_SECONDS, _NEVER_QUIET_SECONDS)
        shown = reset_until_banner(port, reset, _BANNER_WAIT_SECONDS, _RESET_ATTEMPTS, report)
    except BaseException:
        port.close()
        raise
    return SerialConsole(port, reset, shown)


# The ST-LINK keeps what the board printed while the port was closed - the
# whole of a boot, after the reset that ended the last scenario - and hands it
# over once the port is open. Its banner would be taken for the one the next
# reset draws, so it is discarded, until nothing has arrived for quiet_seconds.
# A board still printing after give_up_seconds is not one a run can read.
def discard_until_quiet(port, quiet_seconds, give_up_seconds):
    started = time.monotonic()
    quiet_since = started
    while (time.monotonic() - quiet_since) < quiet_seconds:
        if port.read():
            quiet_since = time.monotonic()
        if (time.monotonic() - started) > give_up_seconds:
            raise RuntimeError(f"The board has not been quiet for {quiet_seconds} seconds in {give_up_seconds}")


# Resets the board, and again if it shows no banner: the banner is the first
# thing a reset draws, so a board without one wait_seconds after a reset has
# not started. Once in about a thousand resets through the probe the board has
# stayed silent, for a reason not yet found, so each time is reported, with
# what the probe said of the reset, rather than passed over. Returns what the
# board showed from the banner on. What came before it is the boot before
# this one, still printing as the reset was asked for, and is discarded: its
# prompt would be taken for a board that is ready while the board is starting.
def reset_until_banner(port, reset, wait_seconds, attempts, report):
    banner_at = -1
    attempt = 0
    while (banner_at < 0) and (attempt < attempts):
        attempt += 1
        said = reset()
        shown = bytearray()
        deadline = time.monotonic() + wait_seconds
        while (banner_at < 0) and (time.monotonic() < deadline):
            shown.extend(port.read())
            banner_at = shown.find(_BANNER_STARTS)
        if (banner_at < 0) and (attempt < attempts):
            report(
                f"NUCLEO-F767ZI: the board showed no banner within {wait_seconds} seconds of reset {attempt},"
                f" and is reset again. It showed {bytes(shown)!r}. The probe said: {said}"
            )
    if banner_at < 0:
        raise RuntimeError(
            f"The board showed no banner after any of {attempts} resets. It last showed {bytes(shown)!r}."
            f" The probe last said: {said}"
        )
    return bytes(shown[banner_at:])


# The port as SerialConsole reads it: what has arrived, without waiting for more.
class _ArrivedBytes:
    def __init__(self, port):
        self._port = port

    def read(self):
        return self._port.read(self._port.in_waiting or 1)

    def write(self, data):
        self._port.write(data)

    def close(self):
        self._port.close()


# Everything the board shows up to its prompt, which a board fresh from a reset
# reaches once its network has had its chance to come up. A board that has not
# shown the prompt within timeout_seconds raises, with what it did show.
def output_to_prompt(console, timeout_seconds):
    shown = bytearray()

    def read():
        data = b"-"
        while data and not shown.endswith(PROMPT):
            data = os.read(console.stdout.fileno(), 1)
            shown.extend(data)

    reader = threading.Thread(target=read, daemon=True)
    reader.start()
    reader.join(timeout_seconds)
    if not shown.endswith(PROMPT):
        # Ending the console ends its output, and the reader with it.
        console.kill()
        reader.join()
        raise TimeoutError(
            f"The board showed no prompt within {timeout_seconds} seconds: {bytes(shown).decode(errors='replace')!r}"
        )
    return bytes(shown).decode(errors="replace")


# Raises unless the banner in output, which is what the board showed from its
# reset to its prompt, names commit. The build writes the first ten digits, and
# "-dirty" after them for a tree with changes the commit does not hold.
def check_image_is_of_commit(output, commit):
    banner = _BANNER_COMMIT.search(output)
    if not banner:
        raise RuntimeError(f"The board showed no banner after its reset: {output!r}")
    if banner.group(1) != commit[:10]:
        raise RuntimeError(
            f"The image on the board is of commit {banner.group(1)}, and the commit under test is {commit[:10]}"
        )


# The address the image was built to take, from the CMakeCache.txt of the build
# that made it. The board sends it as its HOSTNAME.
def board_address_in(cache):
    address = _CACHED_ADDRESS.search(cache)
    if not address:
        raise RuntimeError("SOLIDSYSLOG_BDD_BOARD_IP is not in the build's CMakeCache.txt")
    return address.group(1)
