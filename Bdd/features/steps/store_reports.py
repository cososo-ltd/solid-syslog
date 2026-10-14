"""Reading what the BDD target's block store reported, for the steps that assert on it.

The streams' own logs keep only their most recent output, and a target retrying
a collector that is down fills that window with connect failures in seconds,
pushing out a store report made at the start of the outage. Each stream
therefore also keeps every line the block store reports, which is little,
since the store reports a change of state rather than each occurrence.
"""

import re

_STORE_REPORT_STREAMS = ("_solidsyslog_stdout_store_reports", "_solidsyslog_stderr_store_reports")


class StoreReportRecorder:
    """Keeps every block-store line from one stream, however much else the stream says."""

    def __init__(self):
        self._partial = bytearray()
        self.lines = []

    def feed(self, data):
        # The stdout reader feeds a byte at a time, so a line is split out only
        # once its newline arrives.
        self._partial += data
        if b"\n" in data:
            *complete, rest = bytes(self._partial).split(b"\n")
            self._partial = bytearray(rest)
            self.lines.extend(line for line in complete if b"[BlockStore " in line)


def _store_report_output(process):
    """Every block-store line the target has written, whichever stream it wrote it on."""
    lines = []
    for attribute in _STORE_REPORT_STREAMS:
        recorder = getattr(process, attribute, None)
        if recorder is not None:
            lines.extend(line.decode("utf-8", errors="replace") for line in recorder.lines)
    return "\n".join(lines)


def _block_store_reports(process, text):
    """Every report the block store has made with the given text, as (severity, detail)."""
    pattern = re.compile(r"severity=(\d+) \[BlockStore cat=\d+ detail=(-?\d+)\] " + re.escape(text))
    return [(int(severity), int(detail)) for severity, detail in pattern.findall(_store_report_output(process))]


def reported_store_freezes(process):
    """Every frozen-store report the block store has made, as (severity, detail)."""
    return _block_store_reports(process, "store frozen")


def reported_store_discards(process):
    """Every discarded-record report the block store has made, as (severity, detail)."""
    return _block_store_reports(process, "store record discarded")


def reported_store_losses(process):
    """Every report of records lost to the discard policy, as (severity, detail)."""
    return _block_store_reports(process, "store records lost")
