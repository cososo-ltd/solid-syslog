"""Reading the logger's reports of its own events out of what the collector received.

A report is an ordinary record carrying the library's own SD element, which
names the event on the same axes the error handler sees: the source that
raised it, its category, its detail, and the platform's native code where
there is one. The record's PRIVAL carries the event's severity under the
facility the library fixes for its reports, which is read from the header
that defines it for the reason syslog_severities.py gives.
"""

import pathlib
import re

_REPO_ROOT = pathlib.Path(__file__).resolve().parents[3]
_HEADER = _REPO_ROOT / "Core" / "Interface" / "SolidSyslogPrival.h"

_FACILITY = re.compile(r"\bSOLIDSYSLOG_FACILITY_SYSLOG\b\s*=\s*(\d+)")
_facility_match = _FACILITY.search(_HEADER.read_text(encoding="utf-8"))
if _facility_match is None:
    raise RuntimeError(f"No SOLIDSYSLOG_FACILITY_SYSLOG in {_HEADER}")

# The facility the library gives its own reports.
REPORT_FACILITY = int(_facility_match.group(1))

_LIBRARY_ELEMENT = re.compile(r"\[solidsyslog@67478 ([^\]]*)\]")
_PARAM = re.compile(r'(\w+)="([^"]*)"')


def is_library_report(fields):
    """Whether a parsed oracle record is one the logger made of its own events."""
    return _LIBRARY_ELEMENT.search(fields.get("STRUCTURED_DATA", "")) is not None


def library_report(fields):
    """The event a report names, as a dict of the element's params plus the
    record's facility, severity and MSGID; None for a record the client logged.
    A record the oracle parsed without a priority has neither facility nor
    severity, so a step's check on either fails on the record rather than here."""
    element = _LIBRARY_ELEMENT.search(fields.get("STRUCTURED_DATA", ""))
    if element is None:
        return None
    report = dict(_PARAM.findall(element.group(1)))
    priority = fields.get("PRIORITY")
    if priority is not None:
        prival = int(priority)
        report["facility"] = prival // 8
        report["severity"] = prival % 8
    report["msgid"] = fields.get("MSGID", "")
    return report


def reports_of(reports, category):
    """The reports of one category, in arrival order."""
    return [report for report in reports if report.get("category") == category]
