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
toolchain-specific extensions. The flags a VIP generates select C89, so with
Diab the library is built through `Platform/VxWorks64/solidsyslog-vxworks64.mk`,
which adds the C99 dialect and states each diagnostic it turns off, and why.

The kernel header tree has no `<stdint.h>` or `<stdbool.h>`, which the
SolidSyslog headers include. The pack supplies both, for 32-bit targets, in
`Platform/VxWorks64/Compat/`. Put that directory last on the include path, so it
only fills the gap - for the library, and for every application file that
includes a SolidSyslog header.

The pack's public headers include no VxWorks header; the semaphore and every
other kernel type stay inside its sources. An application file that includes
them therefore compiles in the project's own dialect, and nothing the library is
compiled with reaches it.

## What it has run on

Its unit tests run against fakes that supply the subset of the VxWorks API the
pack calls, declared from the public API reference. That lets them build in an
ordinary host preset with no Wind River installation present, and the same
host build compiles the pack at strict C99.

Target runs are outside CI - the toolchain and the kernel are licensed - so
this section records them as they are made.

It has been built for VxWorks 6.4 on MIPS32 with Diab, and booted under QEMU's
Malta machine. The library compiles with the image's own flags, which include
`-Xlint`, plus the C99 dialect it adds; so built, the pack, Core and an
application file including every public header compile with no diagnostics. The
mutex is not yet exercised on the target: the BDD target will be the first to
run it.

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
