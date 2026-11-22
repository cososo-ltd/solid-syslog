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

The part must also allow a byte that has been programmed to be programmed again
before its block is erased. The
[NorFlash](../../api/structSolidSyslogNorFlash.md) role requires it of any
flash, and says why. SFDP does not report it, so the pack cannot check: read
your part's datasheet.

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

An erase and a page program are each given a fixed time limit. One that
outlasts it fails and is reported as
[`SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED`](../../api/SolidSyslogNorFlashCategories_8h.md)
with a detail saying which.
[`SolidSyslogSpiNorFlash_Create`](../../api/SolidSyslogSpiNorFlash_8h.md)
states the limits, the other faults reported the same way, and what the pack
does not catch. A program or an erase that fails freezes the store
above, which keeps what it holds and does not wait on the part for ever; the
[NorFlash](../../api/structSolidSyslogNorFlash.md) contract states what a
failed read does.

That bound is the pack's. A call of your bus that never returns is outside it,
which is why the bus must bound its own time.

### The flash layer offers no confidentiality or tamper evidence

Bytes are programmed as given. Detecting modification of a stored record, or
keeping it unreadable, is the SecurityPolicy role's job, not this one - see
[at-rest cryptography](../../security/at-rest-cryptography.md).

### A store on raw flash wants a policy that checks each record

With no filesystem beneath the store, nothing below it notices a record that
power cut short as it was programmed. Give the store a SecurityPolicy that
checks each record, which is what detects one. The
[flash block device](../../api/SolidSyslogNorFlashBlockDevice_8h.md) says how
such a record comes about.

### What the pack has been tested against

The pack is unit tested on the host against a simulated part that answers with
the SFDP table read from a Winbond W25Q128JV, and behaves as a part does where
the pack could tell the difference: a program clears bits and sets none, a
program that runs past the end of its page wraps within it, nothing is written
unless writing was enabled first, and a busy part answers only a request for
its status.

On hardware it has run on one part, that W25Q128JV, on SPI at 6.75 MHz. There
it has carried the store scenarios, and 2000 messages over each of UDP and TCP,
through a store kept directly on the part, and the same beneath a filesystem on
the same part. [A store on NOR flash](../../roles/flash-store-costs.md) has
what those runs measured.

Not tested on the part:

- A loss of power during a program or an erase. That is tested on the host
  only, where the flash block device above the pack runs against a simulated
  flash whose power can be cut part way through either.
- The part's times at the ends of its temperature range, or late in its life.
- Any second part.

None of this is qualification against your part.

## If your part is not the one tested

There are more SPI NOR parts than a pack can be proved against, and they differ
in ways SFDP does not describe. Read this pack as a reference. It is designed
for a part within the limits
[`SolidSyslogSpiNorFlash_Create`](../../api/SolidSyslogSpiNorFlash_8h.md)
states, it has run on the one part above, and it is held to the same tests and
analysis as the rest of the library. What it does not carry is a claim about
your part.

Where your part needs something the pack does not do - another command, a
status read another way, a longer time for a program, four-byte addresses -
take the pack as the basis of a platform of your own and make the change there.
What a flash has to do for the store is the
[NorFlash](../../api/structSolidSyslogNorFlash.md) role: five functions, of
which this pack is one filling.

Whether you use the pack as it stands or change it, check it against your part
before you rely on it:

- The block size and block count the flash reports agree with the part's
  datasheet.
- A block erases, and then reads as `0xFF` throughout.
- A program that crosses the end of a page, and one of part of a page, read
  back as written.
- A byte already programmed takes a second program that clears more of its
  bits.
- The longest erase and page program times the datasheet gives are within the
  times the pack allows.
- A processor reset during an erase is followed by a Create that waits for the
  part and succeeds.
- With power removed during a program and during an erase, the store starts
  again on what is left, under the SecurityPolicy you mean to ship.
- A run at the rate you expect to log, long enough for the store to have used
  every block you gave it.
