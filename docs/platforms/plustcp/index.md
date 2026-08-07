# FreeRTOS-Plus-TCP

`Platform/PlusTcp/` wraps FreeRTOS-Plus-TCP for networking on FreeRTOS targets
([FreeRTOS-Plus-TCP documentation](https://www.freertos.org/Documentation/03-Libraries/02-FreeRTOS-plus/02-FreeRTOS-plus-TCP/01-FreeRTOS-Plus-TCP)).

Fills the Resolver, Datagram and Stream [roles](../../roles/index.md), plus the
address handle they share.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogPlusTcpResolver`](../../api/SolidSyslogPlusTcpResolver_8h.md) | DNS resolver (`FreeRTOS_getaddrinfo`) |
| [`SolidSyslogPlusTcpAddress`](../../api/SolidSyslogPlusTcpAddress_8h.md) | address (`freertos_sockaddr`) |
| [`SolidSyslogPlusTcpDatagram`](../../api/SolidSyslogPlusTcpDatagram_8h.md) | UDP sender |
| [`SolidSyslogPlusTcpTcpStream`](../../api/SolidSyslogPlusTcpTcpStream_8h.md) | TCP stream (bounded connect) |

## Requirements

FreeRTOS-Plus-TCP, selected at CMake time with
naming `PlusTcp` in `SOLIDSYSLOG_PLATFORMS`. The resolver wraps `FreeRTOS_getaddrinfo`, so
your `FreeRTOSIPConfig.h` needs `ipconfigUSE_DNS=1`.

## Security behaviour and obligations

### The transport carries syslog in clear

Neither the datagram nor the TCP stream provides confidentiality, integrity or
peer authentication. TLS is a separate role filled by a different platform — the
[platform × capability matrix](../index.md) shows which — layered over this
stream rather than replacing it.

### Resolution is trusted as the stack returns it

The resolver forwards what FreeRTOS-Plus-TCP answers. A deployment that cannot
trust its DNS should give the collector a numeric address rather than a name, so
that no resolution step exists to be poisoned.

### The stack's configuration is yours

Buffer counts, socket limits and timer behaviour are set in your
`FreeRTOSIPConfig.h`. Sizing them for the number of adapter instances you create
is part of the integration, and exhaustion surfaces as a failure to send rather
than as a crash.
