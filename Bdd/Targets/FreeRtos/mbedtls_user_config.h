/* mbedTLS integrator overrides for the FreeRTOS QEMU BDD target.
 *
 * mbedTLS supports an optional integrator config layered on top of its
 * built-in default via `-DMBEDTLS_USER_CONFIG_FILE="path"`. Anything we
 * #define here adds to the default; anything we #undef removes from it.
 *
 * The defaults that mbedTLS picks for a generic build assume a Unix or
 * Windows host — they pull /dev/urandom for entropy, use fopen for cert
 * loading, and call BSD sockets directly. The Cortex-M3 / FreeRTOS QEMU
 * BDD target has none of those, so we strip them and rely on the integrator
 * (main.c, slice 6b) to wire entropy, transport, and cert handles via DI.
 *
 * Anything not touched here keeps mbedTLS's default — including the cipher
 * suite set, RSA, ECC curves, SHA, AES, the PEM parser, x509 parsing, the
 * CTR-DRBG implementation, etc. Trimming further is a binary-size exercise
 * that belongs after the QEMU footprint is measured (slice 6c+).
 */

#ifndef BDD_TARGET_FREERTOS_MBEDTLS_USER_CONFIG_H
#define BDD_TARGET_FREERTOS_MBEDTLS_USER_CONFIG_H

/* Don't compile entropy_poll.c's Unix/Windows code path — mbedTLS would
 * otherwise #error on "Platform entropy sources only work on Unix and
 * Windows". main.c provides a weak entropy callback via
 * mbedtls_entropy_add_source(MBEDTLS_ENTROPY_SOURCE_WEAK). The
 * "demo-only entropy" caveat is documented in the integrator guide. */
#define MBEDTLS_NO_PLATFORM_ENTROPY

/* No filesystem from mbedTLS's point of view. PEMs are baked into the ELF
 * via xxd -i (slice 6b) and parsed via mbedtls_x509_crt_parse / _pk_parse_key
 * against the in-memory buffer, so MBEDTLS_FS_IO is unused dead code and a
 * link hazard against newlib stubs. */
#undef MBEDTLS_FS_IO

/* No BSD sockets — the transport is injected as a SolidSyslogStream
 * (PlusTcpTcpStream) and bridged into mbedTLS via mbedtls_ssl_set_bio
 * callbacks. MBEDTLS_NET_C would otherwise pull in <sys/socket.h>. */
#undef MBEDTLS_NET_C

/* No host clock. Cortex-M3 has no wall-clock; mbedTLS's cert-validity-date
 * check is skipped when this is off, which is fine for BDD with baked certs
 * carrying validity 20240101–20990101. Production integrators with an RTC
 * should turn MBEDTLS_HAVE_TIME[_DATE] back on. */
#undef MBEDTLS_HAVE_TIME
#undef MBEDTLS_HAVE_TIME_DATE

/* Disable mbedTLS's own threading primitive layer. The library runs on the
 * service task only — concurrent access to the ssl_context is not in scope
 * for this target. Per [[project-mbedtls-coexistence-contract]] the library
 * must never install threading hooks. */
#undef MBEDTLS_THREADING_C
#undef MBEDTLS_THREADING_PTHREAD

/* PSA's "internal trusted storage on filesystem" requires MBEDTLS_FS_IO,
 * which we just disabled. We don't use the PSA API surface anyway — the
 * adapter is built on the classic mbedTLS API (mbedtls_ssl_*, mbedtls_x509_*,
 * mbedtls_pk_*, mbedtls_ctr_drbg_*). */
#undef MBEDTLS_PSA_ITS_FILE_C
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C

/* mbedTLS's timing.c uses gettimeofday / clock_gettime — Unix/Windows only.
 * The adapter manages its own bounded handshake retry budget via the
 * injected Sleep callback, so MBEDTLS_TIMING_C is unused. */
#undef MBEDTLS_TIMING_C

/* Route mbedTLS allocations through a runtime-installed calloc/free pair. By
 * default mbedTLS calls libc calloc/free, which on this target funnels through
 * newlib into the small 4 KiB syscall heap in Bdd/Targets/FreeRtos/Common/
 * Syscalls.c — far too small for mbedTLS's per-context allocations (IN/OUT
 * buffers plus handshake state run ~10–20 KiB). Enabling
 * MBEDTLS_PLATFORM_MEMORY lets BddTargetTlsSender_MbedTls_PlusTcpTcp.c call
 * mbedtls_platform_set_calloc_free(...) to redirect those allocations to
 * pvPortMalloc, which uses the 96 KiB heap_4 region — the textbook
 * FreeRTOS+mbedTLS integration. */
#define MBEDTLS_PLATFORM_MEMORY

/* Route PSA crypto's randomness through an integrator-supplied callback rather
 * than PSA's internal entropy pool. mbedTLS 3.6's TLS 1.3 path is built on PSA,
 * so psa_crypto_init() must succeed before any TLS 1.3 handshake — and the
 * default PSA entropy collector returns PSA_ERROR_INSUFFICIENT_ENTROPY
 * (-148) on platforms with no real entropy source (which is us, with
 * MBEDTLS_NO_PLATFORM_ENTROPY defined above). With this define, PSA never
 * tries to seed itself; the integrator provides mbedtls_psa_external_get_random
 * (see BddTargetTlsSender_MbedTls_PlusTcpTcp.c) which feeds the same CTR_DRBG
 * the classic API uses, so PSA and the classic API share one entropy chain. */
#define MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG

/* Shrink the TLS record buffers from the 16 KiB default. The BDD syslog-ng
 * oracle's server cert + chain fits in 4 KiB IN comfortably, and the BDD
 * messages we send fit in 2 KiB OUT. Cuts ~28 KiB off the per-context
 * footprint — important when the FreeRTOS heap also has to satisfy
 * FreeRTOS-Plus-TCP socket buffers, the SolidSyslog Service task, and the
 * interactive task all at once. Embedded integrators replicating this
 * footprint should re-tune both knobs against their peer's largest TLS
 * record. */
#define MBEDTLS_SSL_IN_CONTENT_LEN 4096
#define MBEDTLS_SSL_OUT_CONTENT_LEN 2048

#endif /* BDD_TARGET_FREERTOS_MBEDTLS_USER_CONFIG_H */
