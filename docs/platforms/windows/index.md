# Windows

`Platform/Windows/` wraps the Win32 and Winsock APIs for MSVC targets
([Winsock documentation](https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page-2)).

Fills the Resolver, Datagram, Stream, File, Mutex and AtomicCounter
[roles](../../roles/index.md), plus the clock / hostname / process-id / sleep / sysUpTime
callbacks.

## What it ships

## Requirements

The MSVC toolchain and Winsock - call `WSAStartup` once at process init before
creating a sender.

The keepalive timings below need Windows 10 version 1709 or later; older systems
keep the system defaults, and the adapter does not report the difference.

## Security behaviour and obligations

### The transport carries syslog in clear

Neither the datagram nor the TCP stream provides confidentiality, integrity or
peer authentication. TLS is a separate role filled by a different platform - the
[platform × capability matrix](../index.md) shows which - layered over this
stream rather than replacing it.

### An over-large record is trimmed to the path, not lost

Once the socket is connected the datagram asks Winsock for the path MTU and
reports what is left after the IPv4 and UDP headers. Before that, or where
Winsock does not answer, it reports 480 octets. That is the size RFC 5426 section 3.2
calls the safest assumption for IPv4 - every destination this adapter can
reach - and the conservative answer the
[Datagram](../../api/structSolidSyslogDatagram.md) contract asks for.

A send that exceeds the path is reported as oversize rather than as a failure, so
the sender trims the record to the payload just reported - on a UTF-8 codepoint
boundary - and sends it again. A record longer than the path therefore arrives
truncated rather than being dropped, and the truncation is visible to the
collector as a short message rather than as a gap.

### Winsock initialisation is yours

`WSAStartup` must be called once at process start, before any sender is created,
and the matching `WSACleanup` is yours to place. The adapter does not initialise
Winsock, because a process that already uses sockets has done it.

### A written record is the operating system's, not the disk's

The file layer writes and returns; nothing forces the data further down. A record
the store believes is stored survives the process exiting and may not survive the
machine losing power, so durability is a property of the volume you point the
store at.

### A failed file call is reported, with its errno

Each call the file makes on an open file, and the open itself, is reported
under the file's own error source when it fails, as
[`SOLIDSYSLOG_CAT_FILE_IO_FAILED`](../../api/SolidSyslogFileCategories_8h.md)
with a code naming the call: open, read, write, seek, size, truncate or close.
Nothing here syncs, so the sync and commit codes are never raised. The event's
`NativeCode` is the C run-time `errno`: the value `_sopen_s` and `_chsize_s`
answer directly, and the one the other calls leave behind. A short `_write`
sets none, so `HasNativeCode` is clear on its event. A short read is not a
failure and raises nothing: the store judges it for itself. Nor is a path that
is not there when the file is only asked whether it exists.
[Error severity](../../error-severity.md) gives the level of each.

### A failed connect is reported, with its Winsock error

Each step of a TCP connect that fails is reported under the stream's own error
source, with the detail naming the step, as
[`SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED`](../../api/SolidSyslogStreamCategories_8h.md).
Where Winsock left an error - `WSAGetLastError` after `socket`, `ioctlsocket`,
`connect` or `select`, the pending `SO_ERROR` of a socket the wait ended on
unconnected, or `WSAGetLastError` after the `getsockopt` that could not read
it - the event's `NativeCode` carries it. Winsock answers a refused connect by
flagging the socket in `select`'s exception set, so a refusal normally carries
the `SO_ERROR` it left, `WSAECONNREFUSED` among them. The connect budget
expiring leaves none.

### A collector that closes the connection costs no record

A send first peeks at the socket to learn whether the peer has closed its end,
as the [Stream](../../api/structSolidSyslogStream.md) contract requires. A
socket stays writable after a peer closes, so without that check Winsock would
take a record nothing can deliver, and only the peer's reset would say so, by
which time the record has been reported as delivered. When the peek sees the
close, the send fails, the stream closes itself, and your store replays the
record on the next connection.

The peek sees only a close Winsock has already learned of: a peer that closes
while the record is being sent can still cost that record, because syslog over
TCP has no acknowledgement to say it arrived. Nor can it see past bytes the
peer sent that nothing has read: a close behind them goes unnoticed. A syslog
collector sends nothing back, and a TLS stream layered on this one reads what
is waiting before every write, so in practice nothing is queued when the peek
looks.

### A peer that dies silently is detected, and you size how long that takes

Keepalive probes begin after `SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS` of silence
and repeat at `SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS` until
`SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT` go unanswered. There is no user-timeout
setting here, so a peer that dies with a write in flight is bounded by the
system's retransmission behaviour instead. Sends into a connection not yet
declared dead are accepted and reported as delivered, so the records inside that
window are released by store-and-forward and lost with it. The window is the idle
period plus the interval across every probe, so shortening it costs probe traffic
on an idle link.

### Protection of the store is a property of its directory

Where the store is written, and which accounts can reach it, are decided by the
directory you choose and the access control on it. The files are opened with
sharing permitted, so another process may read or write them while the store is
running. The library sets no policy of its own and checks none.

### Host identity is only as good as the operating system's

The host name is what Windows reports as the physical DNS host name, and the
process id is the one Windows assigns. The library performs no independent check
of either.
