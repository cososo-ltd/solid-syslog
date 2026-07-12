# Integrating SolidSyslog with FreeRTOS-Plus-FAT

`SolidSyslogPlusFatFile` is the `SolidSyslogFile` adapter backed by
[FreeRTOS-Plus-FAT](https://www.freertos.org/Documentation/03-Libraries/05-FreeRTOS-labs/04-FreeRTOS-plus-FAT/00-FreeRTOS-Plus-FAT)
(the `ff_stdio` API). It is the FreeRTOS-Plus ecosystem counterpart to the
OS-agnostic [ChaN FatFs adapter](../Platform/FatFs/); pair it with
FreeRTOS-Plus-TCP for a coherent all-FreeRTOS-Plus storage + transport stack. It
gives the store-and-forward layer (`SolidSyslogBlockStore` over
`SolidSyslogFileBlockDevice`) a real on-flash file backend.

This guide covers what you must supply around the adapter. For the file seam
itself see [`SolidSyslogFile.h`](../Core/Interface/SolidSyslogFile.h); for the
store see [`SolidSyslogBlockStore.h`](../Core/Interface/SolidSyslogBlockStore.h).

## The shape

```text
SolidSyslogBlockStore
        │  (SolidSyslogStore vtable)
SolidSyslogFileBlockDevice          <-- sequence-numbered <prefix><NN>.log files
        │  (SolidSyslogFile vtable)
SolidSyslogPlusFatFile              <-- this adapter: ff_fopen / ff_fread / ff_fwrite / …
        │  (ff_stdio API)
FreeRTOS-Plus-FAT core (ff_*.c)     <-- vendor library, you compile it
        │  (FF_Disk_t block callbacks)
your FF_Disk_t media driver         <-- YOU write this: SD / eMMC / QSPI-flash / RAM
```

The adapter owns only the middle box. The vendor core and the media driver are
yours to provide; the library never reaches the block device directly.

## What you must provide

1. The FreeRTOS-Plus-FAT sources, compiled into your image (`ff_crc.c`,
   `ff_dir.c`, `ff_error.c`, `ff_fat.c`, `ff_file.c`, `ff_format.c`, `ff_ioman.c`,
   `ff_locking.c`, `ff_memory.c`, `ff_stdio.c`, `ff_string.c`, `ff_sys.c`,
   `ff_time.c`). These are not -Wsign-conversion / -Wconversion clean; compile
   them under a relaxed warning set, as you would the FreeRTOS kernel.

2. An `FF_Disk_t` media driver for your storage hardware: `FF_CreateIOManager`
   with read/write block callbacks, `FF_Mount` (format-on-first-use via
   `FF_Partition` + `FF_Format` when no FAT is present), and `FF_FS_Add("/", disk)`
   to register the volume in `ff_stdio`'s virtual file system. Plus-FAT ships
   reference drivers under `portable/` (`ff_ramdisk.c` is the clearest template).
   The library's BDD target ships a semihosting example,
   [`Bdd/Targets/Common/FFSemihostingDisk.c`](../Bdd/Targets/Common/FFSemihostingDisk.c).

3. A `FreeRTOSFATConfig.h` on your include path. `ff_headers.h` pulls it via
   `#include "FreeRTOSFATConfig.h"`; unlike ChaN FatFs's `ffconf.h`, it resolves
   off the `-I` path (no source-tree colocation needed). At minimum set
   `ffconfigBYTE_ORDER` and `ffconfigCWD_THREAD_LOCAL_INDEX`;
   `FreeRTOSFATConfigDefaults.h` fills the rest. See
   [`Bdd/Targets/FreeRtos/FreeRTOSFATConfig.h`](../Bdd/Targets/FreeRtos/FreeRTOSFATConfig.h).

4. Kernel configuration in `FreeRTOSConfig.h`:
   - `configUSE_RECURSIVE_MUTEXES = 1` (Plus-FAT's `ff_locking.c` enforces this).
   - Event groups compiled in (`event_groups.c`); the IO manager uses them.
   - `configNUM_THREAD_LOCAL_STORAGE_POINTERS >= ffconfigCWD_THREAD_LOCAL_INDEX + 3`.
     `ff_stdio` stores its `errno`, CWD, and `FF_Error` in per-task thread-local
     storage at offsets `ffconfigCWD_THREAD_LOCAL_INDEX + {0,1,2}`. With the index
     at 0 that means at least 3 slots; `ff_stdio.h` enforces this at compile
     time.
   - `configSUPPORT_STATIC_ALLOCATION = 1` if your media driver creates its IO
     manager mutex statically (the example does); dynamic allocation otherwise.

## Path convention — absolute paths

With `ffconfigHAS_CWD = 0` (the default), Plus-FAT's `ff_stdio` accepts only
absolute paths; its relative-path resolver (`prvABSPath`) is a pass-through.
Configure `SolidSyslogFileBlockDevice` with an absolute path prefix, e.g.
`/STORE`, so the store files land at the volume root as `/STORE00.log`,
`/STORE01.log`, … (A leading `/` is equally valid for the ChaN FatFs adapter, so
the same prefix works for either backend.) The default 8.3 short-filename mode
(`ffconfigLFN_SUPPORT = 0`) is sufficient for that naming.

## Durability contract

`SolidSyslogPlusFatFile_Write` flushes after every complete write so a power
loss never loses a record the `BlockStore` was told had been stored. Two notes
specific to Plus-FAT:

- There is no per-file flush. `ff_stdio.h` *declares* `ff_fflush`, but the
  library (as of SHA `8d38036`) never *defines* it. The adapter instead calls
  `FF_FlushCache(file->pxIOManager)`, the IO-manager cache flush, which is the
  real durability primitive.
- The directory entry (file size) is committed on `Close`, not on each
  flush. `FF_FlushCache` persists the file's data sectors; the dirent's size
  field is written by `ff_fclose`. A graceful shutdown (the library's
  `SolidSyslog` teardown closes the store file) therefore leaves both data and
  metadata consistent. If your platform must survive a hard power cut
  mid-record, size the records and the discard policy with that in mind.

## Heap usage

The adapter struct itself is pool-allocated (`SOLIDSYSLOG_FILE_POOL_SIZE`) and
never calls `malloc`. FreeRTOS-Plus-FAT, however, does allocate: the IO
manager sector cache and internal buffers come from the FreeRTOS heap (`heap_4`
or your `pvPortMalloc`). This is integrator-scoped and expected, exactly the
mbedTLS precedent: the vendor library allocates; SolidSyslog's own structures do
not. Size your heap for the IO-manager cache you request in `FF_CreateIOManager`.

## Reference integration

[`Bdd/Targets/FreeRtos/`](../Bdd/Targets/FreeRtos/) is the worked example, the
FreeRTOS-Plus-TCP + FreeRTOS-Plus-FAT QEMU BDD target. It wires:

- [`FFSemihostingDisk.c`](../Bdd/Targets/Common/FFSemihostingDisk.c): an
  `FF_Disk_t` over an ARM-semihosting host-backed flat disk (8 MiB, FAT16),
  modelled on Plus-FAT's `ff_ramdisk.c` but persistent (mount-or-format-on-first-
  use, so a power cycle keeps its data).
- [`BddTargetPlusFatMount.c`](../Bdd/Targets/Common/BddTargetPlusFatMount.c):
  the mount/unmount + `SolidSyslogPlusFatFile` create/destroy wired into the
  shared FreeRTOS pipeline's FS-mount seam.
- [`FreeRTOSFATConfig.h`](../Bdd/Targets/FreeRtos/FreeRTOSFATConfig.h) and the
  `configNUM_THREAD_LOCAL_STORAGE_POINTERS` knob in
  [`FreeRTOSConfig.h`](../Bdd/Targets/FreeRtos/FreeRTOSConfig.h).

The full store / capacity / power-cycle-replay BDD suite runs against this target
on QEMU (`bdd-freertos-qemu-plustcp`).

## What this adapter does not own

- The media driver. SD/eMMC/flash/RAM block I/O is yours (or a Plus-FAT
  `portable/` driver). The adapter speaks only `ff_stdio`.
- Mounting and formatting. The adapter opens/reads/writes files; bringing the
  volume up (`FF_Mount` / `FF_Format` / `FF_FS_Add`) is the media driver's job.
- The FreeRTOS-Plus-FAT sources and their licence. You vendor and compile
  them.
