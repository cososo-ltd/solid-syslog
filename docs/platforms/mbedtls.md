# Mbed TLS

`Platform/MbedTls/` wraps Mbed TLS for TLS transport and keyed at-rest crypto on
embedded targets ([Mbed TLS documentation](https://mbed-tls.readthedocs.io/)).

Fills the [Stream](../api/structSolidSyslogStream.md) role with TLS, and the
[SecurityPolicy](../api/structSolidSyslogSecurityPolicy.md) role for at-rest
integrity.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogMbedTlsStream`](../api/SolidSyslogMbedTlsStream_8h.md) | TLS stream over an injected byte transport |
| [`SolidSyslogMbedTlsHmacSha256Policy`](../api/SolidSyslogMbedTlsHmacSha256Policy_8h.md) | at-rest HMAC-SHA256 |
| [`SolidSyslogMbedTlsAesGcmPolicy`](../api/SolidSyslogMbedTlsAesGcmPolicy_8h.md) | at-rest AES-256-GCM |

## Requirements

Your `mbedtls_config.h`. You pass caller-built handles (RNG, cert chain, key), not
file paths. `Platform/MbedTls/Source/` never calls process-global Mbed TLS APIs
(`mbedtls_platform_setup`, `psa_crypto_init`, …) — you own those, so SolidSyslog
coexists with your other Mbed TLS use.

Full setup is [Integrating Mbed TLS](../integrating-mbedtls.md).
