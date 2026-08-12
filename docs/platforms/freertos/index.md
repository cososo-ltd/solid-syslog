# FreeRTOS

`Platform/FreeRtos/` wraps FreeRTOS kernel primitives
([FreeRTOS documentation](https://www.freertos.org/Documentation/00-Overview)).
Networking comes from a separate platform; the
[platform × capability matrix](../index.md) shows which fill it.

Fills the Mutex [role](../../roles/index.md), plus a sysUpTime callback.

## What it ships

## Requirements

`configSUPPORT_STATIC_ALLOCATION=1` — the mutex uses static allocation.

## Security behaviour and obligations

### The mutex guards a buffer shared between tasks

The circular buffer uses it when the task calling `Log` is not the task calling
`Service`. Where both run on one task, the Null mutex is the correct choice and
costs nothing.

### Static allocation is required, and is the point

`configSUPPORT_STATIC_ALLOCATION=1` is not a convenience: the kernel object is
created from storage inside the library's own pool, so the adapter allocates
nothing at run time and cannot fail for want of heap.

### Uptime is a tick count, not a clock

The sysUpTime callback reports kernel ticks since boot. It is not wall-clock
time and carries no timezone or synchronisation quality — the clock callback is
a separate injection point.

> [!WARNING]
> `SolidSyslogFreeRtos_GetSysUpTime` meets the
> [sysUpTime contract](../../api/SolidSyslogMetaSd_8h.md) for a 64-bit
> `TickType_t` at any tick rate, and for a 32-bit one whose `configTICK_RATE_HZ`
> divides 100. At every other rate the tick counter rolls over before 2^32
> hundredths do, and the reported uptime loses phase there and returns to zero:
> after 2^32 / `configTICK_RATE_HZ` seconds rather than RFC 3418's 497 days, so
> roughly 50 days at the 1000 Hz FreeRTOS default and sooner as the rate rises.
> Supply your own `SolidSyslogSysUpTimeFunction` from a time source you already
> have, or move to a dividing rate or a 64-bit tick type. Converting correctly
> at any tick rate is tracked as
> [#755](https://github.com/cososo-ltd/solid-syslog/issues/755).
