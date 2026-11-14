"""A sustained run against the NUCLEO-F767ZI: many messages over UDP, then as
many over TCP, and a check that the collector recorded every one of them once,
in order and as it was sent, with a time on it no earlier than the last.

Run by Run-NucleoF767ziBdd.ps1 -SustainedRun <count>, which builds and writes
the image, starts the collector, and names the board's serial port and the
programmer in the environment, as it does for Behave.

Each message is one `send 1` at the console, answered before the next is
asked for. A `send` of many would log them faster than the target sends them,
and its buffer would discard what it could not hold, which is the library's
doing and not a loss on the wire.
"""

import glob
import json
import os
import re
import subprocess
import sys
import time

from nucleo_f767zi import board_address_in, check_image_is_of_commit, open_board, output_to_prompt, tell_the_time

_BOOT_TIMEOUT_SECONDS = 60
_ANSWER_TIMEOUT_SECONDS = 30
# The collector writes what it has received a little after it receives it.
_RECORD_WAIT_SECONDS = 60
_RECORDS = {"udp": "Bdd/output/received_udp.jsonl", "tcp": "Bdd/output/received_tcp.jsonl"}

_SEQUENCE_ID = re.compile(r'sequenceId="(\d+)"')
_OCTET_COUNTED = re.compile(r"^(\d+) (.*)$", re.DOTALL)
_FAULTS_SHOWN = 10
_BUILD_DIR = os.path.join("build", "nucleo-f767zi-lwip")

# The message the target sends for `send 1`, as Bdd/Targets/Common has it, in
# the parts between what is not the same in every message: the board's address,
# which is its HOSTNAME, the time, the two numbers that count up, and the
# library's version. The run tells the board the time before it sends, so every
# message carries one, and says that its clock is set.
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
_TIME_SENT = re.compile(r"^<\d+>1 (" + _TIME + ") ")


# What a message sent by the board at hostname has to match, whole.
def message_as_sent(hostname):
    between = (_TIME, re.escape(hostname), _COUNT, _COUNT, _VERSION)
    pattern = re.escape(_SENT[0])
    for varying, literal in zip(between, _SENT[1:]):
        pattern += varying + re.escape(literal)
    return re.compile(pattern)


# The messages in lines, which are the collector's file exporter's: each is one
# JSON object holding the records of one write, and each record's body is the
# message as it arrived.
def messages_in(lines):
    messages = []
    for line in lines:
        if line.strip():
            for resource in json.loads(line).get("resourceLogs", []):
                for scope in resource.get("scopeLogs", []):
                    for record in scope.get("logRecords", []):
                        messages.append(record["body"]["stringValue"])
    return messages


# What is wrong with messages as the record of count messages sent one after
# another, the first with sequenceId first: nothing, for every message there
# once, in order, and matching sent, which is message_as_sent's. Over TCP a
# message arrives behind its octet count, which has to be its length. The
# messages are sent one after another, so the time on each is no earlier than
# on the one before: every digit of it is of fixed width, and so it is compared
# as it is written.
def faults_in(messages, first, count, sent):
    faults = []
    ids = []
    time_before = ""
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


def _order_of(ids, expected):
    wanted = set(expected)
    arrived = [sequence_id for sequence_id in ids if sequence_id in wanted]
    if len(arrived) != len(ids):
        return "with others among them"
    if len(set(arrived)) != len(arrived):
        return "with repeats"
    return "in order" if arrived == sorted(arrived) else "out of order"


# The lines of the record at path, with those of the files the exporter moved
# aside as it grew, oldest first.
def _lines_of(path):
    lines = []
    for name in sorted(glob.glob(path[: -len(".jsonl")] + "*.jsonl"), key=os.path.getmtime):
        with open(name, encoding="utf-8") as record:
            lines.extend(record.readlines())
    return lines


def _send(console, count):
    for _ in range(count):
        console.stdin.write("send 1\n")
        output_to_prompt(console, _ANSWER_TIMEOUT_SECONDS)


def _messages_once_arrived(path, count):
    deadline = time.monotonic() + _RECORD_WAIT_SECONDS
    messages = messages_in(_lines_of(path))
    while (len(messages) < count) and (time.monotonic() < deadline):
        time.sleep(1)
        messages = messages_in(_lines_of(path))
    return messages


def main(count):
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip()
    console = open_board(os.environ["NUCLEO_F767ZI_SERIAL_PORT"], os.environ.get("STM32_PROGRAMMER_CLI", "STM32_Programmer_CLI"))
    try:
        check_image_is_of_commit(output_to_prompt(console, _BOOT_TIMEOUT_SECONDS), commit)
        build_dir = os.environ.get("NUCLEO_F767ZI_BUILD_DIR") or _BUILD_DIR
        with open(os.path.join(build_dir, "CMakeCache.txt"), encoding="utf-8") as cache:
            sent = message_as_sent(board_address_in(cache.read()))
        tell_the_time(console, _ANSWER_TIMEOUT_SECONDS)
        _send(console, count)
        console.stdin.write("set transport tcp\n")
        output_to_prompt(console, _ANSWER_TIMEOUT_SECONDS)
        _send(console, count)
        failed = False
        for transport, first in (("udp", 1), ("tcp", count + 1)):
            messages = _messages_once_arrived(_RECORDS[transport], count)
            faults = faults_in(messages, first, count, sent)
            failed = failed or bool(faults)
            print(f"Sustained run, {transport.upper()}: {count} sent, {len(messages)} received, {len(faults)} faults")
            for fault in faults[:_FAULTS_SHOWN]:
                print(f"  {fault}")
        print(f"Commit under test: {commit}")
    finally:
        console.kill()
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(int(sys.argv[1])))
