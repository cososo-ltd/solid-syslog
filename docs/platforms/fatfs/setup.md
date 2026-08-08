# FatFs setup

Giving store-and-forward a file backend. [FatFs](index.md) covers what the
adapter fills and what it leaves to you.

## The shape

```text
SolidSyslogBlockStore
        │
SolidSyslogFileBlockDevice     ◀── sequence-numbered <prefix><NN>.log files
        │
SolidSyslogFatFsFile           ◀── this adapter
        │
FatFs core                     ◀── vendor sources, you compile them
        │
your disk I/O driver           ◀── you write this
```

The adapter owns the middle box only. It never reaches the storage medium.

## What to link

FatFs is configured by a header you own, so the adapter compiles inside your
target against your configuration:

```cmake
set(SOLIDSYSLOG_PLATFORMS "FatFs")
target_link_libraries(my_app PRIVATE SolidSyslog SolidSyslog::FatFs)
```

## What you must provide

**The FatFs sources**, compiled into your image, and a `ffconf.h` where the
library expects to find it — beside the sources rather than on the include
path, which differs from most configuration headers.

**A disk I/O driver** for your storage hardware, implementing the read, write
and control entry points FatFs calls. This is the part that knows about your
medium, and nothing in this library reaches past it.

**A system layer** if you have enabled re-entrancy, providing the
synchronisation objects FatFs expects.

**Mounting** is yours. The adapter opens, reads and writes files; bringing the
volume up belongs to your start-up code.

## Durability

The adapter flushes after every complete write, so a power loss never discards
a record the store was told had been written. Whether that flush reaches the
medium is a property of your disk I/O driver.

FAT is not a journalling file system. A device that loses power partway through
a directory update can leave that entry inconsistent, which is a property of
the format rather than of this adapter. Where records must survive a hard power
cut, size them and choose the store's discard policy with that in mind.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the file fell
back to the Null object, and nothing will be stored.
