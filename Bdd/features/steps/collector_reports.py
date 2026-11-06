"""Reading the logger's reports of its own events out of what the collector received.

A report is an ordinary record carrying the library's own SD element, which
names the event on the same axes the error handler sees: the source that
raised it, its category, its detail, and the platform's native code where
there is one. The record's PRIVAL carries the event's severity under the
facility the library fixes for its reports.
"""

import re

_LIBRARY_ELEMENT = re.compile(r"\[solidsyslog@67478 ([^\]]*)\]")
_PARAM = re.compile(r'(\w+)="([^"]*)"')

# RFC 5424 facility 5, "messages generated internally by syslogd".
REPORT_FACILITY = 5


def is_library_report(fields):
    """Whether a parsed oracle record is one the logger made of its own events."""
    return _LIBRARY_ELEMENT.search(fields.get("STRUCTURED_DATA", "")) is not None


def library_report(fields):
    """The event a report names, as a dict of the element's params plus the
    record's facility, severity and MSGID; None for a record the client logged."""
    element = _LIBRARY_ELEMENT.search(fields.get("STRUCTURED_DATA", ""))
    if element is None:
        return None
    report = dict(_PARAM.findall(element.group(1)))
    prival = int(fields["PRIORITY"])
    report["facility"] = prival // 8
    report["severity"] = prival % 8
    report["msgid"] = fields.get("MSGID", "")
    return report


def reports_of(reports, category):
    """The reports of one category, in arrival order."""
    return [report for report in reports if report.get("category") == category]
