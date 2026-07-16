# FreeRTOS-Plus-TCP

`Platform/PlusTcp/` wraps FreeRTOS-Plus-TCP for networking on FreeRTOS targets
([FreeRTOS-Plus-TCP documentation](https://www.freertos.org/Documentation/03-Libraries/02-FreeRTOS-plus/02-FreeRTOS-plus-TCP/01-FreeRTOS-Plus-TCP)).

Fills the Resolver, Datagram and Stream [roles](../roles/index.md), plus the
address handle they share.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogPlusTcpResolver`](../api/SolidSyslogPlusTcpResolver_8h.md) | static IPv4 resolver (no DNS) |
| [`SolidSyslogPlusTcpAddress`](../api/SolidSyslogPlusTcpAddress_8h.md) | address (`freertos_sockaddr`) |
| [`SolidSyslogPlusTcpDatagram`](../api/SolidSyslogPlusTcpDatagram_8h.md) | UDP sender |
| [`SolidSyslogPlusTcpTcpStream`](../api/SolidSyslogPlusTcpTcpStream_8h.md) | TCP stream (bounded connect) |

## Requirements

FreeRTOS-Plus-TCP, selected at CMake time with
`SOLIDSYSLOG_FREERTOS_NET=PLUSTCP`. The resolver is a fixed IPv4 destination — no
DNS.
