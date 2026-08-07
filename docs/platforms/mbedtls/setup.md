# Mbed TLS setup

Wiring `SolidSyslogMbedTlsStream` so a `SolidSyslogStreamSender` delivers
RFC 5425 syslog over TLS. [Mbed TLS](index.md) covers what the adapter
guarantees and what it leaves to you; the config fields are documented on the
struct itself. This page is the wiring, and the things that bite.

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
    .Sleep           = MyVTaskDelayWrapper,   /* required — no fallback */
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

**On a target with no platform entropy, define
`MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG`** and provide
`mbedtls_psa_external_get_random` wrapping the DRBG you just seeded. This keeps
PSA and the classic API on one entropy chain. If you have defined
`MBEDTLS_NO_PLATFORM_ENTROPY` — usual on embedded — this is effectively
mandatory, and omitting it makes `psa_crypto_init` return
`PSA_ERROR_INSUFFICIENT_ENTROPY`.

**Parse the CA chain, and the client credential if you are using mutual TLS**,
by whatever route suits the build: a filesystem, a baked-in array, a blob pulled
from a security element. PEM input must be NUL-terminated.

## FreeRTOS with newlib

Four sizing traps, all found during bring-up of the FreeRTOS test target.

**Route Mbed TLS allocations to the RTOS heap.** Mbed TLS calls libc `calloc`,
which on newlib typically reaches a small `_sbrk`-backed syscall heap. A single
`mbedtls_ssl_setup` wants roughly 10–16 KiB and fails with
`MBEDTLS_ERR_SSL_ALLOC_FAILED`. Set `MBEDTLS_PLATFORM_MEMORY` and call
`mbedtls_platform_set_calloc_free(pvPortMalloc, vPortFree)` before any
`mbedtls_*_init`.

**Shrink the record buffers from their 16 KiB default.** Set
`MBEDTLS_SSL_IN_CONTENT_LEN` to the largest record the collector will send — a
server certificate and chain is typically 2–4 KiB — and
`MBEDTLS_SSL_OUT_CONTENT_LEN` to your largest message. The defaults cost around
32 KiB of heap per TLS context.

**Budget the handshake state.** `mbedtls_ssl_setup` allocates roughly
`IN + OUT + 3 KiB`. Size `configTOTAL_HEAP_SIZE` across every concurrent TLS
context, not just one.

**Give the mutex role a real implementation** if `Log` and `Service` run on
different tasks. The library will not detect that you have not.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the stream fell
back to the Null object, and nothing will be delivered.
