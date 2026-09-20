# LittleFS

`Platform/LittleFs/` wraps [LittleFS](https://github.com/littlefs-project/littlefs)
as the File layer. It is RTOS-agnostic, and it targets raw flash directly: power-loss
resilience, dynamic wear levelling and bounded RAM are goals of the filesystem
itself rather than obligations it leaves with you.

Fills the [File](../../api/structSolidSyslogFile.md) role - the primitive beneath a
BlockDevice.

## What it ships

## Requirements

A LittleFS checkout, your block-device callbacks in `struct lfs_config`, and a
mounted `lfs_t`. The adapter calls `lfs_file_opencfg`, `lfs_file_read`,
`lfs_file_write`, `lfs_file_sync`, `lfs_file_seek`, `lfs_file_truncate`,
`lfs_file_size`, `lfs_file_close`, `lfs_stat` and `lfs_remove`. Nothing else in
the API needs to be present.

LittleFS's own headers are built with a narrower warning set than this library's,
and `lfs_util.h` will not compile under `-Wsign-conversion`. Include the tree as a
system include; [setting it up](setup.md) says how.

## Security behaviour and obligations

### You mount it, and you keep it

The `lfs_t`, its `struct lfs_config` and the mount are yours, and the mounted
handle is passed in at Create. The adapter never mounts, never formats and never
unmounts, so an application already using the filesystem for its own purposes
keeps control of its lifetime. The handle must outlive every file created from it.

### You supply each file's cache, because only you can size it

`lfs_file_opencfg` gives each open file a cache of the mounted filesystem's
`cache_size`, and that is a runtime property of the config you mounted with. The
library cannot size it and does not guess: `SolidSyslogLittleFsFile_Create` takes
the storage and its size from you, and the storage must outlive the file. It is
the file's cache, not a copy of one.

A buffer of at least `cache_size` is accepted; a smaller one, or no buffer at
all, is refused at Create:
the file falls back to the Null object and a `CRITICAL` is raised. Treat that as
a build defect rather than a runtime condition - `cache_size` is fixed when you
mount, so a size that is right once is right always.

Supplying the buffer is also what keeps the adapter allocation-free. It calls
`lfs_file_opencfg` rather than `lfs_file_open` whatever your configuration, so
the pack never reaches an allocator even where LittleFS is built with one.

### The file layer offers no confidentiality or tamper evidence

Records are written as given. Detecting modification of a stored record, or
keeping it unreadable, is the SecurityPolicy role's job, not this one - see
[at-rest cryptography](../../security/at-rest-cryptography.md).

### Durability is bounded by the write, not guaranteed by it

`lfs_file_sync` runs after every write, so a true return from Write means the
record reached the filesystem rather than a cache. The loss window is one
incomplete write rather than everything since the last close.

What LittleFS then guarantees is stronger than a FAT filesystem can offer: its
copy-on-write metadata is designed so that an interrupted update leaves the
previous committed state intact rather than a half-written directory entry. That
guarantee still rests on your block-device callbacks reporting a program or erase
as complete only once it is, and on the flash part behaving as its datasheet says
through a power cut.

### What the pack has been tested against

The durability claim is tested, not asserted. An integration suite runs the whole store
stack over an emulated flash device that can lose power mid-write: the device
counts program and erase operations and, on a chosen one, either does nothing or
writes half its buffer and then fails, after which every operation fails because
the power is gone. The test then discards the filesystem handle and mounts again
from the same bytes. Every record the store reported as written is still there
and complete, for a clean cut and a torn one, and a record interrupted mid-write
is refused rather than half-kept.

Beyond that, a BDD target exercises the pack end to end under QEMU: the store
writes records to a real LittleFS over a block device backed by a host image,
and the syslog scenarios that cover store-and-forward, capacity and power-cycle
replay run against it like any other filesystem. That is the pack carrying real
traffic rather than a unit fixture.

Both stop short of a real flash part:
erase granularity, partial programming at the cell level, read disturb and wear
are properties of silicon that neither an emulated device nor a host-backed disk
image reproduces - and the BDD image, having no erase of its own, emulates one by
writing the erased value, so a program there can raise bits as a real part could
not. Treat both as evidence that the adapter and LittleFS honour the contract
between them, not as qualification against your flash.

### Wear levelling is the filesystem's, not yours

The store rewrites the same records in rotation, which on raw flash concentrates
erase cycles. LittleFS spreads them itself, which is the main reason to choose it
here over a FAT filesystem on the same part. `block_cycles` in your
`struct lfs_config` decides how eagerly; the default disables it, so set it.
