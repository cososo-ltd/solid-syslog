# VxWorks 6.4 setup

Wiring the UDP transport and the mutex. [VxWorks 6.4](index.md) covers what
they fill and what they leave to you.

## What to link

The VxWorks headers come from your Wind River installation rather than from the
library or the system, so the adapter cannot be precompiled: its sources compile
inside your build, against the headers your kernel ships.

With CMake, select it and link the target it exports:

```cmake
set(SOLIDSYSLOG_PLATFORMS "VxWorks64;<Network>;<Storage>")
target_link_libraries(my_app PRIVATE SolidSyslog SolidSyslog::VxWorks64)
```

With Make, name it in the platform list before including the fragment:

```make
SOLIDSYSLOG_PLATFORMS := VxWorks64
include third_party/solid-syslog/solidsyslog.mk
```

This platform fills the Datagram, Resolver and Mutex roles; the placeholders are whichever platforms the
[capability matrix](../index.md) says fill the rest of what your build needs.
See [naming your platforms](../../build-integration.md#cmake) for how the list
is read.

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
[Adding it to your build](../../build-integration.md#tunables) hands back a Null
object and reports `CRITICAL`.

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

## When it does not work

Install an error handler before you start; [error severity](../../error-severity.md)
says what each level is telling you.
