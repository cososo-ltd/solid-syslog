# Integrating mbedTLS

`SolidSyslogMbedTlsStream` is a `SolidSyslogStream` implementation that
wraps an injected byte-stream (typically `SolidSyslogFreeRtosTcpStream` or
`SolidSyslogPosixTcpStream`) with TLS via Mbed TLS 3.x. It is a reference
adapter — small, dependency-injected, and intended to be auditable line by
line. `SolidSyslogTlsStream` (OpenSSL-backed) remains the host-side
reference; this guide covers the mbedTLS adapter for embedded / FreeRTOS /
resource-constrained integrations where OpenSSL is impractical.

## When to use mbedTLS

| Scenario | Recommended adapter |
|---|---|
| Linux server / appliance / containerised | `SolidSyslogTlsStream` (OpenSSL) |
| Windows server / desktop | `SolidSyslogTlsStream` (OpenSSL) |
| FreeRTOS / Zephyr / bare-metal Cortex-M with TCP/IP | `SolidSyslogMbedTlsStream` |
| RTOS with no full-fat TLS library available | `SolidSyslogMbedTlsStream` |

Both adapters expose the same `SolidSyslogStream` vtable, so callers swap
implementations at link time — `SolidSyslogStreamSender` doesn't know
which TLS library is underneath.

## Coexistence contract

`Platform/MbedTls/Source/` MUST NOT call any process-global mbedTLS APIs
(global RNG installers, global PSA initialisers, global mbedTLS allocator
hooks). The contract is auditable by grep: any future change that
introduces such a call must be flagged in review. See
[[project-mbedtls-coexistence-contract]] for the rationale.

The integrator owns process-global setup. The adapter only consumes
caller-built handles via the config struct.

This matters because mbedTLS allows applications to share a single global
crypto context across multiple library users. If the SolidSyslog adapter
installed its own global state — a process-wide allocator, RNG, or PSA
context — it would silently overwrite the integrator's existing wiring
or be silently overwritten by it.

## Configuration handles

`SolidSyslogMbedTlsStreamConfig` (in
[Platform/MbedTls/Interface/SolidSyslogMbedTlsStream.h](../Platform/MbedTls/Interface/SolidSyslogMbedTlsStream.h))
takes pre-built handles, never file paths or PEM blobs:

| Field | Owner | Notes |
|---|---|---|
| `Transport` | Caller | A `SolidSyslogStream*` carrying the byte transport. The adapter calls `Open`/`Send`/`Read`/`Close` on it — same vtable that `SolidSyslogStreamSender` would use directly for plain TCP. |
| `Sleep` | Caller | A `SolidSyslogSleepFunction` driving the bounded handshake retry between `WANT_READ` / `WANT_WRITE` polls. Required. On FreeRTOS use a `vTaskDelay`-backed wrapper; on POSIX `SolidSyslogPosixSleep` is the natural fit. |
| `Rng` | Caller | `mbedtls_ctr_drbg_context*` — seeded by the integrator before the first handshake. The adapter calls `mbedtls_ctr_drbg_random` against this handle. |
| `CaChain` | Caller | `mbedtls_x509_crt*` parsed by the integrator from whatever source is appropriate (filesystem on POSIX, baked-in `xxd -i` arrays on FreeRTOS, HSM-backed trust store on a secure element). |
| `ServerName` | Caller | SNI string and certificate-name check target. `NULL` skips the name check — only appropriate for closed networks where IP-pinning replaces hostname identity. |
| `ClientCertChain` / `ClientKey` | Caller | `mbedtls_x509_crt*` + `mbedtls_pk_context*` for mTLS. Both NULL = server-auth TLS only. Both non-NULL = mTLS. Supplying only one is treated as "no client cert" — the adapter never half-configures. |

The "caller owns handles" pattern keeps the adapter framework-agnostic: a
deployment that already builds its own X.509 / DRBG handles for other
purposes (key rotation policy, HSM integration, TRNG wiring) reuses those
handles unchanged. See [[project-mbedtls-di-handles]] for the design note.

## Process-wide setup the integrator must do

The order of operations matters — getting it wrong surfaces as misleading
mbedTLS errors (the most common ones are documented at the end of this
guide). Recommended sequence:

