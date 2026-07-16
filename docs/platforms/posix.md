# Posix

`Platform/Posix/` wraps the standard POSIX APIs — BSD sockets, pthreads, POSIX
message queues, `clock_gettime`, stdio
([POSIX.1-2017 specification](https://pubs.opengroup.org/onlinepubs/9699919799/)).
Linux is the reference target.

Fills the Resolver, Datagram, Stream, Buffer, File and Mutex
[roles](../roles/index.md), plus the clock / hostname / process-id / sleep
callbacks.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogGetAddrInfoResolver`](../api/SolidSyslogGetAddrInfoResolver_8h.md) | resolver (`getaddrinfo`) |
| [`SolidSyslogPosixAddress`](../api/SolidSyslogPosixAddress_8h.md) | address |
| [`SolidSyslogPosixDatagram`](../api/SolidSyslogPosixDatagram_8h.md) | UDP sender |
| [`SolidSyslogPosixTcpStream`](../api/SolidSyslogPosixTcpStream_8h.md) | TCP stream (non-blocking, bounded connect) |
| [`SolidSyslogPosixFile`](../api/SolidSyslogPosixFile_8h.md) | file |
| [`SolidSyslogPosixMessageQueueBuffer`](../api/SolidSyslogPosixMessageQueueBuffer_8h.md) | message-queue buffer |
| [`SolidSyslogPosixMutex`](../api/SolidSyslogPosixMutex_8h.md) | mutex |
| [`SolidSyslogPosixClock`](../api/SolidSyslogPosixClock_8h.md) | clock |
| [`SolidSyslogPosixHostname`](../api/SolidSyslogPosixHostname_8h.md) | hostname |
| [`SolidSyslogPosixProcessId`](../api/SolidSyslogPosixProcessId_8h.md) | process-id |
| [`SolidSyslogPosixSleep`](../api/SolidSyslogPosixSleep_8h.md) | sleep |
| [`SolidSyslogPosixSysUpTime`](../api/SolidSyslogPosixSysUpTime_8h.md) | uptime (`CLOCK_BOOTTIME`) |

## Requirements

A POSIX-conformant OS; Linux is the tested target. The message-queue buffer needs
POSIX message queues (link `-lrt` on glibc).
