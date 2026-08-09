# OpenSSL

`Platform/OpenSsl/` wraps [OpenSSL](https://docs.openssl.org/) for TLS transport
and keyed at-rest cryptography on hosted targets. It fills the
[Stream](../../api/structSolidSyslogStream.md) role with TLS and the
[SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role for at-rest
integrity and confidentiality.

## What it ships

## Requirements

OpenSSL 3.0 or later.

Credentials are file paths, read when the stream is opened: a PEM trust bundle,
and for mutual TLS a PEM client certificate chain and private key. A
`SolidSyslogSleepFunction` is required and has no default.

## Security behaviour and obligations

The per-field detail is in
[`SolidSyslogOpenSslStream.h`](../../api/SolidSyslogOpenSslStream_8h.md), alongside the
fields themselves. What follows is the behaviour of the adapter as a whole, and
the work it leaves to you.

### Transport security is fixed by the adapter

`SSL_VERIFY_PEER` is pinned on the context and the protocol floor is set to
TLS 1.2. Both are return-checked, so the stream fails to open rather than
proceeding if the underlying libssl refuses the floor. Every setup call on the
handshake path is checked in the same way, which is what prevents a handshake
completing without the identity check having been applied.

### The trust bundle is mandatory

The trust bundle must load. If it does not, the stream fails to open — there is
no fallback to a system trust store.

### Peer identity is yours to declare

The `ServerName` field supplies both the Server Name Indication sent in the
handshake and the identity checked against the peer certificate. It has a
distinct meaning when set, when empty, and when NULL — including one value that
disables endpoint verification without reporting anything — and the three are
documented on the field. Choosing between them is a deployment decision the
adapter cannot make.

### Mutual TLS is optional and all-or-nothing

A client certificate chain and its private key are supplied together or not at
all. Supplying one without the other is rejected when the stream is opened, so a
partially configured credential cannot result in a connection that silently
omits the client certificate. The key is also checked against the certificate
locally, before any bytes reach the network.

### The cipher policy is yours

A cipher list is passed through to OpenSSL unchanged, and omitting it takes the
OpenSSL default. The library pins no list of its own: the appropriate one
depends on the libssl build present on the target and on the profile the
deployment is held to.

### Rotation is a file replacement and a reconnection

The `SSL_CTX` is rebuilt each time the stream is opened, re-reading every
credential file the config names — the trust anchors always, the client
certificate and key only where mutual TLS is configured. So replacing them takes effect on the next connection — either through
ordinary reconnection after an outage, or by calling
`SolidSyslogSender_Disconnect` to force one. No reload callback is needed.

### Key custody is outside the library

The library holds no keys of its own and reads whatever material it is pointed
at. Filesystem permissions on the private key, whether it is held in a hardware
security module, and how it is rotated are properties of your deployment. The
same applies to the at-rest policies: HMAC-SHA256 and AES-256-GCM are keyed, and
storing and rotating that key is yours.

### Revocation is not checked

The adapter performs no revocation checking, by Certificate Revocation List or
by the Online Certificate Status Protocol. Where a deployment requires it, it
must come from your own configuration of OpenSSL, and confirming that it is in
force is part of your assessment rather than something the adapter reports.