1. **Install the platform allocator** (if not using the default libc one).
   On FreeRTOS this routes mbedTLS allocations through `pvPortMalloc` /
   `vPortFree` so they land in the RTOS heap rather than newlib's tiny
   syscall heap. Requires `MBEDTLS_PLATFORM_MEMORY` in the integrator's
   mbedTLS config:
   ```c
   mbedtls_platform_set_calloc_free(YourCalloc, YourFree);
   ```
2. **Initialise entropy and seed the DRBG.** The adapter does not seed the
   DRBG itself; that responsibility stays with the integrator, who knows
   what hardware sources are available:
   ```c
   mbedtls_entropy_init(&entropy);
   mbedtls_entropy_add_source(&entropy, YourEntropySource, NULL, ...);
   mbedtls_ctr_drbg_init(&drbg);
   mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &entropy, ...);
   ```
3. **Initialise PSA crypto** (mandatory for mbedTLS 3.6 with TLS 1.3
   enabled — even when negotiating an older version, the handshake state
   machine touches PSA). Must come *after* the DRBG is seeded if you are
   using `MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG`:
   ```c
   psa_crypto_init();
   ```
4. **Parse the certificates and keys** the adapter will consume:
   ```c
   mbedtls_x509_crt_init(&caChain);
   mbedtls_x509_crt_parse(&caChain, caPem, caPemLen + 1);  /* +1 for NUL */
   /* …same pattern for client cert and key… */
   ```
5. **Build the adapter** via `SolidSyslogMbedTlsStream_Create(&config)`
   passing the handles initialised above.

The adapter's `Open` / `Close` cycle is idempotent and re-entrant: the
underlying mbedTLS contexts are zeroed on construction and after every
`Close`, so reconnect after an outage is automatic and leak-free.

## FreeRTOS-specific considerations

### Heap budget

mbedTLS `mbedtls_ssl_setup` allocates per-context buffers totalling roughly
`MBEDTLS_SSL_IN_CONTENT_LEN + MBEDTLS_SSL_OUT_CONTENT_LEN + ~3 KiB` of
handshake state. With mbedTLS defaults (16 KiB each) that's >35 KiB per
context — likely larger than your RTOS heap if you haven't sized for it.

Three knobs:

- Size the **FreeRTOS heap** (`configTOTAL_HEAP_SIZE`) to cover the worst
  case: per-SSL-context working set × (number of concurrent TLS connections),
  plus everything else the application allocates.
- Shrink **`MBEDTLS_SSL_IN_CONTENT_LEN`** to the largest TLS record the peer
  will send. Server certificates with intermediates are typically 2–4 KiB;
  a 4096-byte IN buffer is a reasonable starting point for syslog deployments.
- Shrink **`MBEDTLS_SSL_OUT_CONTENT_LEN`** to the largest message you will
  send. The library's default `SOLIDSYSLOG_MAX_MESSAGE_SIZE` fits inside
  2048 comfortably.

The BDD target's [mbedtls_user_config.h](../Bdd/Targets/FreeRtos/mbedtls_user_config.h)
shows the full minimal config — `IN=4096`, `OUT=2048`, with rationale
comments — for the QEMU mps2-an385 reference image.

### newlib's syscall heap

If the FreeRTOS target uses newlib (the SolidSyslog FreeRTOS BDD target
does), libc `calloc` is backed by `_sbrk`, which in turn is typically backed
by a small static buffer (4 KiB in the SolidSyslog BDD reference at
[Syscalls.c](../Bdd/Targets/FreeRtos/Common/Syscalls.c)). That buffer is
intended for newlib's small scratch allocations (printf, etc.), not TLS
contexts.

Without `mbedtls_platform_set_calloc_free`, mbedTLS will silently land
every `calloc` in that 4 KiB buffer and `mbedtls_ssl_setup` will fail with
`MBEDTLS_ERR_SSL_ALLOC_FAILED` (-0x7F00) before the first byte hits the
socket.

### PSA crypto on no-platform-entropy targets

mbedTLS 3.6's TLS 1.3 code path routes random-number requests through PSA
crypto. PSA's built-in entropy collector requires a platform entropy source
— which is exactly what `MBEDTLS_NO_PLATFORM_ENTROPY` turns off on
embedded targets.

The fix is `MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG` plus an integrator-supplied
`mbedtls_psa_external_get_random` that feeds PSA from the same CTR_DRBG
the classic API uses. This keeps the entropy chain single-rooted at your
hardware source.

