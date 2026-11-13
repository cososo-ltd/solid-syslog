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
_RESET_TIMEOUT_SECONDS = 30
_QUIET_SECONDS = 0.5
# The banner follows a reset at once; this is far longer than it has ever taken.
_BANNER_WAIT_SECONDS = 5
_RESET_ATTEMPTS = 3

_BANNER_COMMIT = re.compile(r"NUCLEO-F767ZI, commit (\S+)")
_CACHED_ADDRESS = re.compile(r"^SOLIDSYSLOG_BDD_BOARD_IP:\w+=(\S+)$", re.MULTILINE)


# Resets the board and returns its console, opened first so that the banner
# the reset draws is on it. port_name is the ST-LINK's virtual COM port, and
# programmer is STM32_Programmer_CLI, by path or as PATH finds it.
def open_board(port_name, programmer):
    # Imported here so that only a run against the board needs pyserial.
    import serial

    # Returns what the probe said, for a report of a reset that did not take.
    def reset():
        done = subprocess.run(
            [programmer, "-c", "port=SWD", "-hardRst"],
            check=True, capture_output=True, timeout=_RESET_TIMEOUT_SECONDS,
        )
        said = done.stdout.decode(errors="replace").splitlines()
        return " | ".join(line.strip() for line in said if line.strip())

    port = _ArrivedBytes(serial.Serial(port_name, CONSOLE_BAUD, timeout=_READ_WAIT_SECONDS))
    discard_until_quiet(port, _QUIET_SECONDS)
    reset_until_board_speaks(
        port, reset, _BANNER_WAIT_SECONDS, _RESET_ATTEMPTS,
        lambda report: print(report, file=sys.__stderr__, flush=True),
    )
    return SerialConsole(port, reset)


# Resets the board, and again if it shows nothing: the banner is the first
# thing a reset draws, so a board silent for wait_seconds after one has not
# started. Once in about a thousand resets through the probe the board has
# stayed silent, for a reason not yet found, so each time is reported, with
# what the probe said of the reset, rather than passed over. What the board
# shows is left on the port for the console.
def reset_until_board_speaks(port, reset, wait_seconds, attempts, report):
    spoke = False
    attempt = 0
    while (not spoke) and (attempt < attempts):
        attempt += 1
        said = reset()
        deadline = time.monotonic() + wait_seconds
        while (not port.has_arrived()) and (time.monotonic() < deadline):
            time.sleep(0.01)
        spoke = port.has_arrived()
        if (not spoke) and (attempt < attempts):
            report(
                f"NUCLEO-F767ZI: the board showed nothing within {wait_seconds} seconds of reset {attempt},"
                f" and is reset again. The probe said: {said}"
            )
    if not spoke:
        raise RuntimeError(f"The board showed nothing after any of {attempts} resets. The probe last said: {said}")


# The ST-LINK keeps what the board printed while the port was closed - the
# whole of a boot, after the reset that ended the last scenario - and hands it
# over once the port is open. Read as this boot's, its prompt would be taken
# for a board that is ready while the board is still starting, so it is
# discarded, until nothing has arrived for quiet_seconds.
def discard_until_quiet(port, quiet_seconds):
    quiet_since = time.monotonic()
    while (time.monotonic() - quiet_since) < quiet_seconds:
        if port.read():
            quiet_since = time.monotonic()


# The port as SerialConsole reads it: what has arrived, without waiting for more.
class _ArrivedBytes:
    def __init__(self, port):
        self._port = port

    def read(self):
        return self._port.read(self._port.in_waiting or 1)

    def has_arrived(self):
        return self._port.in_waiting > 0

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
