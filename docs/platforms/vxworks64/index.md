# VxWorks 6.4

`Platform/VxWorks64/` wraps the VxWorks 6.4 kernel API for kernel (VIP) builds.
It is written against the publicly documented API and verified on 6.4; it may
serve as a model for other VxWorks releases, but nothing here has run on one.

Fills the Datagram, Stream (TCP), Resolver, File, Mutex and AtomicCounter
[roles](../../roles/index.md), plus the address handle the transports read back
to send. It also supplies the clock, hostname, sleep and sysUpTime callbacks. There is no
process-id callback: a kernel task belongs to no process, so PROCID is left
unset and sent as the nil value.

## What it ships

## Requirements

A VxWorks 6.4 kernel image and the VxWorks headers on your include path. The
datagram calls `socket`, `sendto` and `close`; the TCP stream calls `socket`,
`connectWithTimeout`, `setsockopt`, `send`, `recv` and `close`; the resolver
calls `inet_addr` and `hostGetByName`; the mutex calls `semMCreate`, `semTake`,
`semGive` and `semDelete`; and the atomic counter calls `intLock` and
`intUnlock`. The image needs the network stack, with TCP for the stream, and the
host library for the transports and the resolver, and mutual-exclusion
semaphores for the mutex. The clock calls
`clock_gettime` and `gmtime_r`, which the image must include; uptime
calls `tick64Get` and `sysClkRateGet`; the hostname calls `gethostname`; and
sleep calls `taskDelay`. The file calls `open`, `read`, `write`, `lseek`,
`ioctl`, `remove` and `close`, and needs the store's directory on a volume the
image has already mounted. Real-time processes (RTPs) are not supported.

The sources are C99. They need nothing from the compiler beyond that, and use no
toolchain-specific extensions. The flags a VIP generates select C89, so with
Diab the library is built through `Platform/VxWorks64/solidsyslog-vxworks64.mk`,
which adds the C99 dialect and states each diagnostic it turns off, and why.

The kernel header tree has no `<stdint.h>` or `<stdbool.h>`, which the
SolidSyslog headers include. The pack supplies both, for 32-bit targets, in
`Platform/VxWorks64/Compat/`. The library's own build puts that directory first,
ahead of the project's include path, which in a DKM can hold an incomplete
`<stdint.h>`. In a VIP, application files that include a SolidSyslog header
take it last, so it only fills the gap. A DKM's own sources do not take it at
all: their headers may define integer types of their own that would conflict.

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
collector on another machine. The BDD target runs the mutex, guarding the buffer
its console and service tasks share.

With dosFs, and again with HRFS, on the image's IDE disk, the file has carried
the BDD store scenarios: records stored while the collector was down were sent once it
returned, and records still in the store when QEMU was stopped mid-run were
replayed after it started again.

## Security behaviour and obligations

### The transports carry syslog in clear

Neither the datagram nor the TCP stream provides confidentiality, integrity or
peer authentication. The pack has no TLS.

### Nothing in the stream waits on a peer

The stream's connect is bounded by the deadline its config supplies, through
`connectWithTimeout`, rather than by the stack's own retransmission budget. An
attempt still under way when the deadline passes is abandoned with its socket,
and the sender tries again on its next pass. Its send and read pass
`MSG_DONTWAIT`, so neither stalls the task: a send the stack cannot take whole
fails and closes the stream, and a read with nothing waiting returns 0 and keeps
the connection.

A send first checks whether the peer has closed its end. A connection stays
writable after a peer closes, so without that check the stack would take a
record nothing can deliver and the record would be gone; instead the send
fails, the stream closes itself, and your store replays the record on the next
connection. The check sees only a close the stack has already learned of: a
peer that closes while the record is being sent can still lose that record,
because syslog over TCP has no acknowledgement to say it arrived. A record the
stack takes only part of fails and is replayed in the same way.

A connect that fails is reported under the stream's own error source, with the
detail naming which step failed. The stack refusing a socket option is
reported as a warning, and the connection stands without it.

### Dead-peer detection is yours to size

The stream turns keepalive on for its own connection with `SO_KEEPALIVE`. VxWorks
6.4 sets keepalive timing for the whole stack rather than per socket, so the
`SOLIDSYSLOG_TCP_KEEPALIVE_*` tunables do not apply on this platform: a silent
peer is probed on the stack's schedule, which is yours to set for the image. A
connection actually carrying records notices sooner, because the send fails and
the stream closes itself so the sender reconnects.

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

### A write counts once the file system has synced it

The file reports a write as done only when `write` took every byte, `FIOSYNC`
on the file then succeeded, and `FIOCOMMITFS` either succeeded or answered
`ENOTSUP`; anything less is a failed write. What the commit means depends on
the volume:

- On HRFS, every write commits its own transaction before it returns, and
  `FIOSYNC` has nothing left to do. HRFS passes `FIOCOMMITFS` down to the block
  device, and the ATA driver answers `ENOTSUP`, which counts as nothing to
  commit.
- On dosFs, `FIOSYNC` writes the file's cached data to the disk. dosFs turns
  `FIOCOMMITFS` into a commit request to the block device beneath it: the ATA
  disk has no transaction and answers OK.
- On dosFs over the transactional block layer (`INCLUDE_XBD_TRANS`),
  `FIOCOMMITFS` commits that layer's transaction, and a commit that fails is a
  failed write. That layer has not been exercised.

Durability has been exercised on dosFs and on HRFS, each on the image's IDE
disk.

### A path that will not open counts as deleted

Delete reports success when `remove` succeeds, and also when it fails but the
path then will not open, which is how an already-absent path shows. A volume
that has become unavailable shows the same way, so a block file still on it
counts as deleted: the store forgets the block, and finds it again at its next
start-up. Nothing is reported either way.
[#965](https://github.com/cososo-ltd/solid-syslog/issues/965) tracks telling
the two apart and reporting a delete that fails.

### Mounting and formatting are yours

The file opens the paths the block device gives it, under a volume the image
has already mounted, and never mounts or formats one itself. Bring the volume
up before the store is created; with no volume there, the file cannot open
the store's files.

### Log from a task, not an interrupt

Lock waits on the semaphore without a timeout, and VxWorks does not allow a
mutual-exclusion semaphore to be taken from an interrupt service routine. Call
`SolidSyslog_Log` from a task.
