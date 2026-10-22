# FreeRTOS

`Platform/FreeRtos/` wraps FreeRTOS kernel primitives
([FreeRTOS documentation](https://www.freertos.org/Documentation/00-Overview)).
Networking comes from a separate platform; the
[capability matrix](../index.md) shows which fill it.

Fills the Mutex [role](../../roles/index.md), plus a sysUpTime callback. It ships
no clock and no sleep callback. Where a backend takes a
`SolidSyslogSleepFunction`, wrap `vTaskDelay` and round a non-zero request up to
one tick: `pdMS_TO_TICKS` rounds down, and below 1000 Hz a 1 ms request becomes
`vTaskDelay(0)`, which only yields.

## What it ships

## Requirements

`configSUPPORT_STATIC_ALLOCATION=1` and `configUSE_MUTEXES=1` - the mutex is a
statically allocated mutex semaphore. Without both, the mutex compiles to nothing
and `SolidSyslogFreeRtosMutex_Create` does not link.

FreeRTOS kernel V10.6.0 or later. The uptime callback selects its
implementation on `configTICK_TYPE_WIDTH_IN_BITS`, which that release
introduced. An older kernel defines neither it nor the `TICK_TYPE_WIDTH_*`
constants, and the preprocessor reads an undefined name as zero, so both
implementations compile in and the link fails on the duplicate definition. The
pack is tested against V11.1.0.

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

The sysUpTime callback reports hundredths of a second since boot, counted from
kernel ticks. It is not wall-clock
time and carries no timezone or synchronisation quality - the clock callback is
a separate injection point.

`SolidSyslogFreeRtos_GetSysUpTime` meets the
[sysUpTime contract](../../api/SolidSyslogMetaSd_8h.md) on a 32- or 64-bit
`TickType_t`, wrapping at about 497 days as RFC 3418 requires. A 16-bit one
resolves to no implementation and will not link; supply your own
`SolidSyslogSysUpTimeFunction` there.

The wraps are counted on a 32-bit counter, because above 100 Hz it reaches
its own wrap first - ten times sooner at 1000 Hz. That costs two things. The
callback must be reached once per wrap, roughly 50 days at 1000 Hz, which
formatting any message does. And it takes a short critical section, so it is
safe from any task but not from an interrupt.
