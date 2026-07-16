# Windows

`Platform/Windows/` wraps the Win32 and Winsock APIs for MSVC targets
([Winsock documentation](https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page-2)).

Fills the Resolver, Datagram, Stream, File, Mutex and AtomicCounter
[roles](../roles/index.md), plus the clock / hostname / process-id / sleep
callbacks.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogWinsockResolver`](../api/SolidSyslogWinsockResolver_8h.md) | resolver |
| [`SolidSyslogWinsockAddress`](../api/SolidSyslogWinsockAddress_8h.md) | address |
| [`SolidSyslogWinsockDatagram`](../api/SolidSyslogWinsockDatagram_8h.md) | UDP sender |
| [`SolidSyslogWinsockTcpStream`](../api/SolidSyslogWinsockTcpStream_8h.md) | TCP stream (non-blocking + `select`, bounded connect) |
| [`SolidSyslogWindowsFile`](../api/SolidSyslogWindowsFile_8h.md) | file |
| [`SolidSyslogWindowsMutex`](../api/SolidSyslogWindowsMutex_8h.md) | mutex (`CRITICAL_SECTION`) |
| [`SolidSyslogWindowsAtomicCounter`](../api/SolidSyslogWindowsAtomicCounter_8h.md) | atomic counter (`Interlocked`) |
| [`SolidSyslogWindowsClock`](../api/SolidSyslogWindowsClock_8h.md) | clock |
| [`SolidSyslogWindowsHostname`](../api/SolidSyslogWindowsHostname_8h.md) | hostname |
| [`SolidSyslogWindowsProcessId`](../api/SolidSyslogWindowsProcessId_8h.md) | process-id |
| [`SolidSyslogWindowsSleep`](../api/SolidSyslogWindowsSleep_8h.md) | sleep |
| [`SolidSyslogWindowsSysUpTime`](../api/SolidSyslogWindowsSysUpTime_8h.md) | uptime (`GetTickCount64`) |

## Requirements

The MSVC toolchain and Winsock — call `WSAStartup` once at process init before
creating a sender.
