# FreeRTOS-Plus-FAT

`Platform/PlusFat/` wraps FreeRTOS-Plus-FAT as the File layer
([FreeRTOS-Plus-FAT documentation](https://www.freertos.org/Documentation/03-Libraries/05-FreeRTOS-labs/04-FreeRTOS-plus-FAT/01-FreeRTOS-plus-FAT)).

Fills the [File](../../api/structSolidSyslogFile.md) role - the primitive beneath a
BlockDevice.

## What it ships

## Requirements

Needs FreeRTOS: Plus-FAT is coupled to the kernel. Supply an `FF_Disk_t` media
driver and `FreeRTOSFATConfig.h`.

## Security behaviour and obligations

### The file layer offers no confidentiality or tamper evidence

Records are written as given. Detecting modification of a stored record, or
keeping it unreadable, is the SecurityPolicy role's job, not this one - see
[at-rest cryptography](../../security/at-rest-cryptography.md).

### Durability is bounded by the write, not guaranteed by it

The adapter flushes after every write, and that flush persists the record's data
sectors. It does not commit the file's recorded size: the directory entry is
written when the file is closed, and the store holds a block file open across
appends. A power cut can therefore leave records on the medium that the recorded
size excludes, and the store will not read those back.

It flushes the IO manager's cache rather than the file: `ff_stdio.h` declares
`ff_fflush` but the library never defines it, so `FF_FlushCache` is the real
durability primitive. That holds of the upstream tree as of June 2026 - the
declaration is still there and no source file defines it - and the pack is
tested against an April 2026 checkout of it, Plus-FAT having no tagged
releases. Whether the flush reaches the medium is a property of your
`FF_Disk_t` driver and the hardware under it. FAT is not a journalling
filesystem, and a partially written directory entry is possible on a device that
loses power mid-update.

### A failed file call is reported, with its errno

Each call the file makes on an open file, and the open itself, is reported
under the file's own error source when it fails, as
[`SOLIDSYSLOG_CAT_FILE_IO_FAILED`](../../api/SolidSyslogFileCategories_8h.md)
with a code naming the call: open, read, write, sync, seek, size, truncate or
close. Nothing here commits, so that code is never raised. The event's
`NativeCode` is the FreeRTOS `errno` the `ff_stdio` call left in the task,
`pdFREERTOS_ERRNO_EIO` and its siblings, except for the sync, which is the
`FF_Error_t` that `FF_FlushCache` answered. An incomplete `ff_fwrite` leaves no
`errno`, so `HasNativeCode` is clear on its event. A read that reaches the end
of the file is not a failure and raises nothing: the store judges it for
itself. Nor is a path `ff_stat` does not find when the file is only asked
whether it exists, nor an open that creates the file because `r+` found none.
[Error severity](../../error-severity.md) gives the level of each.

### The media driver is yours

The `FF_Disk_t` implementation is supplied by you, and so is any wear levelling:
the store rewrites the same blocks in rotation, which on raw flash without wear
levelling concentrates erase cycles.
