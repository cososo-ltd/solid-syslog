"""A sustained run against the NUCLEO-F767ZI. Sends many messages over UDP,
then as many over TCP, and checks that the collector recorded each one exactly
once, in order, as sent, and with a timestamp no earlier than the previous one.

Run by Run-NucleoF767ziBdd.ps1 -SustainedRun <count>, which builds and flashes
the image, starts the collector, and passes the serial port and programmer in
the environment, as it does for Behave. With -FileStore every message goes
through the file store on the image's medium, which is wiped first.

Each message is a separate `send 1` at the console, answered before the next.
A single `send` of many would log faster than the target can send, and the
target's buffer would discard the excess. That is the library working as
designed and would be misread as loss on the wire.
"""

import glob
import json
import os
import re
import subprocess
import sys
import time

from nucleo_f767zi import (
    board_address_in,
    build_directory,
    check_image,
    image_under_test,
    open_board,
    output_to_prompt,
    store_report,
    tell_the_time,
    use_the_file_store,
    wipe_the_disk,
)

_BOOT_TIMEOUT_SECONDS = 60
_ANSWER_TIMEOUT_SECONDS = 30
# The collector writes its records a little after it receives them.
_RECORD_WAIT_SECONDS = 60
_RECORDS = {"udp": "Bdd/output/received_udp.jsonl", "tcp": "Bdd/output/received_tcp.jsonl"}

_SEQUENCE_ID = re.compile(r'sequenceId="(\d+)"')
_OCTET_COUNTED = re.compile(r"^(\d+) (.*)$", re.DOTALL)
_FAULTS_SHOWN = 10

# The message the target sends for `send 1`, as Bdd/Targets/Common builds it,
# split at the fields that vary: the timestamp, the board's address (its
# HOSTNAME), the two counters and the library's version. The run sets the
# board's time first, so every message carries a timestamp and reports its
# clock as set.
_SENT = (
    "<134>1 ",
    " ",
    ' SolidSyslogBddTarget - example [meta sequenceId="',
    '" sysUpTime="',
    '" language="en-GB"][timeQuality tzKnown="1" isSynced="1"][origin software="SolidSyslogBddTarget" swVersion="',
    '" enterpriseId="32473" ip="192.0.2.1"] ﻿Hello from SolidSyslog',
)
_TIME = r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\.\d{6}Z"
_COUNT = r"\d+"
_VERSION = r"\d+\.\d+\.\d+"
_TIME_SENT = re.compile(r"^(?:\d+ )?<\d+>1 (" + _TIME + ") ")


# Returns the pattern a whole message from the board at hostname must match.
def message_as_sent(hostname):
    between = (_TIME, re.escape(hostname), _COUNT, _COUNT, _VERSION)
    pattern = re.escape(_SENT[0])
    for varying, literal in zip(between, _SENT[1:]):
        pattern += varying + re.escape(literal)
    return re.compile(pattern)


# Returns the messages in lines written by the collector's file exporter.
# Each line is one JSON object holding the records of one write, and each
# record's body is a message as it arrived.
def messages_in(lines):
    messages = []
    for line in lines:
        if line.strip():
            for resource in json.loads(line).get("resourceLogs", []):
                for scope in resource.get("scopeLogs", []):
                    for record in scope.get("logRecords", []):
                        messages.append(record["body"]["stringValue"])
    return messages


# Returns the faults in messages, given that count messages were sent in turn
# starting at sequenceId first. There are none when every message is present
# once, in order, and matches sent (a pattern from message_as_sent). Over TCP
# each message follows its octet count, which must equal its length. Each
# timestamp must be no earlier than the one before; timestamps are fixed
# width, so they are compared as strings. The first is also checked against
# not_before, the timestamp of the last message sent before these.
def faults_in(messages, first, count, sent, not_before=""):
    faults = []
    ids = []
    time_before = not_before
    for message in messages:
        counted = _OCTET_COUNTED.match(message)
        if counted:
            message = counted.group(2)
            if int(counted.group(1)) != len(message.encode("utf-8")):
                faults.append(f"an octet count of {counted.group(1)} on a message of another length: {message!r}")
        sequence_id = _SEQUENCE_ID.search(message)
        ids.append(int(sequence_id.group(1)) if sequence_id else None)
        if not sent.fullmatch(message):
            faults.append(f"a message that is not the one sent: {message!r}")
        time_sent = _TIME_SENT.match(message)
        if time_sent:
            if time_sent.group(1) < time_before:
                faults.append(f"a time before that of the message before it: {message!r}")
            time_before = time_sent.group(1)
    expected = list(range(first, first + count))
    if ids != expected:
        arrived = set(ids)
        missing = [sequence_id for sequence_id in expected if sequence_id not in arrived]
        faults.append(
            f"{len(ids)} messages for the {count} sent, {len(missing)} of them missing"
            f" (the first: {missing[:_FAULTS_SHOWN]}), and the rest {_order_of(ids, expected)}"
        )
    return faults


