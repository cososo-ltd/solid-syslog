# Mbed TLS

`Platform/MbedTls/` wraps [Mbed TLS](https://mbed-tls.readthedocs.io/) for TLS
transport and keyed at-rest cryptography on embedded targets. It fills the
[Stream](../../api/structSolidSyslogStream.md) role with TLS and the
[SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role for at-rest
integrity and confidentiality.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogMbedTlsStream`](../../api/SolidSyslogMbedTlsStream_8h.md) | TLS stream over an injected byte transport |
| [`SolidSyslogMbedTlsHmacSha256Policy`](../../api/SolidSyslogMbedTlsHmacSha256Policy_8h.md) | at-rest HMAC-SHA256 |
| [`SolidSyslogMbedTlsAesGcmPolicy`](../../api/SolidSyslogMbedTlsAesGcmPolicy_8h.md) | at-rest AES-256-GCM |

## Requirements

The pack compiles against your own `mbedtls_config.h`, in your target, so the
features you enable are the features it gets.

Credentials are passed as caller-built, caller-owned handles rather than file
paths: a seeded `mbedtls_ctr_drbg_context` for the handshake, an
`mbedtls_x509_crt` trust chain, and for mutual TLS an `mbedtls_x509_crt` and
`mbedtls_pk_context` pair. No part of the adapter opens a file, which is what
allows it to run on targets built without `MBEDTLS_FS_IO`. Each handle must
remain valid for the lifetime of the stream.

A `SolidSyslogSleepFunction` is required and has no default.

## Security behaviour and obligations

The per-field detail is in
[`SolidSyslogMbedTlsStream.h`](../../api/SolidSyslogMbedTlsStream_8h.md),
alongside the fields themselves. What follows is the behaviour of the adapter as
a whole, and the work it leaves to you.

### Transport security is fixed by the adapter

Peer certificate verification is pinned to `MBEDTLS_SSL_VERIFY_REQUIRED` and the
protocol floor to TLS 1.2, both set on the adapter's own `ssl_config`. The floor
is set explicitly rather than inherited from `MBEDTLS_SSL_PRESET_DEFAULT`, which
on a permissive build can negotiate down to TLS 1.0 or 1.1. TLS 1.3 is
negotiated when both peers support it.

### Peer identity is yours to declare

The `ServerName` field supplies both the Server Name Indication sent in the
handshake and the identity checked against the peer certificate. It has a
distinct meaning when set, when empty, and when NULL — including one value that
disables endpoint verification without reporting anything — and the three are
documented on the field. Choosing between them is a deployment decision the
adapter cannot make.

### Mutual TLS is optional and is not validated locally

A client certificate is presented only when both `ClientCertChain` and
`ClientKey` are supplied. If either is absent, no client certificate is
configured and `Open` proceeds with server-authenticated TLS rather than
failing. Where a half-supplied credential must be treated as an error, check for
it before calling `Open`.

The adapter performs no local check that the key matches the certificate, and
does not report a failure to install the pair. A mismatch is therefore seen as a
handshake rejection from the collector rather than as a setup error on the
device.

### Rotation requires a restart of the stream

Because the adapter consumes pre-built handles, refreshing credentials means
parsing new ones and recreating the stream, or the parent
`SolidSyslogStreamSender` so that the next connection uses them. There is no
reload callback.

### Key custody is outside the library

The library holds no keys of its own and uses whatever material is passed to it.
Where a private key is stored, how it is protected at rest, and whether it is
held in a secure element are properties of your platform. The same applies to
the at-rest policies: HMAC-SHA256 and AES-256-GCM are keyed, and storing and
rotating that key is yours.

### Revocation is not checked

The adapter performs no revocation checking, by Certificate Revocation List or
by the Online Certificate Status Protocol. Where a deployment requires it, it
must come from your own configuration of Mbed TLS, and confirming that it is in
force is part of your assessment rather than something the adapter reports.

### Coexistence is an auditable contract

`Platform/MbedTls/Source/` calls no process-global Mbed TLS API. It does not
call `mbedtls_platform_setup` or `mbedtls_platform_teardown`, install
threading-alt hooks, call `psa_crypto_init`, reset the global random number
generator, or replace a debug callback. TLS policy is applied per `ssl_config`,
so it cannot affect the ones you build elsewhere. A device that already uses
Mbed TLS for firmware update or a vendor cloud SDK keeps that configuration
intact, and the claim can be checked against the directory.

## API reference

[Every class in this pack](../../api/group__platform__mbedtls.md), generated
from the headers.

## Setup

Wiring it up, handle by handle: [Mbed TLS setup](setup.md).
