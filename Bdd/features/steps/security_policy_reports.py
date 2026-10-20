"""Reading what the BDD target's keyed security policy reported, for the steps that assert on it."""

import re

from tls_reports import reported_output


def reported_keys_unavailable(process):
    """Every key-unavailable report a security policy has made, as (severity, detail).

    Any source: the OpenSSL and Mbed TLS policies name themselves differently,
    and the category is what the step asserts on."""
    pattern = re.compile(r"severity=(\d+) \[\S+ cat=\d+ detail=(-?\d+)\] SECURITY_POLICY_KEY_UNAVAILABLE")
    return [(int(severity), int(detail)) for severity, detail in pattern.findall(reported_output(process))]
