# OpenSSL

`Platform/OpenSsl/` wraps [OpenSSL](https://docs.openssl.org/) for TLS transport
and keyed at-rest crypto on hosted targets. It fills the
[Stream](../../api/structSolidSyslogStream.md) role with TLS and the
[SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role for at-rest
integrity and confidentiality.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogTlsStream`](../../api/SolidSyslogTlsStream_8h.md) | TLS stream over an injected byte transport |
| [`SolidSyslogOpenSslHmacSha256Policy`](../../api/SolidSyslogOpenSslHmacSha256Policy_8h.md) | at-rest HMAC-SHA256 |
| [`SolidSyslogOpenSslAesGcmPolicy`](../../api/SolidSyslogOpenSslAesGcmPolicy_8h.md) | at-rest AES-256-GCM |

## Requirements

OpenSSL 3.0 or later.

Credentials are **file paths**, read at `Open`: a PEM `CaBundlePath` for the
trust anchors, and for mutual TLS a PEM `ClientCertChainPath` and
`ClientKeyPath`. A `SolidSyslogSleepFunction` is required and has no fallback —
it bridges the handshake's `WANT_READ` / `WANT_WRITE` polls.

## Security behaviour and obligations

**What the adapter guarantees.** `SSL_VERIFY_PEER` is pinned on the context and
the protocol floor is set to TLS 1.2 with the return value checked, so `Open`
fails rather than proceeding if libssl refuses the floor. Every setup call in
the handshake path is return-checked, which is what stops a "handshake succeeded
without checking the name" bypass.

**The trust bundle is mandatory.** `CaBundlePath` must load. If it does not,
`Open` fails — there is no fallback to the system trust store.

**Peer identity is yours to assert.** `ServerName` drives both SNI and the
certificate check. Three cases, and only you know which you want:

| `ServerName` | Behaviour |
|---|---|
| a name | verified against the peer certificate's SAN or CN |
| `""` | chain verified, endpoint identity **not** checked — the opt-out for a closed network or private CA, and silent by design |
| `NULL` | as `""`, but reports a WARNING: the peer is unverified, which is MITM-class |

**Mutual TLS is opt-in and all-or-nothing.** Supply both
`ClientCertChainPath` and `ClientKeyPath` to present a client certificate, or
neither for server-authenticated TLS. Supplying one without the other is
rejected at `Open`, so this adapter cannot silently downgrade. The pairing is
also checked locally with `SSL_CTX_check_private_key` before any bytes reach the
wire. The [Mbed TLS adapter](../mbedtls/index.md) does neither — it proceeds
with server-authenticated TLS instead — so do not carry an assumption from one
to the other.

**The cipher policy is yours.** `CipherList` is passed through; NULL takes the
OpenSSL default. The library ships no baked-in list, because the right one
depends on the libssl build on your target and on your own security profile.
`"ECDHE+AESGCM:ECDHE+CHACHA20"` — TLS 1.2 AEAD with forward secrecy — is a
reasonable starting point to tune from, not a recommendation to adopt unread.

**Rotation is a file replacement plus a reconnect.** The `SSL_CTX` is rebuilt on
every `Open`, re-reading all three files, so replacing them on disk takes effect
on the next connect — either through natural churn (retry, outage recovery) or
by calling `SolidSyslogSender_Disconnect` to force one. No reload callback or
version-fingerprint API is needed.

**Key custody is entirely yours.** The library holds no keys of its own and
reads whatever material you point it at. Filesystem permissions on the key,
whether it is backed by an HSM, and how it is rotated are properties of your
deployment. The same applies to the at-rest policies: HMAC-SHA256 and
AES-256-GCM are keyed, and the key is yours to store and rotate.

**Revocation is not performed.** Neither CRL nor OCSP is checked by the adapter.
If your threat model needs revocation, it has to come from your own OpenSSL
configuration, and verifying that it is actually in force is yours to do.

## Source

[Every class in this pack](../../api/group__platform__openssl.md), generated
from the headers. The code itself is
[`Platform/OpenSsl/`](../../../Platform/OpenSsl/).

## Setup

Wiring it up, over any byte transport: [OpenSSL setup](setup.md).
