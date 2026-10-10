# VxWorks 6.4 setup

Wiring the UDP and TCP transports, the file store, the mutex, the atomic
counter, and the clock, hostname, uptime and sleep callbacks.
[VxWorks 6.4](index.md) covers what they fill and what they leave to you.

## What to link

The adapter compiles inside your build, against the headers your kernel
ships; [Adding it to your build](../../build-integration.md#what-you-link) says
why. The library is built as `libsolidsyslog.a` with your project's own
compiler and flags, by glue that differs with the kind of project:

| | VxWorks Image Project (VIP) | Downloadable kernel module (DKM) |
|---|---|---|
| Glue | `Platform/VxWorks64/solidsyslog.makefile`, run with the project's Makefile | `Platform/VxWorks64/solidsyslog-dkm.makefile`, included from a managed-build extension makefile |
| Library written to | `solidsyslog/` in the project | `solidsyslog/<BUILD_SPEC>/<MODE_DIR>` in the project |
| Headers reach your sources through | `CFLAGS`, set with `vxprj buildmacro`, with `Compat` last | `ADDED_INCLUDES`, without `Compat` |
| The kernel image also needs | nothing more | the compiler intrinsics component for the DKM's toolchain |

### In a VIP

```text
vxprj creates the project
  -> solidsyslog_library, once per build specification
  -> CFLAGS and LIBS set with vxprj buildmacro, once per build specification
  -> the project builds
```

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

### In a DKM

```text
the DKM project's generated Makefile
  -> a managed-build extension makefile includes solidsyslog-dkm.makefile
  -> external_build runs solidsyslog-vxworks64.mk
  -> solidsyslog/<BUILD_SPEC>/<MODE_DIR>/libsolidsyslog.a
  -> PROJECT_TARGETS wait for that archive
  -> ADDED_INCLUDES, ADDED_LIBPATH and ADDED_LIBS reach the DKM's compile and link
```

Include `Platform/VxWorks64/solidsyslog-dkm.makefile` from a managed-build
extension makefile in the DKM project. Workbench's generated Makefile defines
`PRJ_ROOT_DIR`, `BUILD_SPEC`, `MODE_DIR`, `PROJECT_TARGETS` and `TOOL_FAMILY`
before it includes the extension, and the glue reads all five.

The generated Makefile writes its compiler and archiver into each recipe rather
than naming them in variables, so the extension supplies three values the glue
cannot find for itself:

```make
# Copy each value from the DKM project's own build: not a host compiler, and
# not a VIP's flags.
SOLIDSYSLOG_CC := <the DKM's compiler command>
SOLIDSYSLOG_AR := <the DKM's archiver command>
# The DKM's CPU, define and include flags, without its dialect flag: the
# library adds its own C99 dialect.
SOLIDSYSLOG_TARGET_CFLAGS := <the DKM's target flags>

include <checkout>/Platform/VxWorks64/solidsyslog-dkm.makefile
```

The glue stops the build, naming the variable, if any of the three is missing
or empty.

What the glue then does on each build:

- It runs the library's own build, which rewrites the archive only when a
  library source or header has changed. The module's link targets depend on
  the archive, so the module relinks when the library changes and not
  otherwise, and under `make -j` it links only once the archive exists.
- `ADDED_INCLUDES` gains `Core/Interface` and the `Interface` directory of each
  platform in `SOLIDSYSLOG_PLATFORMS`, and `ADDED_LIBPATH` and `ADDED_LIBS`
  add the library. `SOLIDSYSLOG_PLATFORMS` defaults to `VxWorks64`; setting it
  replaces that default, so name `VxWorks64` among the others.
- Cleaning the project cleans the library's build too.

Do not add `Platform/VxWorks64/Compat` to the DKM's own include path. The
library's build takes it first, but the DKM's sources must not:
[VxWorks 6.4](index.md#requirements) says why.

`SOLIDSYSLOG_DIR` defaults to the checkout the glue sits in. Set
`SOLIDSYSLOG_BUILD_DIR` only to run the glue outside a Workbench build: without
`PRJ_ROOT_DIR`, `BUILD_SPEC` and `MODE_DIR` the default would name a directory
at the root of the drive, so the glue refuses to build or clean until you name
one.

The kernel image the module loads into needs the compiler intrinsics component
for the DKM's toolchain, `INCLUDE_DIAB_INTRINSICS` or `INCLUDE_GNU_INTRINSICS`.
The uptime and sleep callbacks divide 64-bit values, and a module takes the
helpers that do that from the kernel when it loads.

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

The datagram waits for a collector on the target's own subnet to resolve
before it sends; `SOLIDSYSLOG_DATAGRAM_RESOLVE_WAIT_MS` bounds the wait. For a
collector on another subnet it cannot, as
[A record waits for its next hop to resolve](index.md#a-record-waits-for-its-next-hop-to-resolve)
explains. Resolve the gateway once at start-up, before the first
`SolidSyslog_Service`, so the records logged before then are not lost:

```c
#include <arpLib.h>
#include <sysLib.h>

static char gateway[] = "192.0.2.1"; /* your gateway */
unsigned short linkAddress[3];       /* an Ethernet address, 16-bit aligned */
(void) arpResolve(gateway, (char*) linkAddress, 2, sysClkRateGet() / 10);
```

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
