# Mbed TLS

`Platform/MbedTls/` wraps [Mbed TLS](https://mbed-tls.readthedocs.io/) for TLS
transport and keyed at-rest crypto on embedded targets, where OpenSSL is too
heavy. It fills the [Stream](../../api/structSolidSyslogStream.md) role with TLS
and the [SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role for
at-rest integrity and confidentiality.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogMbedTlsStream`](../../api/SolidSyslogMbedTlsStream_8h.md) | TLS stream over an injected byte transport |
| [`SolidSyslogMbedTlsHmacSha256Policy`](../../api/SolidSyslogMbedTlsHmacSha256Policy_8h.md) | at-rest HMAC-SHA256 |
| [`SolidSyslogMbedTlsAesGcmPolicy`](../../api/SolidSyslogMbedTlsAesGcmPolicy_8h.md) | at-rest AES-256-GCM |

## Requirements

Your own `mbedtls_config.h` — the adapter's sources compile in your target
against your configuration, so the features you enable are the features it gets.

You pass **caller-built, caller-owned handles**, not file paths: a seeded
`mbedtls_ctr_drbg_context` for the handshake, an `mbedtls_x509_crt` trust chain,
and for mutual TLS an `mbedtls_x509_crt` / `mbedtls_pk_context` pair. Nothing in
the adapter touches a filesystem, which is what lets it work on targets built
without `MBEDTLS_FS_IO`. Every handle must outlive the stream.

A `SolidSyslogSleepFunction` is required and has no fallback — it bridges the
handshake's `WANT_READ` / `WANT_WRITE` polls.

## Security behaviour and obligations

**What the adapter guarantees.** Peer verification is pinned to
`MBEDTLS_SSL_VERIFY_REQUIRED` and the protocol floor to TLS 1.2, both set
explicitly on the adapter's own `ssl_config`. The floor is pinned rather than
inherited from `MBEDTLS_SSL_PRESET_DEFAULT`, which can otherwise negotiate down
to TLS 1.0 or 1.1 on a permissive build. TLS 1.3 still negotiates when both
peers offer it.

**Peer identity is yours to assert.** `ServerName` drives both SNI and the
peer-certificate check. Three cases, and only you know which you want:

| `ServerName` | Behaviour |
|---|---|
| a name | verified against the peer certificate's SAN or CN |
| `""` | chain verified, endpoint identity **not** checked — the opt-out for a closed network or private CA, and silent by design |
| `NULL` | as `""`, but reports a WARNING: the peer is unverified, which is MITM-class |

**Mutual TLS is opt-in, and a half-supplied credential does not fail.** Both
`ClientCertChain` and `ClientKey` set means the client certificate is presented;
either one NULL means no client certificate is configured and `Open` proceeds
with server-authenticated TLS. It does not fail. The
[OpenSSL adapter](../openssl/index.md) rejects the same partial configuration at
`Open`, so the two do not behave alike here. If a half-supplied credential must
be an error on this adapter, check before calling `Open`.

The adapter also performs no local check that `ClientKey` matches
`ClientCertChain` — the OpenSSL adapter does, via `SSL_CTX_check_private_key`. A
mismatched pair here surfaces as a handshake rejection from the peer rather than
as a setup error on the device.

**Rotation is a restart, not a reload.** Because the adapter consumes pre-built
handles rather than paths, refreshing credentials means parsing a new
`mbedtls_x509_crt` and recreating the stream — or the parent
`SolidSyslogStreamSender`, so the next connect picks it up. There is no reload
callback.

**Key custody is entirely yours.** The library holds no keys of its own and
reads whatever material you hand it. Where the private key lives, how it is
protected at rest, and whether it is backed by a secure element are properties
of your platform, not of this adapter. The same applies to the at-rest policies:
HMAC-SHA256 and AES-256-GCM are keyed, and the key is yours to store and rotate.

**Revocation is not performed.** Neither CRL nor OCSP is checked. If your threat
model needs revocation, it has to come from your own configuration of Mbed TLS.

**Coexistence is an auditable contract.** `Platform/MbedTls/Source/` never calls
a process-global Mbed TLS API — no `mbedtls_platform_setup` or `_teardown`, no
threading-alt hooks, no `psa_crypto_init`, no global RNG reset, no replacement of
your debug callback. TLS policy is set per-`ssl_config` so it cannot leak into
the ones you build elsewhere. A device that already wires Mbed TLS for OTA or a
vendor cloud SDK keeps that wiring intact. The claim is grep-auditable against
the directory.

## Source

[Every class in this pack](../../api/group__platform__mbedtls.md), generated
from the headers. The code itself is
[`Platform/MbedTls/`](../../../Platform/MbedTls/).

## Setup

Wiring it up, handle by handle: [Mbed TLS setup](setup.md).
