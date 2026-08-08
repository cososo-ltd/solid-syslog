# Windows

`Platform/Windows/` wraps the Win32 and Winsock APIs for MSVC targets
([Winsock documentation](https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-start-page-2)).

Fills the Resolver, Datagram, Stream, File, Mutex and AtomicCounter
[roles](../../roles/index.md), plus the clock / hostname / process-id / sleep
callbacks.

## What it ships

## Requirements

The MSVC toolchain and Winsock — call `WSAStartup` once at process init before
creating a sender.

## Security behaviour and obligations

### The transport carries syslog in clear

Neither the datagram nor the TCP stream provides confidentiality, integrity or
peer authentication. TLS is a separate role filled by a different platform — the
[platform × capability matrix](../index.md) shows which — layered over this
stream rather than replacing it.

### Winsock initialisation is yours

`WSAStartup` must be called once at process start, before any sender is created,
and the matching `WSACleanup` is yours to place. The adapter does not initialise
Winsock, because a process that already uses sockets has done it.

### Protection of the store is a property of its directory

Where the store is written, and which accounts can reach it, are decided by the
directory you choose and the access control on it. The library sets no policy of
its own and checks none.

### Host identity is only as good as the operating system's

The hostname and process id are read from Windows and forwarded unmodified. The
library performs no independent check of either.
