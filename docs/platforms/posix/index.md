# Posix

`Platform/Posix/` wraps the standard POSIX APIs — BSD sockets, pthreads, POSIX
message queues, `clock_gettime`, stdio
([POSIX.1-2017 specification](https://pubs.opengroup.org/onlinepubs/9699919799/)).
Linux is the reference target.

Fills the Resolver, Datagram, Stream, Buffer, File and Mutex
[roles](../../roles/index.md), plus the clock / hostname / process-id / sleep
callbacks.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogGetAddrInfoResolver`](../../api/SolidSyslogGetAddrInfoResolver_8h.md) | resolver (`getaddrinfo`) |
| [`SolidSyslogPosixAddress`](../../api/SolidSyslogPosixAddress_8h.md) | address |
| [`SolidSyslogPosixDatagram`](../../api/SolidSyslogPosixDatagram_8h.md) | UDP sender |
| [`SolidSyslogPosixTcpStream`](../../api/SolidSyslogPosixTcpStream_8h.md) | TCP stream (non-blocking, bounded connect) |
| [`SolidSyslogPosixFile`](../../api/SolidSyslogPosixFile_8h.md) | file |
| [`SolidSyslogPosixMessageQueueBuffer`](../../api/SolidSyslogPosixMessageQueueBuffer_8h.md) | message-queue buffer |
| [`SolidSyslogPosixMutex`](../../api/SolidSyslogPosixMutex_8h.md) | mutex |
| [`SolidSyslogPosixClock`](../../api/SolidSyslogPosixClock_8h.md) | clock |
| [`SolidSyslogPosixHostname`](../../api/SolidSyslogPosixHostname_8h.md) | hostname |
| [`SolidSyslogPosixProcessId`](../../api/SolidSyslogPosixProcessId_8h.md) | process-id |
| [`SolidSyslogPosixSleep`](../../api/SolidSyslogPosixSleep_8h.md) | sleep |
| [`SolidSyslogPosixSysUpTime`](../../api/SolidSyslogPosixSysUpTime_8h.md) | uptime (`CLOCK_BOOTTIME`) |

## Requirements

A POSIX-conformant OS; Linux is the tested target. The message-queue buffer needs
POSIX message queues (link `-lrt` on glibc).

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

The hostname and process id are read from the OS and forwarded unmodified. They
identify the record's origin exactly as far as the OS can be trusted to report
it, and the library performs no independent check.

### The blocking surface is bounded but not zero

Sockets are non-blocking once open, so a send returns immediately against a
wedged peer. The initial connection is the bounded exception, and its budget is
a tunable. A deployment with a hard real-time deadline should drive delivery
from a service thread rather than the calling one.
