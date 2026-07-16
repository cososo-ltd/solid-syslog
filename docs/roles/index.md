# Roles

A role is one capability the library needs filled, defined as a vtable in
`SolidSyslog<Role>Definition.h`. A platform adapter, a Core implementation, or
your own code fills it. Every role has a Null fallback, so an unfilled slot
degrades safely instead of dangling at link time.

There are twelve. Each page below is that role's contract — the vtable itself,
and a generated diagram of the backends that realise it.

## Networking

| Role | Does |
|---|---|
| [Resolver](../api/structSolidSyslogResolver.md) | turn a host and port into an address |
| [Datagram](../api/structSolidSyslogDatagram.md) | send one UDP payload |
| [Stream](../api/structSolidSyslogStream.md) | carry bytes over a connection — plain TCP, or TLS layered over it |
| [Sender](../api/structSolidSyslogSender.md) | frame a record and deliver it |

## Storage

| Role | Does |
|---|---|
| [Buffer](../api/structSolidSyslogBuffer.md) | hold records between `Log` and `Service` |
| [Store](../api/structSolidSyslogStore.md) | store and forward across an outage |
| [BlockDevice](../api/structSolidSyslogBlockDevice.md) | block-indexed storage beneath a Store |
| [File](../api/structSolidSyslogFile.md) | the file primitive beneath a BlockDevice |

## OS primitives

| Role | Does |
|---|---|
| [Mutex](../api/structSolidSyslogMutex.md) | guard a Buffer shared across tasks |
| [AtomicCounter](../api/structSolidSyslogAtomicCounter.md) | the RFC 5424 `sequenceId` source |

## Evidence and integrity

| Role | Does |
|---|---|
| [StructuredData](../api/structSolidSyslogStructuredData.md) | emit one RFC 5424 SD element |
| [SecurityPolicy](../api/structSolidSyslogSecurityPolicy.md) | seal and open stored records |

## Bring your own

Filling a role is implementing its vtable. [Porting](../porting.md) puts the
twelve contracts side by side, each with its Null fallback and a reference
implementation.

Which platform fills a role on your target is the
[platform × capability matrix](../platforms/index.md).

<!-- markdownlint-disable MD033 — the sticky is styled HTML (.postit-note in brand.css); md_in_html keeps its body as Markdown. -->

<div class="postit-note" markdown>
**Or let us fill it.**

Nothing shipped backs the role you need? We write the platform adapter for you —
and the tests that prove it against the contract.
[Talk to us about it](https://www.cososo.co.uk/?service=solidsyslog#contact).
</div>

<!-- markdownlint-enable MD033 -->
