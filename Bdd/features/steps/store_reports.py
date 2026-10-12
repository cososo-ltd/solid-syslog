"""Reading what the BDD target's block store reported, for the steps that assert on it."""

import re

from tls_reports import target_output

_STORE_FROZEN = re.compile(r"severity=(\d+) \[BlockStore cat=\d+ detail=(-?\d+)\] store frozen")


def reported_store_freezes(process):
    """Every frozen-store report the block store has made, as (severity, detail)."""
    return [
        (int(severity), int(detail))
        for severity, detail in _STORE_FROZEN.findall(target_output(process))
    ]
