# Windows

`Platform/Windows/` wraps the Win32 and Winsock APIs for MSVC targets
([Winsock documentation](https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page-2)).

Fills the Resolver, Datagram, Stream, File, Mutex and AtomicCounter
[roles](../../roles/index.md), plus the clock / hostname / process-id / sleep
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
Winsock does not answer, it reports 480 octets. That is the size RFC 5426 §3.2
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
