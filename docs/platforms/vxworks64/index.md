# VxWorks 6.4

`Platform/VxWorks64/` wraps the VxWorks 6.4 kernel API for kernel (VIP) builds.
It is written against the publicly documented API and verified on 6.4; it may
serve as a model for other VxWorks releases, but nothing here has run on one.
Storage comes from a separate platform; the
[capability matrix](../index.md) shows which fill it.

Fills the Datagram, Resolver, Mutex and AtomicCounter
[roles](../../roles/index.md), plus the address handle the datagram reads back
to send. TCP is not yet supported. It
also supplies the clock, hostname, sleep and sysUpTime callbacks. There is no
process-id callback: a kernel task belongs to no process, so PROCID is left
unset and sent as the nil value.

## What it ships

## Requirements

A VxWorks 6.4 kernel image and the VxWorks headers on your include path. The
datagram calls `socket`, `sendto` and `close`; the resolver calls `inet_addr`
and `hostGetByName`; the mutex calls `semMCreate`, `semTake`, `semGive` and
`semDelete`; and the atomic counter calls `intLock` and `intUnlock`. The image
needs the network stack and the host library for the first two, and
mutual-exclusion semaphores for the third. The clock calls
`clock_gettime` and `gmtime_r`, which the image must include; uptime
calls `tick64Get` and `sysClkRateGet`; the hostname calls `gethostname`; and
sleep calls `taskDelay`. Real-time processes (RTPs) are not supported.

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

The whole pack has been built for VxWorks 6.4 on MIPS32 with Diab, and booted
under QEMU's Malta machine. The library compiles with the image's own flags,
which include `-Xlint`, plus the C99 dialect it adds; so built, the pack, Core
and an application file including every public header compile with no
diagnostics.

On that image, the UDP transport and the resolver have delivered a message,
resolved from a dotted address, through QEMU's user network to a syslog-ng
collector on another machine. The mutex is not yet exercised on the target: the
BDD target will be the first to run it.

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

### A record is trimmed to fit, never fragmented

The stack offers neither a path-MTU query nor a don't-fragment option for UDP,
so it would fragment a record too large for the path rather than refuse it. The
datagram therefore refuses one itself: a record larger than its payload limit,
the conservative figure for an unknown IPv4 path
(`SolidSyslogUdpPayload_UnknownPath`), is reported as oversize without reaching
the stack, and the sender trims it to fit and sends it again. A send the stack
refuses with `EMSGSIZE` is reported as oversize in the same way.

Raising `SOLIDSYSLOG_MAX_MESSAGE_SIZE` above that limit therefore does not
produce larger datagrams on this platform; it only lengthens the records the
sender has to trim.

### The clock is only as right as whatever set it

The clock reads `CLOCK_REALTIME` and reports it in UTC; the library never sets
it. On a board without a battery-backed clock it runs from wherever the kernel
started it until something sets it, and every record sent before then carries
that time. If the clock cannot be read the record is sent with no timestamp at
all.

### Time quality is yours to report

VxWorks 6.4 has no standard call that says whether the clock is synchronised,
or how closely, so the pack supplies no time-quality callback. Whatever sets
the clock knows; report it in the callback you give the time-quality
structured data.

### Host identity is only as good as the kernel's

The hostname is what `gethostname` reports, up to `MAXHOSTNAMELEN` characters.
It identifies the record's origin exactly as far as whatever set it can be
trusted, and the library performs no independent check.

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

### The atomic counter assumes a single CPU

The counter increments with interrupts locked, which excludes every other
writer, tasks and interrupt service routines alike, only because VxWorks 6.4
runs on one CPU. Interrupts stay locked only for the read, the compare and the
store.

### Log from a task, not an interrupt

Lock waits on the semaphore without a timeout, and VxWorks does not allow a
mutual-exclusion semaphore to be taken from an interrupt service routine. Call
`SolidSyslog_Log` from a task.
