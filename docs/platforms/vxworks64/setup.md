# VxWorks 6.4 setup

Wiring the UDP transport and the mutex. [VxWorks 6.4](index.md) covers what
they fill and what they leave to you.

## What to link

The VxWorks headers come from your Wind River installation rather than from the
library or the system, so the adapter cannot be precompiled: its sources compile
inside your build, against the headers your kernel ships. The library is built
as `libsolidsyslog.a` with your project's own compiler and flags, by running
`Platform/VxWorks64/solidsyslog.makefile` against the project's Makefile. This
is the route verified with a VxWorks Image Project (VIP) created by `vxprj`.

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
one sender. Drawing past the pool sizes in
[Adding it to your build](../../build-integration.md#tunables) reports
`CRITICAL`. The resolver and the datagram then hand back their Null objects. The
address hands back one shared, writable fallback instead, so every sender drawn
beyond the pool shares that storage and races on it; raising
`SOLIDSYSLOG_ADDRESS_POOL_SIZE` removes the race.

## Wiring the mutex

The mutex exists to make a buffer safe when the task calling `SolidSyslog_Log`
is not the task calling `SolidSyslog_Service`:

```c
static uint8_t ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(8)];

struct SolidSyslogMutex* mutex = SolidSyslogVxWorks64Mutex_Create();

struct SolidSyslogBuffer* buffer =
    SolidSyslogCircularBuffer_Create(mutex, ring, sizeof(ring));
```

The ring memory and the mutex must outlive the buffer.

If both calls happen on one task, pass `SolidSyslogNullMutex_Get()` - it is the
right answer and costs nothing.

## The callbacks

`SolidSyslogConfig` takes the clock and hostname as callbacks rather than
components, and this platform supplies one of each ready to use:
`SolidSyslogVxWorks64_GetTimestamp` and `SolidSyslogVxWorks64_GetHostname`.
Leave `GetProcessId` unset; a kernel task has no process id to report.
`SolidSyslogVxWorks64_GetSysUpTime` fills the meta structured data's sysUpTime,
and `SolidSyslogVxWorks64_Sleep` is ready for any component that takes a sleep
callback.

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
