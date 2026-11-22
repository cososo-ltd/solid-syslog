"""Drives the NUCLEO-F767ZI for a BDD run: the console on the ST-LINK's virtual
COM port, reset through the ST-LINK's debug probe, and checks of the image's
banner against the build that made it.
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
# Read timeout on the port.
_READ_WAIT_SECONDS = 0.05
_PROBE_TIMEOUT_SECONDS = 30
_PROBE_ATTEMPTS = 3
# Three immediate retries have all failed within 200 ms, where a retry a
# second later succeeded.
_PROBE_RECOVERY_SECONDS = 1
_QUIET_SECONDS = 0.5
# A boot prints for a few seconds and then waits at its prompt.
_NEVER_QUIET_SECONDS = 30
# The banner follows a reset at once, so this is generous.
_BANNER_WAIT_SECONDS = 5
_RESET_ATTEMPTS = 3

_BANNER_STARTS = b"SolidSyslog BDD target"
_BANNER = re.compile(r"NUCLEO-F767ZI, image (\S+), commit (\S+)")
_STORE_REPORT = re.compile(r"\[store\] flash: erases (\d+), programs (\d+), reads (\d+)")
_CACHED_ADDRESS = re.compile(r"^SOLIDSYSLOG_BDD_BOARD_IP:\w+=(\S+)$", re.MULTILINE)


# Resets the board and returns its console. The port is opened before the
# reset so that the banner is captured. port_name is the ST-LINK's virtual COM
# port, and programmer is STM32_Programmer_CLI, by path or by name on PATH.
def open_board(port_name, programmer):
    # Imported here so that only a run against the board needs pyserial.
    import serial

    def report(said):
        print(said, file=sys.__stderr__, flush=True)

    # Returns the probe's exit code (None on a timeout) and its output.
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


# Appends each line to the file at path as it arrives, with the time of day,
# so the record is complete however the run ends.
def _timed_lines_in(path):
    lock = threading.Lock()

    def record(line):
        now = time.time()
        stamp = time.strftime("%H:%M:%S", time.localtime(now)) + f".{int(now * 1000) % 1000:03d}"
        with lock, open(path, "a", encoding="utf-8") as lines:
            lines.write(f"{stamp} {line}{chr(10)}")

    return record


# A port that records what crosses it. Recording at the port, ahead of the
# pipe and threads the run reads through, shows what the board actually sent.
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
        try:
            self._record("port closed")
        finally:
            self._port.close()


# Wraps reset to record when it was requested and what the probe reported.
def recorded_reset(reset, record):
    def recorded():
        record("reset asked of the probe")
        said = reset()
        record(f"reset done, the probe said: {said}")
        return said

    return recorded


# Resets the board through the probe and returns the probe's output. The
# probe fails about once in several thousand resets, so it is retried and each
# failure is reported with its output. The cause outlasts an immediate retry,
# so wait is called between attempts.
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


# Returns the console of a freshly reset board on a port just opened. If the
# board does not start, the port is closed so that the next run can open it.
def console_after_reset(port, reset, report):
    try:
        discard_until_quiet(port, _QUIET_SECONDS, _NEVER_QUIET_SECONDS)
        shown = reset_until_banner(port, reset, _BANNER_WAIT_SECONDS, _RESET_ATTEMPTS, report)
    except BaseException:
        port.close()
        raise
    return SerialConsole(port, reset, shown)


# The ST-LINK buffers what the board printed while the port was closed, which
# is a whole boot after the reset that ended the last scenario, and delivers
# it when the port opens. That banner would be mistaken for the next reset's,
# so input is discarded until nothing has arrived for quiet_seconds. Raises if
# the board is still printing after give_up_seconds.
def discard_until_quiet(port, quiet_seconds, give_up_seconds):
    started = time.monotonic()
    quiet_since = started
    while (time.monotonic() - quiet_since) < quiet_seconds:
        if port.read():
            quiet_since = time.monotonic()
        if (time.monotonic() - started) > give_up_seconds:
            raise RuntimeError(f"The board has not been quiet for {quiet_seconds} seconds in {give_up_seconds}")


# Resets the board, and retries if no banner appears within wait_seconds. The
# banner is the first thing a boot prints, so a board without one has not
# started. About once in a thousand probe resets the board stays silent, for a
# reason not yet found, so each occurrence is reported with the probe's
# output. Returns the output from the banner onwards. Anything before the
# banner belongs to the previous boot and is discarded, because its prompt
# would make a board that is still starting look ready.
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


# Adapts the port for SerialConsole: read returns whatever has arrived.
class _ArrivedBytes:
    def __init__(self, port):
        self._port = port

    def read(self):
        return self._port.read(self._port.in_waiting or 1)

    def write(self, data):
        self._port.write(data)

    def close(self):
        self._port.close()


# Returns the board's output up to its prompt. After a reset the prompt
# appears once the network has had time to come up. Raises, with the output
# so far, if there is no prompt within timeout_seconds.
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
        # Killing the console closes its output, which ends the reader.
        console.kill()
        reader.join()
        raise TimeoutError(
            f"The board showed no prompt within {timeout_seconds} seconds: {bytes(shown).decode(errors='replace')!r}"
        )
    return bytes(shown).decode(errors="replace")


# Sets the board's time from this machine's, as UTC seconds since the epoch.
# The board's real-time clock keeps it through every reset, so a run sets it
# once. The board treats the second it is given as just beginning, so the
# command is sent on a second boundary. Raises if the board refuses, which
# means its real-time clock has not started.
def tell_the_time(console, timeout_seconds, now=time.time, wait=time.sleep):
    wait(1.0 - (now() % 1.0))
    console.stdin.write(f"set time {round(now())}\n")
    answer = output_to_prompt(console, timeout_seconds)
    if "set: invalid" in answer:
        raise RuntimeError(f"The board did not take the time: {answer!r}")


# Leaves the board's disk holding no store, so that the next one starts
# empty. The disk is a card or a flash part and survives a reset, so without
# this a scenario would inherit the previous one's records. Raises if the
# board refuses, which means the medium did not answer or did not accept the
# change.
def wipe_the_disk(console, timeout_seconds):
    console.stdin.write("set store wipe\n")
    answer = output_to_prompt(console, timeout_seconds)
    if "set: invalid" in answer:
        raise RuntimeError(f"The board did not wipe its disk: {answer!r}")


# Switches the target to the file store on the disk. Raises if the board
# refuses, which means the disk would not mount.
def use_the_file_store(console, timeout_seconds):
    console.stdin.write("set store file\n")
    answer = output_to_prompt(console, timeout_seconds)
    if "set: invalid" in answer:
        raise RuntimeError(f"The board did not take the file store: {answer!r}")


# Returns what the store's medium has been asked to do since the target
# started it, as counts of erases, programs and reads. Returns None from an
# image that keeps no count, which refuses the command.
def store_report(console, timeout_seconds):
    console.stdin.write("set store report\n")
    counts = _STORE_REPORT.search(output_to_prompt(console, timeout_seconds))
    if not counts:
        return None
    return {"erases": int(counts.group(1)), "programs": int(counts.group(2)), "reads": int(counts.group(3))}


# Each image has its own name and build directory, so neither environment
# variable has a default.
def image_under_test():
    return _named_by_the_run("NUCLEO_F767ZI_IMAGE", "the image under test, as its build names it: SOLIDSYSLOG_BDD_TARGET")


def build_directory():
    return _named_by_the_run("NUCLEO_F767ZI_BUILD_DIR", "the directory the image under test was built in")


def _named_by_the_run(variable, meaning):
    value = os.environ.get(variable)
    if not value:
        raise RuntimeError(f"{variable} is not set to {meaning}")
    return value


# Raises unless the banner in output (the board's output from reset to
# prompt) names image and commit. All images built from one commit carry the
# same commit, so the image name is checked as well. The build writes the
# first ten digits of the commit, with "-dirty" appended for a tree with
# uncommitted changes.
def check_image(output, image, commit):
    banner = _BANNER.search(output)
    if not banner:
        raise RuntimeError(f"The board showed no banner after its reset: {output!r}")
    if banner.group(1) != image:
        raise RuntimeError(f"The image on the board is {banner.group(1)}, and the image under test is {image}")
    if banner.group(2) != commit[:10]:
        raise RuntimeError(
            f"The image on the board is of commit {banner.group(2)}, and the commit under test is {commit[:10]}"
        )


# Returns the address the image was built with, read from the build's
# CMakeCache.txt. The board sends it as its HOSTNAME.
def board_address_in(cache):
    address = _CACHED_ADDRESS.search(cache)
    if not address:
        raise RuntimeError("SOLIDSYSLOG_BDD_BOARD_IP is not in the build's CMakeCache.txt")
    return address.group(1)
