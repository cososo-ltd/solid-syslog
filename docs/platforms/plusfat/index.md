# FreeRTOS-Plus-FAT

`Platform/PlusFat/` wraps FreeRTOS-Plus-FAT as the File layer
([FreeRTOS-Plus-FAT documentation](https://www.freertos.org/Documentation/03-Libraries/05-FreeRTOS-labs/04-FreeRTOS-plus-FAT/01-FreeRTOS-plus-FAT)).

Fills the [File](../../api/structSolidSyslogFile.md) role — the primitive beneath a
BlockDevice.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogPlusFatFile`](../../api/SolidSyslogPlusFatFile_8h.md) | file — `ff_fflush` after every write |

## Requirements

FreeRTOS — Plus-FAT is FreeRTOS-coupled. Supply an `FF_Disk_t` media driver and
`FreeRTOSFATConfig.h`.

## Security behaviour and obligations

### The file layer offers no confidentiality or tamper evidence

Records are written as given. Detecting modification of a stored record, or
keeping it unreadable, is the SecurityPolicy role's job, not this one — see
[at-rest cryptography](../../security/at-rest-cryptography.md).

### Durability is bounded by the write, not guaranteed by it

`ff_fflush` runs after every write, so at most the record in flight is lost on
power failure. Whether that reaches the medium is a property of your `FF_Disk_t`
driver and the hardware under it. FAT is not a journalling filesystem, and a
partially written directory entry is possible on a device that loses power
mid-update.

### The media driver is yours

The `FF_Disk_t` implementation is supplied by you, and so is any wear levelling:
the store rewrites the same blocks in rotation, which on raw flash without wear
levelling concentrates erase cycles.
