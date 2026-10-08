# VxWorks 6.4 setup

Wiring the UDP and TCP transports, the file store, the mutex, the atomic
counter, and the clock, hostname, uptime and sleep callbacks.
[VxWorks 6.4](index.md) covers what they fill and what they leave to you.

## What to link

The adapter compiles inside your build, against the headers your kernel
ships; [Adding it to your build](../../build-integration.md#what-you-link) says
why. The library is built as `libsolidsyslog.a` with your project's own
compiler and flags, by running `Platform/VxWorks64/solidsyslog.makefile`
against the project's Makefile. This is the route verified with a VxWorks Image
Project (VIP) created by `vxprj`.

Build the library from the project directory, for each build specification you
use. `SOLIDSYSLOG_DIR` points at the SolidSyslog checkout, and
`SOLIDSYSLOG_PLATFORMS` names this platform and whichever others the
[capability matrix](../index.md) says fill the rest of what your build needs:

```sh
make -C <project> -f Makefile -f <checkout>/Platform/VxWorks64/solidsyslog.makefile \
     BUILD_SPEC=<spec> SOLIDSYSLOG_DIR=<checkout> SOLIDSYSLOG_PLATFORMS=VxWorks64 \
     solidsyslog_library
```

The library is written to `solidsyslog/` in the project directory unless you
set `SOLIDSYSLOG_BUILD_DIR`. Run it after `vxprj` has generated the project's
configuration: the VxWorks headers include the generated `prjComps.h`, so the
library cannot build in a project that has just been cleaned.

Then give the project the headers and the library, with
`vxprj buildmacro set` on each build specification:

- `CFLAGS` gains `-I<checkout>/Core/Interface`,
  `-I<checkout>/Platform/VxWorks64/Interface` and, last,
  `-I<checkout>/Platform/VxWorks64/Compat`.
- `LIBS` names the library ahead of `$(VX_OS_LIBS)`, which it calls into.

[VxWorks 6.4](index.md#requirements) says why the library needs its own dialect
and what `Compat` supplies. `Bdd/Targets/VxWorks64/New-VxWorks64Vip.ps1` and
`Build-VxWorks64Vip.ps1` carry out every step above for the BDD target.

A downloadable kernel module (DKM) build of the library also needs the kernel
image to include the compiler intrinsics component for the DKM's toolchain,
`INCLUDE_GNU_INTRINSICS` or `INCLUDE_DIAB_INTRINSICS`, because the uptime and
sleep callbacks use 64-bit division.

## Drawing the UDP pieces

```c
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64Datagram.h"
#include "SolidSyslogVxWorks64Resolver.h"

struct SolidSyslogAddress*  address  = SolidSyslogVxWorks64Address_Create();
struct SolidSyslogResolver* resolver = SolidSyslogVxWorks64Resolver_Create();
struct SolidSyslogDatagram* datagram = SolidSyslogVxWorks64Datagram_Create();
```

None of them takes a configuration. Hand all three to
`SolidSyslogUdpSender_Create`; one address, one resolver and one datagram serve
one sender.

## Drawing the TCP pieces

```c
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64Resolver.h"
#include "SolidSyslogVxWorks64TcpStream.h"

struct SolidSyslogAddress*  address  = SolidSyslogVxWorks64Address_Create();
struct SolidSyslogResolver* resolver = SolidSyslogVxWorks64Resolver_Create();

/* NULL leaves the connect deadline at the SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS
   tunable; supply a getter where the deadline has to move at runtime. */
struct SolidSyslogStream* stream = SolidSyslogVxWorks64TcpStream_Create(NULL);
```

Hand all three to `SolidSyslogStreamSender_Create`; one address, one resolver
and one stream serve one sender. Only the stream takes a configuration, and
then only to tune its connect deadline.

## Giving store-and-forward a file

```c
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogVxWorks64File.h"

struct SolidSyslogFile* file = SolidSyslogVxWorks64File_Create();

/* Block files /ata0a/STORE00.log and on, up to 64 KiB each. */
struct SolidSyslogBlockDevice* device = SolidSyslogFileBlockDevice_Create(file, "/ata0a/STORE", 65536);
```

Hand the device to `SolidSyslogBlockStore_Create`. The file takes no
configuration; one file serves one block device. The prefix names a mounted
volume, here dosFs or HRFS on the primary IDE disk, and the names the block
device adds to it fit dosFs's 8.3 limit, which HRFS does not impose.
[Mounting and formatting are yours](index.md#mounting-and-formatting-are-yours)
says what the volume needs before the store is created.

## When a pool runs out

Drawing past the pool sizes in
[Adding it to your build](../../build-integration.md#tunables) reports
`CRITICAL`. The resolver, the datagram, the stream and the file then hand back
their Null objects. The [address](../../api/SolidSyslogVxWorks64Address_8h.md)
hands back a shared fallback instead; raise `SOLIDSYSLOG_ADDRESS_POOL_SIZE` so
that no sender draws it.

## Wiring the mutex

The mutex makes a
[circular buffer](../../api/SolidSyslogCircularBuffer_8h.md) safe when the task
calling `SolidSyslog_Log` is not the task calling `SolidSyslog_Service`:

```c
static uint8_t ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(8)];

struct SolidSyslogMutex* mutex = SolidSyslogVxWorks64Mutex_Create();

struct SolidSyslogBuffer* buffer =
    SolidSyslogCircularBuffer_Create(mutex, ring, sizeof(ring));
```

The ring memory and the mutex must outlive the buffer.

If both calls happen on one task, pass `SolidSyslogNullMutex_Get()` instead.

## Wiring the atomic counter

The counter supplies the meta structured data's sequenceId, and pairs naturally
with the uptime callback below:

```c
struct SolidSyslogAtomicCounter* counter = SolidSyslogVxWorks64AtomicCounter_Create();

struct SolidSyslogMetaSdConfig metaConfig = {
    .Counter      = counter,
    .GetSysUpTime = SolidSyslogVxWorks64_GetSysUpTime,
};
struct SolidSyslogStructuredData* meta = SolidSyslogMetaSd_Create(&metaConfig);
```

Create takes no configuration. The counter must outlive the meta structured
data, which does not destroy it. One counter serves one logger; drawing past
the pool size in [Adding it to your build](../../build-integration.md#tunables)
hands back the Null counter, as the header describes.

## The callbacks

`SolidSyslogConfig` takes the clock and hostname as callbacks rather than
components, and this platform supplies one of each ready to use:
`SolidSyslogVxWorks64_GetTimestamp` and `SolidSyslogVxWorks64_GetHostname`.
Leave `GetProcessId` unset; a kernel task has no process id to report.
`SolidSyslogVxWorks64_GetSysUpTime` fills the meta structured data's sysUpTime,
and `SolidSyslogVxWorks64_Sleep` is ready for any component that takes a sleep
callback for its short waits; its header states the range it covers.

```c
#include "SolidSyslogVxWorks64Clock.h"
#include "SolidSyslogVxWorks64Hostname.h"

config.Clock       = SolidSyslogVxWorks64_GetTimestamp;
config.GetHostname = SolidSyslogVxWorks64_GetHostname;
```

Set the clock, and the hostname if the image does not, before the first record
is logged. [VxWorks 6.4](index.md#the-clock-is-only-as-right-as-whatever-set-it)
says what a record carries before then.

## When it does not work

Install an error handler before you start; [error severity](../../error-severity.md)
says what each level is telling you.
