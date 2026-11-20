# SPI NOR flash

`Platform/SpiNorFlash/` drives a SPI NOR flash part over a bus you supply, so a
store's blocks can be kept on the part itself with no filesystem between them. It
learns the part from the part's own SFDP table (JESD216), so it is not written for
one maker's parts.

Fills the [NorFlash](../../api/structSolidSyslogNorFlash.md) role - the flash
beneath a [NorFlashBlockDevice](../../api/SolidSyslogNorFlashBlockDevice_8h.md).

## What it ships

## Requirements

A part that publishes an SFDP basic parameter table, takes three-byte addresses
and holds no more than 16 MiB. The pack uses the plain SPI commands only: read
data `03h`, page program `02h`, read status `05h`, write enable `06h`, read
identification `9Fh`, read SFDP `5Ah`, and the erase command the part's table
lists for its smallest erase.

A bus to reach it over: the five functions of
[`SolidSyslogSpiNorFlashBus`](../../api/structSolidSyslogSpiNorFlashBus.md), which
[setting it up](setup.md) describes. The pack needs nothing of the toolchain
beyond C99, and nothing of an operating system.

## Security behaviour and obligations

### A part it cannot drive is refused at Create

The pack reads the basic parameter table and no other. A part with no table, one
larger than 16 MiB, and one that takes only four-byte addresses are each refused
when the flash is created: Create reports why and answers the Null flash, so a
store built on it holds nothing and says so.
[`SolidSyslogSpiNorFlash_Create`](../../api/SolidSyslogSpiNorFlash_8h.md) lists
every refusal and what an older table's missing fields default to.

### It never unlocks a part

A part with any of bits 2 to 6 of its first status register set is protected
against writing, in part or in whole, and is refused at Create. The pack never
writes a status register, so clearing the protection is a decision you take, in
your own code, before you create the flash.

### The flash layer offers no confidentiality or tamper evidence

Bytes are programmed as given. Detecting modification of a stored record, or
keeping it unreadable, is the SecurityPolicy role's job, not this one - see
[at-rest cryptography](../../security/at-rest-cryptography.md).

### What the pack has been tested against

The pack is unit tested on the host against a simulated part that answers with
the SFDP table read from a Winbond W25Q128JV, and behaves as a part does where
the pack could tell the difference: a program clears bits and sets none, a
program that runs past the end of its page wraps within it, nothing is written
unless writing was enabled first, and a busy part answers only a request for
its status.

None of this is qualification against your part.