# Returns the last timestamp in messages, or "" if none carries one.
def last_time_in(messages):
    times = [time_sent.group(1) for time_sent in map(_TIME_SENT.match, messages) if time_sent]
    return times[-1] if times else ""


def _order_of(ids, expected):
    wanted = set(expected)
    arrived = [sequence_id for sequence_id in ids if sequence_id in wanted]
    if len(arrived) != len(ids):
        return "with others among them"
    if len(set(arrived)) != len(arrived):
        return "with repeats"
    return "in order" if arrived == sorted(arrived) else "out of order"


# Returns the lines of the record at path, including the files the exporter
# rotated out as it grew, oldest first.
def _lines_of(path):
    lines = []
    for name in sorted(glob.glob(path[: -len(".jsonl")] + "*.jsonl"), key=os.path.getmtime):
        with open(name, encoding="utf-8") as record:
            lines.extend(record.readlines())
    return lines


# Returns the line that says what count messages cost the store's medium,
# from its counts before the first and after the last.
def cost_line(before, after, count):
    erases, programs, reads = (after[kind] - before[kind] for kind in ("erases", "programs", "reads"))
    return f"Sustained run, medium: {erases} erases, {programs} programs and {reads} reads for {count} messages"


def _send(console, count, interval_seconds):
    for _ in range(count):
        console.stdin.write("send 1\n")
        output_to_prompt(console, _ANSWER_TIMEOUT_SECONDS)
        time.sleep(interval_seconds)


def _messages_once_arrived(path, count):
    deadline = time.monotonic() + _RECORD_WAIT_SECONDS
    messages = messages_in(_lines_of(path))
    while (len(messages) < count) and (time.monotonic() < deadline):
        time.sleep(1)
        messages = messages_in(_lines_of(path))
    return messages


def main(count, store, file_store_interval_seconds):
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    console = open_board(os.environ["NUCLEO_F767ZI_SERIAL_PORT"], os.environ.get("STM32_PROGRAMMER_CLI", "STM32_Programmer_CLI"))
    try:
        check_image(output_to_prompt(console, _BOOT_TIMEOUT_SECONDS), image_under_test(), commit)
        with open(os.path.join(build_directory(), "CMakeCache.txt"), encoding="utf-8") as cache:
            sent = message_as_sent(board_address_in(cache.read()))
        tell_the_time(console, _ANSWER_TIMEOUT_SECONDS)
        interval_seconds = 0
        medium_before = None
        if store == "file":
            wipe_the_disk(console, _ANSWER_TIMEOUT_SECONDS)
            use_the_file_store(console, _ANSWER_TIMEOUT_SECONDS)
            interval_seconds = file_store_interval_seconds
            medium_before = store_report(console, _ANSWER_TIMEOUT_SECONDS)
        _send(console, count, interval_seconds)
        if store == "file":
            # The console answers `send 1` before the message has left the
            # store, and a message still stored when the transport changes
            # would be sent over the new one.
            _messages_once_arrived(_RECORDS["udp"], count)
        console.stdin.write("set transport tcp\n")
        output_to_prompt(console, _ANSWER_TIMEOUT_SECONDS)
        _send(console, count, interval_seconds)
        failed = False
        not_before = ""
        for transport, first in (("udp", 1), ("tcp", count + 1)):
            messages = _messages_once_arrived(_RECORDS[transport], count)
            faults = faults_in(messages, first, count, sent, not_before)
            not_before = last_time_in(messages) or not_before
            failed = failed or bool(faults)
            print(f"Sustained run, {transport.upper()}: {count} sent, {len(messages)} received, {len(faults)} faults")
            for fault in faults[:_FAULTS_SHOWN]:
                print(f"  {fault}")
        # Only an image whose medium is a flash counts what it was asked, and
        # a report that cannot be read costs the run its cost line, not its
        # verdict.
        medium_after = store_report(console, _ANSWER_TIMEOUT_SECONDS) if medium_before else None
        if medium_after:
            print(cost_line(medium_before, medium_after, 2 * count))
        print(f"Commit under test: {commit}")
    finally:
        console.kill()
    return 1 if failed else 0


# Returns the count, the store and the interval from the command line:
# <count> [null|file [<seconds between file-store messages>]]. A file store
# requires an interval, because the medium is slower than the console and the
# target's buffer would otherwise fill and discard. The rate depends on the
# medium, so there is no default.
def arguments_in(argv):
    count = int(argv[1])
    store = argv[2] if len(argv) > 2 else "null"
    if store != "file":
        return count, store, 0
    if len(argv) <= 3:
        raise ValueError("A file-store run needs the seconds between messages: the run script has each medium's")
    return count, store, float(argv[3])


if __name__ == "__main__":
    sys.exit(main(*arguments_in(sys.argv)))
