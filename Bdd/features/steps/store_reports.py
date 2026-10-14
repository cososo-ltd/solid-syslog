"""Reading what the BDD target's block store reported, for the steps that assert on it."""

import re

from tls_reports import reported_output


def _block_store_reports(process, text):
    """Every report the block store has made with the given text, as (severity, detail)."""
    pattern = re.compile(r"severity=(\d+) \[BlockStore cat=\d+ detail=(-?\d+)\] " + re.escape(text))
    return [(int(severity), int(detail)) for severity, detail in pattern.findall(reported_output(process))]


def reported_store_freezes(process):
    """Every frozen-store report the block store has made, as (severity, detail)."""
    return _block_store_reports(process, "store frozen")


def reported_store_discards(process):
    """Every discarded-record report the block store has made, as (severity, detail)."""
    return _block_store_reports(process, "store record discarded")


def reported_store_losses(process):
    """Every report of records lost to the discard policy, as (severity, detail)."""
    return _block_store_reports(process, "store records lost")
