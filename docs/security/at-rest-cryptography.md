# At-rest cryptography

The block store seals every record through an injected
`SolidSyslogSecurityPolicy` before it is written and opens it again on
replay-read. The policy owns a fixed-size *trailer* appended to each record;
`SolidSyslogBlockStore` treats the trailer as opaque.

## Policy spectrum

| Policy | Integrity | Confidentiality | Trailer | Use |
|---|---|---|---|---|
| `SolidSyslogNullSecurityPolicy` | none | none | 0 B | no protection |
| `SolidSyslogCrc16Policy` | checksum | none | 2 B | accidental-corruption detection |
| `SolidSyslogOpenSslHmacSha256Policy` / `SolidSyslogMbedTlsHmacSha256Policy` | cryptographic | none | 32 B | tamper-evident records (IEC 62443 CR 3.4, and CR 3.9 for the audit store) |
| `SolidSyslogOpenSslAesGcmPolicy` / `SolidSyslogMbedTlsAesGcmPolicy` | cryptographic | AEAD | 28 B | encrypted + tamper-evident records (adds CR 4.1) |

The library is the abstraction, not an algorithm catalogue: one HMAC variant and
one AEAD variant are demonstrated. Other algorithms (ChaCha20-Poly1305,
HMAC-SHA384, AES-GCM-SIV, …) plug into the same `SolidSyslogSecurityPolicy` slot
following the same pattern.

## AES-256-GCM

Two implementations of one policy, `SolidSyslogOpenSslAesGcmPolicy` and
`SolidSyslogMbedTlsAesGcmPolicy`, selected by which crypto library your build
already links rather than by which target you are on. Both fill the same
`SolidSyslogSecurityPolicy` slot and produce the same record format, so the
choice is a build-time one and nothing above it changes. The
[platform × capability matrix](../platforms/index.md) says where each is
available.

Authenticated encryption: the record body (the syslog message) is encrypted in
place, while the cleartext header the store needs to find the record (magic +
length) is authenticated as associated data but left readable. The 28-byte
trailer is `nonce (12) ‖ tag (16)`.

- Key: a caller-supplied 32-byte key fetched on demand through the
  integrator's `SolidSyslogKeyFunction` and wiped after every operation —
  `OPENSSL_cleanse` on the OpenSSL policy, `mbedtls_platform_zeroize` on the Mbed TLS
  one. The key is never stored on the policy instance.
- Nonce: a fresh 12-byte random nonce per record, written into the trailer.
  The OpenSSL policy draws it from `RAND_bytes`; the Mbed TLS policy draws it from a
  seeded CTR-DRBG you inject as `Rng` and continue to own, because Mbed TLS has no
  context-free RNG. Random (not counter-based) nonces carry no
  cross-power-cycle counter state to lose, so a reboot cannot force the
  systematic reuse a reset counter would — provided each new DRBG instance is seeded
  from fresh entropy. On the Mbed TLS path that guarantee is yours: a CTR-DRBG
  re-seeded from a repeating source reproduces its output, and so reproduces nonces
  under the same key. [Integrating Mbed TLS](../platforms/mbedtls/setup.md) states the
  entropy the adapter assumes and the silent failure mode when it is missing.
  Uniqueness stays probabilistic, bounded by the 2³² per-key envelope below.
- Failure: `OpenRecord` returns a single `bool`. A tag mismatch (the
  expected tamper-detected outcome) returns `false` silently and the record is
  discarded on read; only a genuine library or crypto-backend error is routed to the
  error handler.

### Nonce envelope (not a concern at syslog volumes)

GCM with random nonces is bounded by NIST SP 800-38D §8.3 to 2³² invocations per
key (the birthday-collision ceiling). For at-rest security-event logging
this is a non-issue: at a sustained one event per second, counting every source
that shares the key, that envelope is roughly 136 years, orders of magnitude
beyond any realistic volume. The figure is recorded here as headroom, not as a caveat. An integrator
whose volumes or key-rotation discipline could approach it should plug a
nonce-misuse-resistant mode (e.g. AES-256-GCM-SIV) into the same policy slot.

## Key management is the integrator's responsibility

The library provides the cryptographic primitive and the abstraction. It does
not provision, store, rotate, or destroy keys. The configured policy is what makes a
record tamper-evident; key custody, rotation, audit of key use, and the product-level
controls around them live in the integrator's product. The reference policies
take a key-accessor callback so the key material's lifetime stays under the
integrator's control.
