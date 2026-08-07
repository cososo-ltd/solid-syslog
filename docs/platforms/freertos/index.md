# FreeRTOS

`Platform/FreeRtos/` wraps FreeRTOS kernel primitives
([FreeRTOS documentation](https://www.freertos.org/Documentation/00-Overview)).
Networking comes from a separate platform; the
[platform × capability matrix](../index.md) shows which fill it.

Fills the Mutex [role](../../roles/index.md), plus a sysUpTime callback.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogFreeRtosMutex`](../../api/SolidSyslogFreeRtosMutex_8h.md) | mutex (`xSemaphoreCreateMutexStatic`) |
| [`SolidSyslogFreeRtosSysUpTime`](../../api/SolidSyslogFreeRtosSysUpTime_8h.md) | uptime (`xTaskGetTickCount`) |

## Requirements

`configSUPPORT_STATIC_ALLOCATION=1` — the mutex uses static allocation.

## Security behaviour and obligations

### The mutex is what makes a shared buffer safe

Where the application task calling `Log` and the task calling `Service` are
different, the buffer between them needs this mutex. Leaving the role unfilled
resolves to the Null mutex, which is a working no-op on a single-task target and
a silent data race on a multi-task one. Nothing reports the difference, because
nothing can detect it.

### Static allocation is required, and is the point

`configSUPPORT_STATIC_ALLOCATION=1` is not a convenience: the kernel object is
created from storage inside the library's own pool, so the adapter allocates
nothing at run time and cannot fail for want of heap.

### Uptime is a tick count, not a clock

The sysUpTime callback reports kernel ticks since boot. It is not wall-clock
time and carries no timezone or synchronisation quality — the clock callback is
a separate injection point.
