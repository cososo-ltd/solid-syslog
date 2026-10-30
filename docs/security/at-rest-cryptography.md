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
| `SolidSyslogOpenSslHmacSha256Policy` / `SolidSyslogMbedTlsHmacSha256Policy` | cryptographic | none | 32 B | tamper-evident records |
| `SolidSyslogOpenSslAesGcmPolicy` / `SolidSyslogMbedTlsAesGcmPolicy` | cryptographic | AEAD | 28 B | encrypted and tamper-evident records |

The library is the abstraction, not an algorithm catalogue: one HMAC variant and
one AEAD variant are demonstrated. Other algorithms (ChaCha20-Poly1305,
HMAC-SHA384, AES-GCM-SIV...) plug into the same `SolidSyslogSecurityPolicy` slot
following the same pattern, provided the trailer fits
`SOLIDSYSLOG_MAX_INTEGRITY_SIZE`: 32 bytes by default, where HMAC-SHA384 needs 48.
The store refuses a policy whose trailer does not fit, and stores nothing, rather
than keep records without it; the policies the library ships fail the build
instead.

A store keeps records unsealed and in clear whenever it holds the Null policy, and
two ways to reach it are easy to miss: no policy wired, which is optional; and a
keyed policy whose Create failed - a missing key callback or RNG, or an exhausted
pool - which is reported once at `CRITICAL`.

## AES-256-GCM

Two implementations of one policy, `SolidSyslogOpenSslAesGcmPolicy` and
`SolidSyslogMbedTlsAesGcmPolicy`, selected by which crypto library your build
already links rather than by which target you are on. Both fill the same
`SolidSyslogSecurityPolicy` slot and produce the same record format, so the
choice is a build-time one and nothing above it changes. The
[capability matrix](../platforms/index.md) says where each is
available.

Authenticated encryption: the record body (the syslog message) is encrypted in
place, while the cleartext header the store needs to find the record (magic +
length) is authenticated as associated data but left readable. The 28-byte
trailer is `nonce (12) ‖ tag (16)`.

- Key: a caller-supplied 32-byte key fetched on demand through the
  integrator's `SolidSyslogKeyFunction` and wiped after every operation -
  `OPENSSL_cleanse` on the OpenSSL policy, `mbedtls_platform_zeroize` on the Mbed TLS
  one. The key is never stored on the policy instance.
- Nonce: a fresh 12-byte random nonce per record, written into the trailer.
  The OpenSSL policy draws it from `RAND_bytes`; the Mbed TLS policy draws it from a
  seeded CTR-DRBG you inject as `Rng` and continue to own, because Mbed TLS has no
  context-free RNG. Random (not counter-based) nonces carry no
  cross-power-cycle counter state to lose, so a reboot cannot force the
  systematic reuse a reset counter would - provided each new DRBG instance is seeded
  from fresh entropy. On the Mbed TLS path that guarantee is yours: a CTR-DRBG
  re-seeded from a repeating source reproduces its output, and so reproduces nonces
  under the same key. [Integrating Mbed TLS](../platforms/mbedtls/setup.md) states the
  entropy the adapter assumes and the silent failure mode when it is missing.
  Uniqueness stays probabilistic, bounded by the 2^32 per-key envelope below.
- Failure: `OpenRecord` tells a verdict from a fault. A tag mismatch (the
  expected tamper-detected outcome) is rejected silently, and the store discards
  the record and reports the discard. A missing key or a crypto-backend error is
  not a verdict: the policy reports it, and the store keeps the record until the
  policy can run. The [SecurityPolicy contract](../api/structSolidSyslogSecurityPolicy.md)
  states both. Sealing has no such reprieve: a record written while the key is
  unavailable is not stored, and the store reports it lost as
  `SOLIDSYSLOG_BLOCK_STORE_ERROR_SEAL_FAILED`.

### Nonce envelope

GCM with random 96-bit nonces is bounded by NIST SP 800-38D section 8.3 to 2^32
seals per key. One device sealing one record a second takes about 136 years to
reach that, but the bound counts every seal under the key: a thousand such devices
sharing one key reach it in under two months. Provision a key per device - which
also confines a key extracted from one device to that device's records - and where
one key must seal more, plug a nonce-misuse-resistant mode such as AES-256-GCM-SIV
into the same slot.

## What a seal covers

A policy seals the content of one record: its magic, length and message. It does
not seal the byte after the trailer that marks the record sent or discarded, the
record's place in the store, or which records are there. Someone who can write the
medium, without the key, can therefore cut records from the end of a block, mark
unsent records as sent so they are never delivered, or mark sent records as unsent -
or put back an older copy of a block - so the device delivers them again. None of
that fails verification, because every record left is genuine. Where
`SolidSyslogMetaSd` is wired, the collector sees removal as a gap in the sequence
number and replay as a repeat, except for records removed from the end of the store
before a restart, which leave no gap. Where the medium can be reached, protect it on
the platform as well.

## Key management is the integrator's responsibility

The library provides the cryptographic primitive and the abstraction. It does
not provision, store, rotate, or destroy keys. The configured policy is what makes a
record tamper-evident; key custody, rotation, audit of key use, and the product-level
controls around them live in the integrator's product. The reference policies
take a key-accessor callback so the key material's lifetime stays under the
integrator's control.

Rotate the key, or change the policy, once the store has drained. A record names
neither the key nor the policy that sealed it, so the store opens each stored record
with the policy and key in force now, and one sealed under another is discarded and
reported under `SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED`, as a tampered record is.
