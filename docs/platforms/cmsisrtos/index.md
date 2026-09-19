# CMSIS-RTOS2

`Platform/CmsisRtos/` wraps the CMSIS-RTOS2 API
([CMSIS-RTOS2 documentation](https://arm-software.github.io/CMSIS_6/latest/RTOS2/index.html)).
It targets the API rather than any one kernel, so the same adapter serves every
kernel that implements it. Networking, storage and time come from separate
platforms; the [capability matrix](../index.md) shows which fill them.

Fills the Mutex [role](../../roles/index.md).

## What it ships

## Requirements

An implementation of the CMSIS-RTOS2 mutex API, and its `cmsis_os2.h` on your
include path. The adapter calls `osMutexNew`, `osMutexAcquire`,
`osMutexRelease` and `osMutexDelete` and nothing else, so no other part of the
API needs to be present or configured.

## Security behaviour and obligations

### The mutex guards a buffer shared between tasks

The circular buffer uses it when the task calling `Log` is not the task calling
`Service`. Where both run on one task, the Null mutex is the correct choice and
costs nothing.

### You size the control block, because only you can

CMSIS-RTOS2 does not standardise how large a mutex control block is - each
implementation decides, and the API offers no way to ask. So the library does
not guess: `SolidSyslogCmsisRtosMutex_Create` takes the storage and its size
from you and passes both through untouched. The storage must outlive the mutex,
and it is the RTOS object itself rather than a copy of one.

Get the size wrong and the implementation refuses it. That is reported as a
`CRITICAL` at create time and the mutex falls back to the Null object, whose
Lock and Unlock are no-ops - so a buffer shared across tasks would be left
unguarded. Treat that event as a build defect rather than a runtime condition:
it cannot start happening later, because the size is fixed at compile time.

### Nothing is allocated, unless you ask for it

Supplying the control block is what keeps the adapter allocation-free, which is
what makes it usable where a heap is unavailable or prohibited. Passing NULL
and a zero size instead is the form CMSIS-RTOS2 defines for letting the
implementation allocate the control block from its own pool; the adapter passes
that through as readily, and an implementation configured for static allocation
only will refuse it.

### Priority inheritance is requested, not guaranteed

The mutex is created with `osMutexPrioInherit` set, so a low-priority task
holding the lock is not left preempted indefinitely while a high-priority task
waits. CMSIS-RTOS2 leaves the attribute optional: an implementation whose
mutexes always inherit ignores the bit, and one that supports neither will
create the mutex without it. Where priority inversion is a hazard you must
bound, confirm your implementation honours it.