Reference implementation (BDD target):
[BddTargetTlsSender_MbedTls_FreeRtosTcp.c](../Bdd/Targets/Common/BddTargetTlsSender_MbedTls_FreeRtosTcp.c).

### Bounded handshake retry

The adapter polls `mbedtls_ssl_handshake` against the non-blocking
transport — it never blocks the FreeRTOS service task indefinitely. The
retry budget is `HANDSHAKE_TIMEOUT_MILLISECONDS` (5 seconds at time of
writing, configurable in
[SolidSyslogMbedTlsStream.c](../Platform/MbedTls/Source/SolidSyslogMbedTlsStream.c)),
sleeping `HANDSHAKE_POLL_INTERVAL_MILLISECONDS` between attempts via the
caller's injected `Sleep` callback. A wedged peer surfaces as a fast Open
failure on the integrator's reconnect loop, not a hang.

## Reference integration

The cleanest end-to-end reference is the FreeRTOS BDD target:

- Adapter wiring:
  [Bdd/Targets/Common/BddTargetTlsSender_MbedTls_FreeRtosTcp.c](../Bdd/Targets/Common/BddTargetTlsSender_MbedTls_FreeRtosTcp.c)
- mbedTLS user config:
  [Bdd/Targets/FreeRtos/mbedtls_user_config.h](../Bdd/Targets/FreeRtos/mbedtls_user_config.h)
- Stream composition (TLS over FreeRTOS-Plus-TCP):
  [Bdd/Targets/FreeRtos/main.c](../Bdd/Targets/FreeRtos/main.c)

The host-side reference (POSIX) lives at
[Bdd/Targets/Common/BddTargetTlsSender_OpenSsl_PosixTcp.c](../Bdd/Targets/Common/BddTargetTlsSender_OpenSsl_PosixTcp.c)
and demonstrates the same shape using the OpenSSL-backed
`SolidSyslogTlsStream` for comparison.

## Common failure modes

| Symptom | Likely cause |
|---|---|
| `mbedtls_ssl_setup` returns `MBEDTLS_ERR_SSL_ALLOC_FAILED` (-0x7F00) | Heap too small or routed to wrong allocator. Check the FreeRTOS heap budget and confirm `mbedtls_platform_set_calloc_free` was called before any `mbedtls_*_init`. |
| `mbedtls_ssl_handshake` returns `MBEDTLS_ERR_ERROR_GENERIC_ERROR` (-0x0001) at the first call with no BIO traffic | `psa_crypto_init` was not called or returned non-zero. Verify PSA is initialised after DRBG is seeded if using `MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG`. |
| `psa_crypto_init` returns `PSA_ERROR_INSUFFICIENT_ENTROPY` (-148) | PSA's built-in entropy collector cannot find a source. Define `MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG` and provide `mbedtls_psa_external_get_random`. |
| Handshake fails with a hostname / SAN error | `ServerName` doesn't match a SAN on the peer's certificate. Pass the same string your peer's cert advertises (typically the DNS name the client is connecting to, not an IP). |
| `mbedtls_x509_crt_parse` fails on baked-in PEM arrays | Missing NUL terminator. `xxd -i` does not append one; allocate `sizeof(array) + 1` and write `'\0'` at the last byte before parsing. |

## Out of scope

The adapter does not own:

- **PEM-to-handle conversion.** Caller parses on the integrator's terms
  (filesystem, baked-in, HSM-pulled). The DI shape is intentional —
  baking parsing into the adapter would couple it to mbedTLS's filesystem
  abstractions, which are typically disabled on embedded targets.
- **Certificate rotation.** A rotation occurs when the integrator
  re-parses the PEM into a new `mbedtls_x509_crt` and rebuilds the
  adapter (or destroys and recreates the SolidSyslogStreamSender so the
  next Connect picks up the new chain).
- **HSM / TRNG integration.** The integrator's entropy source feeds
  CTR_DRBG via `mbedtls_entropy_add_source`. On targets with no real
  entropy source, the BDD reference uses a deliberately weak demo source
  to keep the path testable — this is loudly marked as not for production.
- **Per-connection TLS configuration.** A single adapter instance carries
  one ssl_config; deployments needing per-peer cipher / version pinning
  build multiple adapters.
