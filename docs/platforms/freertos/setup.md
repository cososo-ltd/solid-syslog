# FreeRTOS setup

Wiring the kernel primitives. [FreeRTOS](index.md) covers what they fill and
what they leave to you.

## What to link

FreeRTOS is configured by a header you own, so the adapter cannot be
precompiled: its sources compile inside your target, against your
`FreeRTOSConfig.h`. Select it and link the target it exports:

```cmake
set(SOLIDSYSLOG_PLATFORMS "FreeRtos;<Network>;<Storage>")
target_link_libraries(my_app PRIVATE SolidSyslog SolidSyslog::FreeRtos)
```

This platform fills the Mutex role and the sysUpTime callback; the placeholders
are whichever platforms the [capability matrix](../index.md) says fill the rest
of what your build needs, and one you do not need comes out. See
[naming your platforms](../../build-integration.md#cmake) for how the list is
read.

Your target supplies the kernel's include path, because it is your kernel and
your configuration.

`configSUPPORT_STATIC_ALLOCATION` must be 1. The mutex is created from storage
inside the library's own pool, so nothing is allocated at run time and creation
cannot fail for want of heap.

## Wiring the mutex

The mutex exists to make a buffer safe when the task calling `SolidSyslog_Log`
is not the task calling `SolidSyslog_Service`:

```c
struct SolidSyslogMutex* mutex = SolidSyslogFreeRtosMutex_Create();

struct SolidSyslogBuffer* buffer =
    SolidSyslogCircularBuffer_Create(&(struct SolidSyslogCircularBufferConfig) {
        .Sender = sender,
        .Mutex  = mutex,
        /* ring storage sized with SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES */
    });
```

The ring memory and the mutex must both outlive the buffer.

If both calls happen on one task, leave the role unfilled — the Null mutex is
the right answer and costs nothing.

## Uptime

`SolidSyslogFreeRtosSysUpTime` reports kernel ticks since boot. It is not
wall-clock time — the clock callback in `SolidSyslogConfig` is a separate
injection point, and on a target with no real-time clock a timestamp the
library cannot establish is emitted as absent rather than as a plausible wrong
value.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the component
fell back to its Null object, and nothing will be delivered.
