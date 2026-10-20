"""Reading what the BDD target's buffer reported, for the steps that assert on it."""

import re

from tls_reports import reported_output

# The buffer behind the logger on each hosted target, and the logger itself,
# which reports under the buffer's category when it drops the record it holds
# because the buffer filled behind it.
_BUFFER_SOURCES = "CircularBuffer|PosixMessageQueueBuffer|SolidSyslog"


def reported_buffer_losses(process):
    """Every report of records lost at the buffer, as (severity, source, detail)."""
    pattern = re.compile(
        r"severity=(\d+) \[(" + _BUFFER_SOURCES + r") cat=\d+ detail=(-?\d+)\] BUFFER_RECORDS_LOST"
    )
    return [
        (int(severity), source, int(detail))
        for severity, source, detail in pattern.findall(reported_output(process))
    ]
