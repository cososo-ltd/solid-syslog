"""Reading what the BDD target's TCP stream and its sender reported about an outage.

Keyed on the category name the handler prints rather than on a source name or
a role mark: every TCP backend reports STREAM_CONNECT_FAILED under its own
source, and every target's handler prints the category by name, so one reader
serves every lane.
"""

import re

from tls_reports import reported_output

_CONNECT_FAILED = re.compile(r"severity=(\d+) \[\S+ cat=\d+ detail=(-?\d+)\] STREAM_CONNECT_FAILED")
_DELIVERY_RESTORED = re.compile(r"severity=(\d+) \[\S+ cat=\d+ detail=(-?\d+)\] SENDER_DELIVERY_RESTORED")


def reported_connect_failures(process):
    """Every failed connect the stream has reported, as (severity, detail)."""
    return [(int(severity), int(detail)) for severity, detail in _CONNECT_FAILED.findall(reported_output(process))]


def reported_delivery_restorations(process):
    """Every recovery the sender has reported, as (severity, detail)."""
    return [
        (int(severity), int(detail)) for severity, detail in _DELIVERY_RESTORED.findall(reported_output(process))
    ]
