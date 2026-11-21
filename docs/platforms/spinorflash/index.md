# SPI NOR flash

`Platform/SpiNorFlash/` drives an SPI NOR flash part over a bus you supply, so a
store's blocks can be kept on the part itself with no filesystem between them. It
learns the part from the part's own SFDP table (JESD216), so it is not written for
one maker's parts.

Fills the [NorFlash](../../api/structSolidSyslogNorFlash.md) role - the flash
beneath a [NorFlashBlockDevice](../../api/SolidSyslogNorFlashBlockDevice_8h.md).

## What it ships

## Requirements

A part that publishes an SFDP basic parameter table, erases the same way
throughout, takes three-byte addresses, holds no more than 16 MiB, and programs
in pages of 256 bytes or a multiple of that. The pack uses the plain SPI
commands only: read data `03h`, page program `02h`, read status `05h`, write
enable `06h`, read SFDP `5Ah`, and the erase command the part's table lists for
its smallest erase.

A bus to reach it over: the five functions of
[`SolidSyslogSpiNorFlashBus`](../../api/structSolidSyslogSpiNorFlashBus.md), which
[setting it up](setup.md) describes. The pack needs nothing of the toolchain
beyond C99, and nothing of an operating system.

## Security behaviour and obligations

### A part it cannot drive is refused at Create

The pack reads the basic parameter table and no other. A part outside what it
drives is refused when the flash is created: Create reports why and answers the
Null flash, so a store built on it holds nothing and says so.
[`SolidSyslogSpiNorFlash_Create`](../../api/SolidSyslogSpiNorFlash_8h.md) lists
every refusal, and the three things the pack does not check.

### Write protection is yours

The pack neither looks for write protection nor changes it: it never writes a
status register. Leave unprotected the blocks you give the store, and protect
the rest of the part as you see fit.
[`SolidSyslogSpiNorFlash_Create`](../../api/SolidSyslogSpiNorFlash_8h.md) says
what becomes of a write to a block the part protects.

### A part that stops answering costs a bounded wait

An erase and a page program are each given a fixed time limit, which
[`SolidSyslogSpiNorFlash_Create`](../../api/SolidSyslogSpiNorFlash_8h.md)
states. One that outlasts it fails, as does any operation on a part that will
not enable writing or whose line is no longer driven, and each is reported as
[`SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED`](../../api/SolidSyslogNorFlashCategories_8h.md)
with a detail saying which. A program or an erase that fails freezes the store
above, which keeps what it holds and does not wait on the part for ever; the
[NorFlash](../../api/structSolidSyslogNorFlash.md) contract states what a
failed read does.

That bound is the pack's. A call of your bus that never returns is outside it,
which is why the bus must bound its own time.

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
