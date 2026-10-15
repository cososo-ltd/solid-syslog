"""Reading what the BDD target reported, for the steps that assert on it.

Where the report is read from differs by target and cannot be helped. On Linux
and Windows the error handler writes to stderr; on FreeRTOS `_write` ignores the
file descriptor (Syscalls.c) so it lands on the same UART as the prompt
protocol. Both captured streams are searched.

Each stream keeps two things. Its log holds only the latest 16 KB, for a failure's
diagnostics; a target retrying a collector that is down fills that with connect
failures in a second or two, pushing out a report made at the start of the
outage. So each stream also records every line the error handler wrote, and the
reports are read from that record, never from the log. A step asserting that a
report was made once, or never, then sees every report the target made.

Detail codes are per-class, so a bare `detail=` is ambiguous: a resolver fault
reporting 16 would be indistinguishable from PEER_CERTIFICATE_UNTRUSTED. Both
handlers mark a report from the TLS-stream role, and only those are read here.
Filtering on the mark rather than on the source name keeps this pack-agnostic:
the two names share no pattern that excludes PosixTcpStream or StreamSender,
and a third TLS pack would otherwise have to be added to this regex.
"""

import re

_REPORT = re.compile(r"(?:severity=(\d+) )?\[[^\]]*role=tls cat=\d+ detail=(-?\d+)")

# The two forms an error handler writes a report in: the ordinary one, and the
# one a hosted target prints as it exits on an ERROR.
_REPORT_LINE_MARKERS = (b"[solidsyslog] severity=", b"BDD-TARGET: FATAL: [")

_REPORT_RECORDS = ("_solidsyslog_stdout_reports", "_solidsyslog_stderr_reports")


class ReportRecorder:
    """Keeps every report line from one stream, however much else the stream says."""

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
            self.lines.extend(
                line for line in complete if any(marker in line for marker in _REPORT_LINE_MARKERS)
            )


def target_output(process):
    """The latest of what the target has said, whichever stream it said it on, for diagnostics."""
    if process is None:
        return ""

    chunks = []
    for attribute in ("_solidsyslog_stdout_log", "_solidsyslog_stderr_log"):
        log = getattr(process, attribute, None)
        if log:
            chunks.append(bytes(log).decode("utf-8", errors="replace"))
    return "\n".join(chunks)


def reported_output(process):
    """Every report the target has made, whichever stream it made it on."""
    if process is None:
        return ""

    lines = []
    for attribute in _REPORT_RECORDS:
        recorder = getattr(process, attribute, None)
        if recorder is not None:
            lines.extend(line.decode("utf-8", errors="replace") for line in recorder.lines)
    return "\n".join(lines)


def reported_reports(process):
    """Every TLS-stream report so far, as (severity, detail).

    The severity is None for a report the target treated as fatal - that form
    names no severity because reaching it is what ERROR means to the handler.
    """
    return [
        (int(severity) if severity else None, int(detail))
        for severity, detail in _REPORT.findall(reported_output(process))
    ]


_DELIVERY = re.compile(r"severity=(\d+) \[StreamSender cat=(\d+) detail=(-?\d+)\]")


def reported_delivery_faults(process):
    """Every delivery-health report the sender has made, as (severity, detail).

    A different source from the TLS-stream reports above, and deliberately so:
    the peer tore the connection down behind a write that had already
    succeeded, which is the sender's fault to report rather than the stream's.
    """
    return [
        (int(severity), int(detail))
        for severity, _category, detail in _DELIVERY.findall(reported_output(process))
    ]


def reported_details(process):
    """Every TLS-stream detail code the target has reported so far."""
    return [detail for _, detail in reported_reports(process)]
