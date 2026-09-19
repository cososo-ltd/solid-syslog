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

`SolidSyslogFreeRtos_GetSysUpTime` meets the
[sysUpTime contract](../../api/SolidSyslogMetaSd_8h.md) at any tick rate on a
32-bit `TickType_t`, and on a 64-bit one, which needs no help to get there.
Above 100 Hz the counter reaches its own wrap before 2^32 hundredths do - at
1000 Hz, ten times sooner - so how often it has wrapped is carried alongside
it, and the reported value wraps where RFC 3418 says, at about 497 days.

A 16-bit `TickType_t`, which `configUSE_16_BIT_TICKS` selects, is not carried
past its own wrap: uptime returns to zero every 65536 ticks. Supply your own
`SolidSyslogSysUpTimeFunction` where that matters.

Two things follow from carrying that phase rather than deriving it. The
callback has to be reached at least once per counter rollover - about 50 days
at the 1000 Hz default - or a rollover passes unseen; every formatted message
reaches it, so only a device that logs nothing for that long is affected. And
it keeps state, so it takes a short critical section and must not be called
from an interrupt.
