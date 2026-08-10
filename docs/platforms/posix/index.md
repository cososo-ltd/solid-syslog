# Posix

`Platform/Posix/` wraps the standard POSIX APIs — BSD sockets, pthreads, POSIX
message queues, `clock_gettime`, stdio
([POSIX.1-2017 specification](https://pubs.opengroup.org/onlinepubs/9699919799/)).
Linux is the reference target.

Fills the Resolver, Datagram, Stream, Buffer, File and Mutex
[roles](../../roles/index.md), plus the clock / hostname / process-id / sleep
callbacks.

## What it ships

## Requirements

A POSIX-conformant OS; Linux is the tested target. The message-queue buffer needs
POSIX message queues.

## Security behaviour and obligations

### The transport carries syslog in clear

Neither the datagram nor the TCP stream provides confidentiality, integrity or
peer authentication. Anything on the path can read and alter the records. TLS is
a separate role filled by a different platform — the
[platform × capability matrix](../index.md) shows which — layered over this
stream rather than replacing it.

### The store file is owner-only; its directory is yours

Files are created readable and writable by the owning user alone. That protects
the file, not the path to it: the ownership and permissions of the directory you
place the store in, and whether another process on the device runs as the same
user, are yours to set and to verify.

### Host identity is only as good as the operating system's

The host name is what `gethostname` reports and the process id is `getpid`. They
identify the record's origin exactly as far as the OS can be trusted to report
it, and the library performs no independent check.

### A written record is the kernel's, not the disk's

The file layer writes and returns; nothing calls `fsync`. A record the store
believes is stored survives the process exiting and may not survive the machine
losing power, so durability is a property of the filesystem and volume you point
the store at.

### A peer that dies silently is detected in 30 to 85 seconds

Keepalive probes after 45 seconds idle, then four times at ten-second intervals,
and a write already in flight is bounded separately by a 30-second user timeout.
Until one of those fires, sends into the dead connection are accepted by the
kernel and reported as delivered, so the records inside that window are released
by store-and-forward and lost with the connection.

### The message-queue buffer is capped by the system, not by you

`SolidSyslogPosixMessageQueueBuffer_Create` passes its `maxMessages` and
`maxMessageSize` to `mq_open`, and an unprivileged process cannot exceed
`fs/mqueue/msg_max` and `fs/mqueue/msgsize_max` — 10 messages and 8192 bytes on a
default Linux, where asking for 11 already fails. A failed open reports `ERROR`
and returns the Null buffer, so nothing is delivered through it. Size the queue
within the limits, or raise them for the deployment.

### The blocking surface is bounded but not zero

The TCP stream's socket is non-blocking once open, so a send returns immediately
against a wedged peer. The initial connection is the bounded exception, and its
budget is a tunable. The UDP socket is left blocking: a datagram send is not
expected to wait, but nothing here bounds it if the kernel's send buffer fills. A deployment with a hard real-time deadline should drive delivery
from a service thread rather than the calling one.
