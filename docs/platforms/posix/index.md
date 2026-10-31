# Posix

`Platform/Posix/` wraps the standard POSIX APIs - BSD sockets, pthreads, POSIX
message queues, `clock_gettime`, stdio
([POSIX.1-2017 specification](https://pubs.opengroup.org/onlinepubs/9699919799/)).
It needs Linux; see the requirements below.

Fills the Resolver, Datagram, Stream, Buffer, File and Mutex
[roles](../../roles/index.md), plus the clock / hostname / process-id / sleep / sysUpTime
callbacks.

## What it ships

## Requirements

Linux. The pack uses `CLOCK_BOOTTIME`, `IP_MTU`, `IP_MTU_DISCOVER` and
`TCP_USER_TIMEOUT`, which POSIX does not define. The message-queue buffer needs
POSIX message queues.

## Security behaviour and obligations

### The transport carries syslog in clear

Neither the datagram nor the TCP stream provides confidentiality, integrity or
peer authentication. Anything on the path can read and alter the records. TLS is
a separate role filled by a different platform - the
[capability matrix](../index.md) shows which - layered over this
stream rather than replacing it.

### An over-large record is trimmed to the path, not lost

Once the socket is connected the datagram asks the kernel for the path MTU and
reports what is left after the IPv4 and UDP headers. Before that, or where the
kernel does not answer, it reports 480 octets. That is the size RFC 5426 section 3.2
calls the safest assumption for IPv4 - every destination this adapter can
reach - and the conservative answer the
[Datagram](../../api/structSolidSyslogDatagram.md) contract asks for.

A send that exceeds the path is reported as oversize rather than as a failure, so
the sender trims the record to the payload just reported - on a UTF-8 codepoint
boundary - and sends it again. A record longer than the path therefore arrives
truncated rather than being dropped, and the truncation is visible to the
collector as a short message rather than as a gap.

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

### A failed file call is reported, with its errno

Each call the file makes on an open file, and the open itself, is reported
under the file's own error source when it fails, as
[`SOLIDSYSLOG_CAT_FILE_IO_FAILED`](../../api/SolidSyslogFileCategories_8h.md)
with a code naming the call: open, read, write, seek, size, truncate or close.
Nothing here syncs, so the sync and commit codes are never raised. Where the
call set `errno`, the event's `NativeCode` carries it. A short `write` sets no
`errno`, so `HasNativeCode` is clear on its event. A short read is not a
failure and raises nothing: the store judges it for itself. Nor is a path that
is not there when the file is only asked whether it exists.
[Error severity](../../error-severity.md) gives the level of each.

### A failed connect is reported, with its errno

Each step of a TCP connect that fails is reported under the stream's own error
source, with the detail naming the step, as
[`SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED`](../../api/SolidSyslogStreamCategories_8h.md).
Where the kernel left an `errno` - from `socket`, `fcntl`, `connect` or
`select`, the pending `SO_ERROR` of a socket the wait ended on unconnected,
or the `errno` of the `getsockopt` that could not read it - the event's
`NativeCode` carries it. The connect budget expiring leaves none, nor does a
socket `select` flags in its exception set with no pending error.

### A collector that closes the connection costs no record

A send first peeks at the socket to learn whether the peer has closed its end,
as the [Stream](../../api/structSolidSyslogStream.md) contract requires. A
socket stays writable after a peer closes, so without that check the kernel
would take a record nothing can deliver, and only the peer's reset would say so,
by which time the record has been reported as delivered. When the peek sees the
close, the send fails, the stream closes itself, and your store replays the
record on the next connection.

The peek sees only a close the kernel has already learned of: a peer that
closes while the record is being sent can still cost that record, because
syslog over TCP has no acknowledgement to say it arrived. Nor can it see past
bytes the peer sent that nothing has read: a close behind them goes unnoticed.
A syslog collector sends nothing back, so in practice nothing is queued when the
peek looks.

### A peer that dies silently is detected, and you size how long that takes

Keepalive probes begin after `SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS` of silence
and repeat at `SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS` until
`SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT` go unanswered. A write already in flight
is bounded separately by a user timeout, which probing does not cover. Until one
of those fires, sends into the dead connection are accepted by the kernel and
reported as delivered, so the records inside that window are released by
store-and-forward and lost with the connection. The window is the idle period
plus the interval across every probe, so shortening it costs probe traffic on an
idle link.

### The message-queue buffer is capped by the system, not by you

`SolidSyslogPosixMessageQueueBuffer_Create` passes its `maxMessages`, and
`SOLIDSYSLOG_MAX_MESSAGE_SIZE` as the message size, to `mq_open`, and an
unprivileged process cannot exceed `fs/mqueue/msg_max` and
`fs/mqueue/msgsize_max` - 10 messages and 8192 bytes on a default Linux, where
asking for 11 already fails. The message size is set only by the tunable, so one
raised past `msgsize_max` fails every open. A failed open reports `ERROR`, with
the errno as the event's native code, and returns the Null buffer, so nothing
is delivered through it. Size the queue within the limits, or raise them for
the deployment.

### The message-queue buffer has no name once it is open

The queue is created as a kernel object named `/solidsyslog_<pid>_<slot>`,
readable and writable by its owner alone, and the name is unlinked as soon as
the open returns. From then on the queue is reached only through the descriptor
this process holds, which a forked child inherits: no other process can open it
by name, and a process that dies takes its queue with it. A queue that already
exists under the name when the buffer creates its own is refused rather than
adopted, so a queue planted ahead of this process cannot hand it records or read
its own; the refusal leaves that queue in place.

### The blocking surface is bounded but not zero

The TCP stream's socket is non-blocking once open, so a send returns immediately
against a wedged peer. The initial connection is the bounded exception, and its
budget is a tunable. The UDP socket is left blocking: a datagram send is not
expected to wait, but nothing here bounds it if the kernel's send buffer fills. A deployment with a hard real-time deadline should drive delivery
from a service thread rather than the calling one.
