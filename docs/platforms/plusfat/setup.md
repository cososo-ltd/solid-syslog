# FreeRTOS-Plus-FAT setup

Giving store-and-forward a real file backend.
[FreeRTOS-Plus-FAT](index.md) covers what the adapter fills and what it leaves
to you. This page is what you must supply around it.

## The shape

```text
SolidSyslogBlockStore
        │
SolidSyslogFileBlockDevice     ◀── sequence-numbered <prefix><NN>.log files
        │
SolidSyslogPlusFatFile         ◀── this adapter, over the ff_stdio API
        │
FreeRTOS-Plus-FAT core         ◀── vendor sources, you compile them
        │
your FF_Disk_t media driver    ◀── you write this
```

The adapter owns the middle box only. It speaks `ff_stdio` and never reaches
the block device.

## What you must provide

**The FreeRTOS-Plus-FAT sources**, compiled into your image. They are not clean
under the conversion warnings this project builds with, so compile them under a
relaxed warning set as you would the kernel itself.

**An `FF_Disk_t` media driver** for your storage hardware — the IO manager with
its read and write block callbacks, the mount, format-on-first-use where no
file system is present, and registration of the volume in the virtual file
system. Plus-FAT ships reference drivers; its RAM disk is the clearest
template.

**A `FreeRTOSFATConfig.h`** on your include path. Unlike some file systems this
resolves off `-I` rather than needing to sit beside the sources. Set at least
the byte order and the thread-local storage index; the defaults header fills
the rest.

**Kernel configuration**, because Plus-FAT requires it:

- recursive mutexes, which its locking enforces
- event groups, used by the IO manager
- enough thread-local storage slots — `ff_stdio` keeps its error state, working
  directory and error code in three consecutive slots from the index you
  configure, and its header enforces this at compile time
- static allocation, if your media driver creates its IO manager mutex
  statically

## Paths must be absolute

Without the working-directory option compiled in, `ff_stdio` accepts absolute
paths only — its relative-path resolver is a pass-through. Give
`SolidSyslogFileBlockDevice` an absolute prefix such as `/STORE`, so files land
at the volume root as `/STORE00.log`, `/STORE01.log`. The default short
filename mode is sufficient for that naming.

## Durability

The adapter flushes after every complete write, so a power loss never discards
a record the store was told had been written.

Two things are specific to Plus-FAT and worth knowing. There is **no per-file
flush**: `ff_stdio.h` declares `ff_fflush` but the library never defines it, so
the adapter flushes the IO manager's cache instead, which is the real
durability primitive. And the **directory entry is committed on close, not on
each flush** — the cache flush persists the file's data sectors, while the
recorded file size is written when the file is closed. A graceful shutdown
therefore leaves data and metadata consistent. If the device must survive a
hard power cut mid-record, size your records and choose your discard policy
with that in mind.

## Memory

The adapter allocates nothing; its instance comes from a static pool. Plus-FAT
does allocate — the IO manager's sector cache and its internal buffers come
from the RTOS heap — so size that heap for the cache you ask the IO manager
for.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the file fell
back to the Null object, and nothing will be stored.
