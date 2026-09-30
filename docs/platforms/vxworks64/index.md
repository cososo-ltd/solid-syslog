# VxWorks 6.4

`Platform/VxWorks64/` wraps the VxWorks 6.4 kernel API for kernel (VIP) builds.
It is written against the publicly documented API and verified on 6.4; it may
serve as a model for other VxWorks releases, but nothing here has run on one.
Storage and time come from separate platforms; the
[capability matrix](../index.md) shows which fill them.

Fills the Datagram, Resolver and Mutex [roles](../../roles/index.md), plus the
address handle the datagram reads back to send. TCP is not yet supported.

## What it ships

## Requirements

A VxWorks 6.4 kernel image and the VxWorks headers on your include path. The
datagram calls `socket`, `sendto` and `close`; the resolver calls `inet_addr`
and `hostGetByName`; the mutex calls `semMCreate`, `semTake`, `semGive` and
`semDelete`. The image needs the network stack and the host library for the
first two, and mutual-exclusion semaphores for the third. Real-time processes
(RTPs) are not supported.

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

### The transport carries syslog in clear

The datagram provides no confidentiality, integrity or peer authentication.

### Resolution is by literal, then by host library

A dotted IPv4 literal is taken as it stands. Anything else goes to
`hostGetByName`, which consults the image's host table and, where the image
includes it, the DNS client - so a lookup by name may block while the query is
outstanding. A failed lookup fails that send, and the sender resolves again on
its next one.

Both calls answer all ones for a host they cannot resolve, so
`255.255.255.255` cannot be used as a collector address.

### An over-large record is refused, not fragmented

The stack offers no path-MTU query for UDP, so the datagram reports the
conservative payload limit the library uses for an unknown IPv4 path. A record
the stack still refuses with `EMSGSIZE` is reported as oversize, and the sender
treats it as such.

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
