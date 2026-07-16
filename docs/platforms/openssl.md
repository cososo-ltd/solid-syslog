# OpenSSL

`Platform/OpenSsl/` wraps OpenSSL for TLS transport and keyed at-rest crypto
([OpenSSL documentation](https://docs.openssl.org/)).

Fills the [Stream](../api/structSolidSyslogStream.md) role with TLS, and the
[SecurityPolicy](../api/structSolidSyslogSecurityPolicy.md) role for at-rest
integrity.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogTlsStream`](../api/SolidSyslogTlsStream_8h.md) | TLS stream — server-cert + hostname verification, cipher pinning, optional mutual TLS |
| [`SolidSyslogOpenSslHmacSha256Policy`](../api/SolidSyslogOpenSslHmacSha256Policy_8h.md) | at-rest HMAC-SHA256 |
| [`SolidSyslogOpenSslAesGcmPolicy`](../api/SolidSyslogOpenSslAesGcmPolicy_8h.md) | at-rest AES-256-GCM |

## Requirements

OpenSSL 3.0 or later.
