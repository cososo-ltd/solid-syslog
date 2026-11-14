"""A sustained run against the NUCLEO-F767ZI: many messages over UDP, then as
many over TCP, and a check that the collector recorded every one of them once,
in order and unchanged.

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

from nucleo_f767zi import check_image_is_of_commit, open_board, output_to_prompt

_BOOT_TIMEOUT_SECONDS = 60
_ANSWER_TIMEOUT_SECONDS = 30
# The collector writes what it has received a little after it receives it.
_RECORD_WAIT_SECONDS = 60
_RECORDS = {"udp": "Bdd/output/received_udp.jsonl", "tcp": "Bdd/output/received_tcp.jsonl"}

_SEQUENCE_ID = re.compile(r'sequenceId="(\d+)"')
# What differs from one message to the next, and nothing else may.
_VARIES = re.compile(r'(sequenceId|sysUpTime)="\d+"')
_OCTET_COUNTED = re.compile(r"^(\d+) (.*)$", re.DOTALL)
_FAULTS_SHOWN = 10


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
# once, in order, and the same as the first but for what counts up. Over TCP a
# message arrives behind its octet count, which has to be its length.
def faults_in(messages, first, count):
    faults = []
    ids = []
    pattern = None
    for message in messages:
        counted = _OCTET_COUNTED.match(message)
        if counted:
            message = counted.group(2)
            if int(counted.group(1)) != len(message.encode("utf-8")):
                faults.append(f"an octet count of {counted.group(1)} on a message of another length: {message!r}")
        sequence_id = _SEQUENCE_ID.search(message)
        ids.append(int(sequence_id.group(1)) if sequence_id else None)
        shape = _VARIES.sub(r'\1="N"', message)
        if pattern is None:
            pattern = shape
        elif shape != pattern:
            faults.append(f"a message unlike the first: {message!r}")
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
        _send(console, count)
        console.stdin.write("set transport tcp\n")
        output_to_prompt(console, _ANSWER_TIMEOUT_SECONDS)
        _send(console, count)
        failed = False
        for transport, first in (("udp", 1), ("tcp", count + 1)):
            messages = _messages_once_arrived(_RECORDS[transport], count)
            faults = faults_in(messages, first, count)
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
