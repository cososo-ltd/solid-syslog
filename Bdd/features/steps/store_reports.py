"""Reading what the BDD target's block store reported, for the steps that assert on it."""

import re

from tls_reports import reported_output


def _block_store_reports(process, text):
    """Every report the block store has made under the named category, as (severity, detail)."""
    pattern = re.compile(r"severity=(\d+) \[BlockStore cat=\d+ detail=(-?\d+)\] " + re.escape(text))
    return [(int(severity), int(detail)) for severity, detail in pattern.findall(reported_output(process))]


def reported_store_freezes(process):
    """Every frozen-store report the block store has made, as (severity, detail)."""
    return _block_store_reports(process, "STORE_FROZEN")


def reported_store_discards(process):
    """Every discarded-record report the block store has made, as (severity, detail)."""
    return _block_store_reports(process, "STORE_RECORD_DISCARDED")


def reported_store_losses(process):
    """Every report of records lost to the discard policy, as (severity, detail)."""
    return _block_store_reports(process, "STORE_RECORDS_LOST")
