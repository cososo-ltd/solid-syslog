# OpenSSL

`Platform/OpenSsl/` wraps [OpenSSL](https://docs.openssl.org/) for TLS transport
and keyed at-rest cryptography on hosted targets. It fills the
[Stream](../../api/structSolidSyslogStream.md) role with TLS and the
[SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role for at-rest
integrity and confidentiality.

What a TLS stream must do is the same whichever library provides it, and is
stated once under [TLS obligations](../../tls.md). This page covers what this
adapter needs, how credentials reach it, and where it does not yet meet that
contract.

## What it ships

## Requirements

OpenSSL 3.0 or later. The CMake configure fails below that rather than the build,
so an older libssl is caught before anything compiles.

A `SolidSyslogSleepFunction` is required and has no default.

## Credentials are file paths

Trust anchors, and for mutual TLS the client certificate chain and its private
key, are PEM files named in the configuration. The adapter reads them, so it
needs them present and readable by the process at the moment a connection is
made, not at startup.

The `SSL_CTX` is rebuilt on every open, re-reading each file named in the
configuration. Rotation is therefore a file replacement and a reconnection:
replace the file, and the new material is in force on the next connection —
either through ordinary reconnection after an outage, or immediately by calling
`SolidSyslogSender_Disconnect`. Nothing needs to be reloaded and nothing needs to
be restarted.

## Where it differs from the contract

Five differences at 0.1.0, each tracked. Read them before relying on the
corresponding obligation.

### The credential paths and the peer identity are fixed when the stream is created

New material written to the same paths is picked up on the next connection, as
above. Pointing the stream at a *different* path is not possible: the
configuration is copied when the stream is created and nothing can replace it
afterwards. `ServerName` is fixed the same way, so redirecting a device to
another collector through the endpoint callback leaves it checking the peer
certificate against the name it was created with. Tracked as `#735`.

### A half-supplied client credential stops delivery

A certificate without its key, or a key without its certificate, is rejected when
the stream opens, so nothing is delivered until the configuration is corrected.
The contract asks for it to be reported with delivery continuing, on the grounds
that the collector is the enforcement point for our own credential.

This adapter is stricter than the contract rather than weaker, and the stricter
behaviour is safe. Tracked as `#734`.

### An expired certificate stops delivery

A peer certificate that is expired or not yet valid fails the handshake, even
where it still chains to a trusted anchor. The contract asks for it to be
reported with delivery continuing, because clock skew is the dominant cause and a
device with a wrong clock is one whose logs you still want. Tracked as `#731`.

### The cipher policy does not bind a TLS 1.3 connection

The cipher list is passed to OpenSSL unchanged and pins nothing of the library's
own, as the contract asks. It governs TLS 1.2 and below only. OpenSSL has kept
TLS 1.3 ciphersuites in a separate list since 1.1.1, and this adapter sets a
protocol floor without a ceiling, so against a modern peer the negotiated
connection uses OpenSSL's own TLS 1.3 defaults and the configured list has no
effect on it. Tracked as `#733`.

### The configuration is not checked when the stream is created

A configuration missing something the stream cannot work without is accepted, and
the fault appears on the first connection attempt rather than at setup. Tracked
as `#732`.
