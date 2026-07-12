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
| `SolidSyslogOpenSslHmacSha256Policy` / `SolidSyslogMbedTlsHmacSha256Policy` | cryptographic | none | 32 B | tamper-evident records (IEC 62443 SR 3.4) |
| `SolidSyslogOpenSslAesGcmPolicy` | cryptographic | AEAD | 28 B | encrypted + tamper-evident records (SR 3.4 + SR 4.1) |

The library is the abstraction, not an algorithm catalogue: one HMAC variant and
one AEAD variant are demonstrated. Other algorithms (ChaCha20-Poly1305,
HMAC-SHA384, AES-GCM-SIV, …) plug into the same `SolidSyslogSecurityPolicy` slot
following the same pattern.

## AES-256-GCM (`SolidSyslogOpenSslAesGcmPolicy`)

Authenticated encryption: the record body (the syslog message) is encrypted in
place, while the cleartext header the store needs to find the record (magic +
length) is authenticated as associated data but left readable. The 28-byte
trailer is `nonce (12) ‖ tag (16)`.

- Key: a caller-supplied 32-byte key fetched on demand through the
  integrator's `SolidSyslogKeyFunction` and `OPENSSL_cleanse`'d after every
  operation. The key is never stored on the policy instance.
- Nonce: a fresh 12-byte random nonce per record via OpenSSL `RAND_bytes`,
  written into the trailer. Random (not counter-based) nonces carry no
  cross-power-cycle state, so a reboot can never reuse one.
- Failure: `OpenRecord` returns a single `bool`. A tag mismatch (the
  expected tamper-detected outcome) returns `false` silently and the record is
  discarded on read; only a genuine library/OpenSSL error is routed to the
  error handler.

### Nonce envelope (not a concern at syslog volumes)

GCM with random nonces is bounded by NIST SP 800-38D §8.3 to 2³² invocations per
key (the birthday-collision ceiling). For at-rest security-event logging
this is a non-issue: at a sustained one event per second that envelope is roughly
136 years under a single key, orders of magnitude beyond any realistic
volume. The figure is recorded here as headroom, not as a caveat. An integrator
whose volumes or key-rotation discipline could approach it should plug a
nonce-misuse-resistant mode (e.g. AES-256-GCM-SIV) into the same policy slot.

## Key management is the integrator's responsibility

The library provides the cryptographic primitive and the abstraction. It does
not provision, store, rotate, or destroy keys; those controls (and the
tamper-evident storage, key custody, and audit that distinguish an IEC 62443 SL3
deployment from SL4) live in the integrator's product. The reference policies
take a key-accessor callback so the key material's lifetime stays under the
integrator's control.
