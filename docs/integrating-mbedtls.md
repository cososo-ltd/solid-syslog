# Integrating SolidSyslog with mbedTLS

`SolidSyslogMbedTlsStream` lets you deliver RFC 5425 (syslog over TLS)
records from SolidSyslog through Mbed TLS instead of OpenSSL. It is the
recommended adapter on embedded / FreeRTOS / bare-metal targets where
OpenSSL is too large or impractical. Hosted Linux / Windows deployments
should use `SolidSyslogTlsStream` (OpenSSL); both adapters expose the
same `SolidSyslogStream` vtable, so the rest of the wiring
(`SolidSyslogStreamSender`, your buffer, your store) is identical.

This document covers what you, the integrator, plug in. It does not
re-teach mbedTLS. For that, see the
[upstream Mbed TLS documentation](https://mbed-tls.readthedocs.io/).

---

## The shape

```text
SolidSyslog_Log ─▶ Buffer ─▶ Sender ─▶ SolidSyslogStreamSender
                                              │
                                              ▼
                                  SolidSyslogMbedTlsStream  ◀── you build CA/cert/key/DRBG handles
                                              │
                                              ▼
                                       SolidSyslogStream    ◀── you pick / write the TCP backend
                                              │
                                              ▼
                                          (your TCP/IP stack)
```

You supply two things directly: the byte-transport `SolidSyslogStream` and
the per-context mbedTLS handles passed through
`SolidSyslogMbedTlsStreamConfig`.

---

## What you need to provide

| Item | Owner | Notes |
|---|---|---|
| `Transport` | You | A `SolidSyslogStream*` carrying TCP. The library ships `SolidSyslogPosixTcpStream` (POSIX), `SolidSyslogWinsockTcpStream` (Windows), and `SolidSyslogPlusTcpTcpStream` (FreeRTOS-Plus-TCP). If your TCP/IP stack is different (LwIP, NicheStack, vendor BSP), write your own `SolidSyslogStream`; see [`Platform/Posix/Source/SolidSyslogPosixTcpStream.c`](../Platform/Posix/Source/SolidSyslogPosixTcpStream.c) as a reference. |
| `Sleep` | You | A `SolidSyslogSleepFunction`. Drives the bounded handshake retry between `WANT_READ` / `WANT_WRITE` polls. On FreeRTOS use a `vTaskDelay`-backed wrapper; on POSIX `SolidSyslogPosixSleep` is the natural fit. Required. |
| `GetHandshakeTimeoutMs` / `HandshakeTimeoutContext` | You (optional) | Per-instance accessor pair for the bounded handshake budget. `NULL` falls back to the `SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS` compile-time tunable (default 5000 ms). Install when you need to runtime-tune the handshake deadline: slow peers on a constrained link, or per-tenant policy from your existing configuration store. The accessor is called on every `Open`. |
| `Rng` | You | `mbedtls_ctr_drbg_context*` you seeded yourself. The adapter calls `mbedtls_ctr_drbg_random` against it. Required. |
| `CaChain` | You | `mbedtls_x509_crt*` you parsed yourself (from filesystem, baked-in PEM, HSM, whatever fits your build). Required. |
| `ServerName` | You | SNI + peer-identity check string. A non-empty name is verified against the server certificate (SAN/CN), rejecting any other CA-issued cert. `NULL` connects but the peer is unverified (any cert chaining to a trusted CA is accepted, MITM-class), so the library emits a `WARNING` (`BAD_CONFIG` / `MBEDTLSSTREAM_ERROR_SERVER_NAME_NOT_SET`). Note that having no DNS does not force this: an IP-pinned target can still set `ServerName` to the name (or IP SAN) on its certificate and get full identity verification. Use `""` only as a deliberate opt-out for closed networks / private CAs where there is genuinely no name to verify; it connects chain-only with no diagnostic. |
| `ClientCertChain` / `ClientKey` | You | `mbedtls_x509_crt*` + `mbedtls_pk_context*` for mTLS. Both `NULL` = server-auth-only TLS. Both non-`NULL` = mTLS. Supplying only one is treated as "no client cert"; the adapter never half-configures. |

The full struct shape lives in
[`Platform/MbedTls/Interface/SolidSyslogMbedTlsStream.h`](../Platform/MbedTls/Interface/SolidSyslogMbedTlsStream.h).

The adapter pins the minimum protocol version to TLS 1.2 on its own
`ssl_config` rather than inheriting `MBEDTLS_SSL_PRESET_DEFAULT`, which, on a
permissive mbedTLS build (2.x, or 3.x with `MBEDTLS_SSL_PROTO_TLS1_0/1_1`
enabled), can otherwise negotiate down to TLS 1.0/1.1. This matches the OpenSSL
reference adapter's explicit floor, so the two are equivalent in downgrade
resistance; it is not something you configure.

---

## Scenario A: you already have Mbed TLS in your image

If your firmware already wires Mbed TLS for another subsystem (a cloud
client, an OTA updater, a vendor security framework), you keep that wiring
intact. The adapter consumes the handles you've already built; it never
calls `mbedtls_platform_setup` / `_teardown`, never installs
threading-alt hooks, never resets the global RNG, never replaces your
debug callback. See the [coexistence contract](#coexistence-contract)
below for the auditable list.

Concretely, on top of your existing setup:

1. Pick a `SolidSyslogStream` for the byte transport. Use one of the
   shipped adapters that matches your TCP/IP stack
   (`SolidSyslogPlusTcpTcpStream`, `SolidSyslogPosixTcpStream`,
   `SolidSyslogWinsockTcpStream`) or write your own backing the same
   `SolidSyslogStream` vtable. If you wrote your own, the existing
   shipped adapters are the worked examples.
2. Fill in `SolidSyslogMbedTlsStreamConfig` with the handles you
   already have:

   ```c
   struct SolidSyslogMbedTlsStreamConfig cfg = {
       .Transport               = myTcpStream,        /* from step 1 */
       .Sleep                   = MyVTaskDelayWrapper, /* or PosixSleep / similar */
       .GetHandshakeTimeoutMs   = NULL,  /* defaults to SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS (5000 ms) */
       .HandshakeTimeoutContext = NULL,
       .Rng                     = &myAlreadySeededDrbg,
       .CaChain                 = &myAlreadyParsedCaChain,
       .ServerName              = "syslog.example.com",
       .ClientCertChain         = &myClientCert,  /* NULL for server-auth-only */
       .ClientKey               = &myClientKey,   /* paired with ClientCertChain */
   };
   struct SolidSyslogStream* tlsStream = SolidSyslogMbedTlsStream_Create(&cfg);
   ```

3. Wire `tlsStream` into a `SolidSyslogStreamSender` as the `Stream`
   field, the same way you'd wire a plain TCP stream. RFC 6587
   octet-counting framing is applied by `StreamSender` on top of the
   adapter.

That's the whole integration on the SolidSyslog side. There are no
process-wide hooks to install and nothing to teardown beyond the matching
`SolidSyslogMbedTlsStream_Destroy` when you tear the sender down.

---

## Scenario B: you do not have Mbed TLS yet

If you're bringing Mbed TLS in fresh for SolidSyslog, do that work first
following the upstream
[Mbed TLS porting guide](https://mbed-tls.readthedocs.io/en/latest/kb/how-to/how-do-i-port-mbed-tls-to-a-new-environment-OS/).
Once Mbed TLS itself is building on your target, you need the following
specifically for this adapter:

- A seeded `mbedtls_ctr_drbg_context`. `mbedtls_entropy_init` +
  `mbedtls_entropy_add_source` for at least one source registered as
  `MBEDTLS_ENTROPY_SOURCE_STRONG` (without a STRONG-tagged source,
  `mbedtls_entropy_func` never satisfies its internal threshold and
  every `mbedtls_ctr_drbg_seed` call returns
  `MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED`, silent on the wire,
  loud in your tests), then `mbedtls_ctr_drbg_init` +
  `mbedtls_ctr_drbg_seed`. Production-quality entropy is a hardware
  question: TRNG, vendor HSM, or a board-specific source.
- `psa_crypto_init()` called *after* the DRBG is seeded. Mbed TLS
  3.6's TLS 1.3 code path routes through PSA. If PSA isn't initialised,
  the first handshake state transition returns
  `MBEDTLS_ERR_ERROR_GENERIC_ERROR` (-0x0001) before any TLS bytes leave
  the socket. If your target has no platform entropy source (a common
  embedded case), `#define MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG` in your
  mbedTLS config and provide
  `mbedtls_psa_external_get_random` that wraps the DRBG you just seeded;
  this keeps PSA and the classic mbedTLS API on the same entropy chain.
- A parsed CA chain. `mbedtls_x509_crt_init` +
  `mbedtls_x509_crt_parse` against whatever delivery mechanism fits your
  build (filesystem on POSIX, baked-in array via `xxd -i` on bare-metal,
  HSM-pulled blob, etc.). PEM input must be NUL-terminated.
- (mTLS only) a parsed client cert chain and private key. Same
  pattern as the CA chain plus `mbedtls_pk_init` /
  `mbedtls_pk_parse_key`.
- A byte-transport `SolidSyslogStream` matching your TCP/IP stack,
  exactly as in [Scenario A](#scenario-a-you-already-have-mbed-tls-in-your-image).

A worked end-to-end example for all of the above lives at
[`Bdd/Targets/Common/BddTargetTlsSender_MbedTls_PlusTcpTcp.c`](../Bdd/Targets/Common/BddTargetTlsSender_MbedTls_PlusTcpTcp.c)
(FreeRTOS-Plus-TCP on QEMU mps2-an385). The matching Mbed TLS config
overrides live at
[`Bdd/Targets/FreeRtos/mbedtls_user_config.h`](../Bdd/Targets/FreeRtos/mbedtls_user_config.h).

---

## Coexistence contract

`Platform/MbedTls/Source/` is auditably free of process-global Mbed TLS
calls. The adapter never:

- calls `mbedtls_platform_setup` / `_teardown`
- calls `mbedtls_threading_set_alt`
- calls `psa_crypto_init` (you do)
- calls `mbedtls_platform_set_calloc_free` (you do, if you need it)
- calls `mbedtls_debug_set_threshold` / `mbedtls_ssl_conf_dbg`
- frees any handle you passed in via the config struct

Everything in that list is global state your existing integration may
already own. Auditors verify the contract by grepping
`Platform/MbedTls/Source/`; any future change that introduces a global
call must be flagged in review.

---

## FreeRTOS-specific gotchas

These bit us during the BDD-target bring-up. If you're on FreeRTOS with
newlib, treat them as integrator-side checklist items:

- Route mbedTLS allocations to the RTOS heap. Mbed TLS calls libc
  `calloc`, which on newlib targets typically hits a tiny `_sbrk`-backed
  syscall heap (4 KiB in the SolidSyslog BDD reference at
  [`Bdd/Targets/FreeRtos/Common/Syscalls.c`](../Bdd/Targets/FreeRtos/Common/Syscalls.c)).
  A single `mbedtls_ssl_setup` wants ~10–16 KiB and will fail with
  `MBEDTLS_ERR_SSL_ALLOC_FAILED` (-0x7F00). Set
  `MBEDTLS_PLATFORM_MEMORY` in your config and call
  `mbedtls_platform_set_calloc_free(yourCalloc, yourFree)` (pvPortMalloc
  / vPortFree) before any `mbedtls_*_init`.
- Shrink the TLS record buffers from the 16 KiB default. Set
  `MBEDTLS_SSL_IN_CONTENT_LEN` to the largest TLS record your peer will
  send (server cert + chain is typically 2–4 KiB), and
  `MBEDTLS_SSL_OUT_CONTENT_LEN` to your largest application message.
  The defaults cost ~32 KiB of FreeRTOS heap per TLS context.
- `mbedtls_ssl_setup` allocates roughly `IN + OUT + ~3 KiB` of
  handshake state. Size your FreeRTOS heap
  (`configTOTAL_HEAP_SIZE`) accordingly across all concurrent TLS
  contexts.
- `MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG` + the external RNG hook are
  effectively mandatory if you've defined
  `MBEDTLS_NO_PLATFORM_ENTROPY` (which you typically have on
  embedded). Without it, `psa_crypto_init` returns
  `PSA_ERROR_INSUFFICIENT_ENTROPY` (-148).

The BDD target's
[mbedtls_user_config.h](../Bdd/Targets/FreeRtos/mbedtls_user_config.h)
shows the minimal config that satisfies the above for QEMU mps2-an385.

---

## Reference integrations

| Target | Adapter source | Mbed TLS config | Notes |
|---|---|---|---|
| FreeRTOS QEMU mps2-an385 + FreeRTOS-Plus-TCP | [BddTargetTlsSender_MbedTls_PlusTcpTcp.c](../Bdd/Targets/Common/BddTargetTlsSender_MbedTls_PlusTcpTcp.c) | [mbedtls_user_config.h](../Bdd/Targets/FreeRtos/mbedtls_user_config.h) | Demo-quality entropy and baked-in PEMs; loudly tagged not-for-production. |
| Linux host (host-TDD parity with the embedded path) | [Tests/MbedTlsIntegration/](../Tests/MbedTlsIntegration/) | — | In-process TLS server drives a real handshake against the wrapper. |
| POSIX (OpenSSL reference, for comparison) | [BddTargetTlsSender_OpenSsl_PosixTcp.c](../Bdd/Targets/Common/BddTargetTlsSender_OpenSsl_PosixTcp.c) | — | Same composition shape using `SolidSyslogTlsStream` for the TLS layer. |

---

## What this adapter does not own

- PEM-to-handle conversion: you parse, in whatever way fits your
  build.
- Certificate rotation: re-parse and rebuild the adapter, or destroy
  / re-create the `SolidSyslogStreamSender` so the next Connect picks up
  the new chain.
- HSM / TRNG integration: your entropy source feeds CTR_DRBG; the
  adapter consumes the seeded DRBG.
- Per-connection TLS configuration: one adapter instance, one
  `ssl_config`. If you need per-peer cipher / version pinning, build
  multiple adapters.
