# VxWorks 6.4

`Platform/VxWorks64/` wraps the VxWorks 6.4 kernel API for kernel (VIP) builds.
It is written against the publicly documented API and verified on 6.4; it may
serve as a model for other VxWorks releases, but nothing here has run on one.
Networking, storage and time come from separate platforms; the
[capability matrix](../index.md) shows which fill them.

Fills the Mutex [role](../../roles/index.md).

## What it ships

## Requirements

A VxWorks 6.4 kernel image with mutual-exclusion semaphore support, and the
VxWorks headers on your include path. The mutex calls `semMCreate`, `semTake`,
`semGive` and `semDelete`. Real-time processes (RTPs) are not supported.

The sources are C99. They need nothing from the compiler beyond that, and use no
toolchain-specific extensions.

## What it has run on

Its unit tests run against fakes that supply the subset of the VxWorks API the
pack calls, declared from the public API reference. That lets them build in an
ordinary host preset with no Wind River installation present, and the same
host build compiles the pack at strict C99.

It has not yet been run on a VxWorks target. Target runs are outside CI - the
toolchain and the kernel are licensed - so this section will record them as
they are made.

## Security behaviour and obligations

### The mutex guards a buffer shared between tasks

The circular buffer uses it when the task calling `Log` is not the task calling
`Service`. Where both run on one task, the Null mutex is the correct choice and
costs nothing.

### The kernel allocates the semaphore

`semMCreate` takes the semaphore from the kernel's memory; the library holds
only the id it returns, in a slot of its own static pool. If the kernel cannot
allocate it, that is reported as a `CRITICAL` at create time and the mutex falls
back to the Null object, whose Lock and Unlock are no-ops - so a buffer shared
across tasks would be left unguarded. Size the image's memory so that it cannot
happen.

### Priority inheritance and deletion safety are both on

The semaphore is created with `SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
SEM_DELETE_SAFE`. Inversion safety means a low-priority task holding the lock
inherits the priority of a higher-priority task waiting on it, rather than
being preempted indefinitely. Deletion safety means a task holding the lock
cannot be deleted until it releases it; without it, deleting that task would
leave the lock held and every task that logs would block.

### Log from a task, not an interrupt

Lock waits on the semaphore without a timeout, and VxWorks does not allow a
mutual-exclusion semaphore to be taken from an interrupt service routine. Call
`SolidSyslog_Log` from a task.
