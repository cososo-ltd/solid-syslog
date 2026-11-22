# A store on NOR flash, with a filesystem or without

A store on a NOR flash part can be built two ways. A
[File](../api/structSolidSyslogFile.md) backend puts each store block in a file
of a filesystem that runs on the part. A
[NorFlash](../api/structSolidSyslogNorFlash.md) backend puts each store block
in one of the part's own erase blocks, with no filesystem between them.

This page sets the two side by side on one part, so that an integrator free to
choose has something to choose by. It compares
[LittleFS](../platforms/littlefs/index.md), the filesystem this library is
tested on over NOR flash, with the
[SPI NOR flash](../platforms/spinorflash/index.md) pack under Core's
[flash block device](../api/SolidSyslogNorFlashBlockDevice_8h.md).

## What was measured, and on what

Both stores ran on the same Winbond W25Q128JV, a 16 MiB part with 4 KiB erase
blocks, on SPI at 6.75 MHz, from an STM32F767 at 216 MHz under CMSIS-RTOS2. Each
was given the first 8 MiB of the part. Both reached the part through the SPI
NOR flash pack, so every erase, program and read was counted at the same
interface. LittleFS was 2.11.3, with a program unit of 16 bytes and a cache of
512.

Each message was about 300 bytes as stored, under the CRC-16 security policy,
and was stored and then sent to a collector that was reachable throughout. The
figures are from the development of release 0.5.0.

| | LittleFS | No filesystem |
|---|---|---|
| A pace the store kept up with | 1 message every 260 ms | 1 message every 20 ms |
| A pace it did not keep up with | 1 every 120 ms | 1 every 10 ms |
| Erases a message | 2.08 | 0.077, which is one for every 13 messages |
| Programs a message | 11.6 | 2.2 |
| Reads a message | 105 | 9.5 |
| Creating the store on a part that holds one | 432 ms and 731 reads | 65 ms and 2100 reads |
| Creating the store on a blank part | 154 ms, with the filesystem's format | 120 ms, with the first block's erase |

A program and a read are each one call at the pack's interface, whatever it
spans.

## Reading the figures

**Pace.** A store kept up when every message of a run arrived at the collector
once and in order: 2000 over UDP and 2000 over TCP with no filesystem, and 1000
over each with LittleFS. It did not keep up when messages were discarded on the
way in. Only the paces in the table were tried, so each store's limit lies
somewhere between its two rows.

**Erases.** With no filesystem a block is erased once as the store takes it,
and holds 13 of these messages. The
[LittleFS page](../platforms/littlefs/index.md#what-the-store-costs-the-flash-in-erases)
says what a message costs under that filesystem, and how to turn erases a
message into the life of a part.

**Creating the store.** Each time includes about 10 ms for the console command
that asked for it. With no filesystem, the store asks about every block it was
given as it is created, which is one short read for each: 2048 of them here.
The time grows with the number of blocks you give it. With LittleFS the time is
the mount, and asking the filesystem about each file the store could have.

## What this is not

It is one part, one board, one SPI rate and one size of message. A different
part's erase time moves every row. The runs were minutes long, with the
collector reachable, so they say nothing about a backlog being stored and
drained, about wear over a part's life, or about power lost during a write.

The figures are not measured again at each release. Both images, and the count
of what each asks of the part, are in the repository under
`Bdd/Targets/Boards/NucleoF767zi/`, so they can be.

Nor is it the whole of the choice. A filesystem gives you files of your own on
the same part, and LittleFS brings wear levelling across everything on it and
its own answer to a loss of power. With no filesystem the store has the blocks
you give it to itself, wears them evenly by using them in rotation, and leaves
the rest of the part to you.
