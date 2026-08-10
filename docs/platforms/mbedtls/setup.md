# Mbed TLS setup

Wiring `SolidSyslogMbedTlsStream` so a `SolidSyslogStreamSender` delivers
RFC 5425 syslog over TLS. The [TLS obligations](../../tls.md) page covers what
any TLS stream must do. The [Mbed TLS](index.md) page covers what this adapter
needs and where it falls short of that. The config fields are documented on the
struct itself, and this page is the wiring — and the things that bite.

## The layering

TLS is a Stream wrapped around another Stream. The TLS adapter carries the
records; the transport underneath carries the bytes.

```text
SolidSyslog_Log ─▶ Buffer ─▶ SolidSyslogStreamSender
                                     │
                                     ▼
                         SolidSyslogMbedTlsStream   ◀── your CA / cert / key / DRBG handles
                                     │
                                     ▼
                            a byte-transport Stream  ◀── your TCP/IP stack
```

You supply two things: the byte transport, and the Mbed TLS handles. Everything
above the TLS stream is unchanged from a plaintext wiring — `StreamSender`
applies RFC 6587 octet-counting framing on top either way.

## Wiring it

```c
struct SolidSyslogMbedTlsStreamConfig cfg = {
    .Transport       = myTcpStream,
    .Sleep           = MySleep,               /* required — no fallback */
    .Rng             = &mySeededDrbg,
    .CaChain         = &myParsedCaChain,
    .ServerName      = "syslog.example.com",
    .ClientCertChain = &myClientCert,         /* both, or neither */
    .ClientKey       = &myClientKey,
};
struct SolidSyslogStream* tls = SolidSyslogMbedTlsStream_Create(&cfg);
```

Wire `tls` into a `SolidSyslogStreamSender` as its `Stream`, exactly as you
would a plain TCP stream, and call `SolidSyslogMbedTlsStream_Destroy` when the
sender is torn down. There is nothing process-wide to install.

If your firmware already uses Mbed TLS for something else — a cloud client, an
OTA updater, a vendor framework — that is the whole integration: the adapter
consumes handles you have already built and touches no global state.

## Bringing Mbed TLS up, if it is new to the target

Port Mbed TLS itself first, following the
[upstream porting guide](https://mbed-tls.readthedocs.io/en/latest/kb/how-to/how-do-i-port-mbed-tls-to-a-new-environment-OS/).
Four things then matter specifically for this adapter, and three of them fail in
ways that are hard to read.

**Seed the DRBG from a source registered as `MBEDTLS_ENTROPY_SOURCE_STRONG`.**
Without a strong-tagged source, `mbedtls_entropy_func` never reaches its
internal threshold and every `mbedtls_ctr_drbg_seed` returns
`MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED`. Production entropy is a hardware
question — a true random number generator, a vendor security element, or a
board-specific source.

**Call `psa_crypto_init()` after the DRBG is seeded, not before.** Mbed TLS
3.6 routes TLS 1.3 through PSA, and if PSA is uninitialised the first handshake
state transition returns `MBEDTLS_ERR_ERROR_GENERIC_ERROR` before any byte
reaches the socket.

**On a target with no platform entropy, give PSA a strong source.** With
`MBEDTLS_NO_PLATFORM_ENTROPY` defined — usual on embedded — `mbedtls_entropy_init`
registers no source of its own, and `psa_crypto_init` then fails with
`PSA_ERROR_INSUFFICIENT_ENTROPY`. Two routes out, and either is enough:

- Define `MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG` and provide
  `mbedtls_psa_external_get_random` wrapping the DRBG you just seeded. PSA
  bypasses the entropy subsystem entirely, so PSA and the classic API run off
  one chain.
- Or define `MBEDTLS_ENTROPY_HARDWARE_ALT` and provide `mbedtls_hardware_poll`.
  `mbedtls_entropy_init` then registers it as a strong source, and PSA seeds its
  own DRBG through the standard path.

The first keeps one chain and is the simpler thing to reason about; the second
suits a target whose randomness already arrives through a hardware poll.

**Parse the CA chain, and the client credential if you are using mutual TLS**,
by whatever route suits the build: a filesystem, a baked-in array, a blob pulled
from a security element. PEM input must be NUL-terminated.

## Memory

The adapter allocates nothing itself. Everything a TLS session costs is Mbed
TLS's own allocation, governed by your `mbedtls_config.h` — the record buffer
sizes dominate it, and their defaults are sized for a general-purpose host
rather than a constrained target. Budget for every TLS session you intend to
run concurrently, not one, and take the sizing guidance from the
[upstream documentation](https://mbed-tls.readthedocs.io/) rather than from
here.

Where Mbed TLS takes its memory from is also your configuration. On a target
whose libc heap is not the one you intend it to use, `mbedtls_ssl_setup` is
where that shows up.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the stream fell
back to the Null object, and nothing will be delivered.
