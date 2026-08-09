# FatFs

`Platform/FatFs/` wraps ChaN FatFs as the File layer
([FatFs documentation](http://elm-chan.org/fsw/ff/)). RTOS-agnostic — bare-metal
or under any RTOS.

Fills the [File](../../api/structSolidSyslogFile.md) role — the primitive beneath a
BlockDevice.

## What it ships

## Requirements

Your `ffconf.h`, a `diskio.c` media driver, and — if `FF_FS_REENTRANT=1` — an
`ffsystem.c`.

## Security behaviour and obligations

### The file layer offers no confidentiality or tamper evidence

Records are written as given. Detecting modification of a stored record, or
keeping it unreadable, is the SecurityPolicy role's job, not this one — see
[at-rest cryptography](../../security/at-rest-cryptography.md).

### Durability is bounded by the write, not guaranteed by it

`f_sync` runs after every write. It writes back the cached data, updates the
directory entry so the recorded file size includes the record, and issues
`CTRL_SYNC` to your driver — so the loss window is one incomplete write rather
than everything since the last close. Whether the sync reaches the medium, and
what the FAT metadata looks like afterwards, is a property of your `diskio.c`
driver and the hardware under it. FAT is not a journalling filesystem, and a
partially written directory entry is possible on a device that loses power
mid-update.

### The media driver is yours

`diskio.c`, and `ffsystem.c` where `FF_FS_REENTRANT=1`, are supplied by you. So
is any wear levelling: the store rewrites the same blocks in rotation, which on
raw flash without wear levelling concentrates erase cycles.
