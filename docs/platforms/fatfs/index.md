# FatFs

`Platform/FatFs/` wraps ChaN FatFs as the File layer
([FatFs documentation](http://elm-chan.org/fsw/ff/)). RTOS-agnostic - bare-metal
or under any RTOS.

Fills the [File](../../api/structSolidSyslogFile.md) role - the primitive beneath a
BlockDevice.

## What it ships

## Requirements

Your `ffconf.h`, a `diskio.c` media driver, and - if `FF_FS_REENTRANT=1` - an
`ffsystem.c`.

## Security behaviour and obligations

### The file layer offers no confidentiality or tamper evidence

Records are written as given. Detecting modification of a stored record, or
keeping it unreadable, is the SecurityPolicy role's job, not this one - see
[at-rest cryptography](../../security/at-rest-cryptography.md).

### Durability is bounded by the write, not guaranteed by it

`f_sync` runs after every write: it writes back the cached data, updates the
directory entry so the recorded file size includes the record, and issues
`CTRL_SYNC` to your driver. The loss window is therefore one incomplete write rather
than everything since the last close. Whether the sync reaches the medium, and
what the FAT metadata looks like afterwards, is a property of your `diskio.c`
driver and the hardware under it. FAT is not a journalling filesystem, and a
partially written directory entry is possible on a device that loses power
mid-update.

### A failed file call is reported, with its FRESULT

Each call the file makes on an open file, and the open itself, is reported
under the file's own error source when it fails, as
[`SOLIDSYSLOG_CAT_FILE_IO_FAILED`](../../api/SolidSyslogFileCategories_8h.md)
with a code naming the call: open, read, write, sync, seek, truncate or close.
The size comes from the file object and cannot fail, and nothing here commits,
so those two codes are never raised. Where the call answered an `FRESULT`
other than `FR_OK`, the event's `NativeCode` carries it. A full volume is a
write that `f_write` answers `FR_OK` for with fewer bytes than asked: that is
reported as a failed write with `HasNativeCode` clear. A short read is not a
failure and raises nothing: the store judges it for itself. Nor is a path
`f_stat` does not find when the file is only asked whether it exists.
[Error severity](../../error-severity.md) gives the level of each.

A close is always reported closed, as the
[File](../../api/structSolidSyslogFile.md) contract requires. When its sync
fails, though, `f_close` returns without releasing the file object and, under
`FF_FS_LOCK`, its lock entry, so the file owes FatFs that close. The next Open
settles it first, and the store opens the file again for every block it moves
to, so a disk error that clears costs one report. An Open whose owed close
fails again reports it, answers false and opens nothing, because `f_open`
would discard what FatFs still holds; the store treats that as a failed
device and freezes, as it does for any open that fails. A close that answers
`FR_INVALID_OBJECT` owes nothing: FatFs no longer knows the object, as after a
remount, which releases its locks. A file destroyed with a close still owed
tries it once more and gives its slot back regardless, and under
`FF_FS_LOCK` FatFs then holds the lock entry until the volume is mounted
again: a write-mode open of that path answers `FR_LOCKED` until then.

### The media driver is yours

`diskio.c`, and `ffsystem.c` where `FF_FS_REENTRANT=1`, are supplied by you. So
is any wear levelling: the store rewrites the same blocks in rotation, which on
raw flash without wear levelling concentrates erase cycles.
