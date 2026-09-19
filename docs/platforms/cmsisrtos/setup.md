# CMSIS-RTOS2 setup

Wiring the mutex. [CMSIS-RTOS2](index.md) covers what it fills and what it
leaves to you.

## What to link

`cmsis_os2.h` comes from your RTOS distribution rather than from the library or
the system, so the adapter cannot be precompiled: its sources compile inside
your target, against the header your kernel ships. Select it and link the
target it exports:

```cmake
set(SOLIDSYSLOG_PLATFORMS "CmsisRtos;<Network>;<Storage>")
target_link_libraries(my_app PRIVATE SolidSyslog SolidSyslog::CmsisRtos)
```

This platform fills the Mutex role and supplies the uptime callback; the
placeholders are whichever platforms
the [capability matrix](../index.md) says fill the rest of what your build
needs. See [naming your platforms](../../build-integration.md#cmake) for how
the list is read.

Your target supplies the include path for `cmsis_os2.h`, because it is your
kernel and your distribution.

## Wiring the mutex

The mutex exists to make a buffer safe when the task calling `SolidSyslog_Log`
is not the task calling `SolidSyslog_Service`:

```c
static uint8_t ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(8)];
static MutexControlBlock_t controlBlock;

struct SolidSyslogMutex* mutex =
    SolidSyslogCmsisRtosMutex_Create(&controlBlock, sizeof(controlBlock));

struct SolidSyslogBuffer* buffer =
    SolidSyslogCircularBuffer_Create(mutex, ring, sizeof(ring));
```

The ring memory, the control block and the mutex must all outlive the buffer.

If both calls happen on one task, pass `SolidSyslogNullMutex_Get()` - it is the
right answer and costs nothing.

### What to declare the control block as

`MutexControlBlock_t` above stands in for whatever your implementation's mutex
object is. This is the one thing the library cannot do for you, because
CMSIS-RTOS2 does not expose the size and every implementation chooses its own.
Find it in your RTOS documentation or its headers, and give the storage static
duration so it outlives every use.

Two ways to get it wrong are worth knowing apart. Storage that is too small is
refused outright: create time reports a `CRITICAL`, you get the Null mutex, and
the buffer you meant to protect is unguarded. Storage that is large enough but
does not outlive the buffer is far worse - it is a use-after-scope the library
cannot see, so give it static duration rather than sizing it generously on a
stack.

To let the implementation allocate the control block instead, pass NULL and
zero:

```c
struct SolidSyslogMutex* mutex = SolidSyslogCmsisRtosMutex_Create(NULL, 0);
```

That is the CMSIS-RTOS2 form for it, and it is the right choice where a heap is
available and unconstrained. An implementation built for static allocation only
will refuse it, reported the same way as a control block that is too small.

## Wiring the uptime callback

`SolidSyslogCmsisRtos_GetSysUpTime` is a plain function - point the meta SD's
config at it and there is nothing to create or keep alive:

```c
struct SolidSyslogMetaSdConfig metaConfig = {0};
metaConfig.Counter = counter;
metaConfig.GetSysUpTime = SolidSyslogCmsisRtos_GetSysUpTime;

struct SolidSyslogStructuredData* meta = SolidSyslogMetaSd_Create(&metaConfig);
```

Leaving the field NULL omits the `sysUpTime` PARAM instead.

It reports hundredths of a second since boot, not wall-clock time - the clock
callback in `SolidSyslogConfig` is a separate injection point. [CMSIS-RTOS2](index.md)
covers what the tick counter's width costs and why the callback is task-only.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you - a `CRITICAL` at create time means the component
fell back to its Null object.
