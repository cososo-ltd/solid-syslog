# Mbed TLS setup

Wiring `SolidSyslogMbedTlsStream` so a `SolidSyslogStreamSender` delivers
RFC 5425 syslog over TLS. What any TLS stream must do is under
[TLS obligations](../../tls.md); what this pack needs and reports is on the
[Mbed TLS](index.md) page. Every config field is documented on its struct. This
page is the order to wire them in, and what bites on the way.

## What you need

- Mbed TLS built against your own `mbedtls_config.h`, with
  `MBEDTLS_HAVE_TIME_DATE` on. The adapter will not build without it; the
  [Mbed TLS](index.md#certificate-validity-depends-on-your-build-carrying-a-clock)
  page says why, and what a target with no clock defines instead.
- A seeded `mbedtls_ctr_drbg_context`. The stream and both credentials sources
  take one, and one serves all of them. It must outlive everything built on it.
- A platform supplying the TCP stream, the address and the resolver. The
  [capability matrix](../index.md) says which fills each role on your target,
  and that platform's setup page shows how to create them.
- A `SolidSyslogSleepFunction`. The handshake polls and sleeps between polls.
  Your platform pack supplies one, or wrap your OS sleep in one line.

The adapter sources compile in your target, so add them to your build as
[adding it to your build](../../build-integration.md) describes. Include the
Mbed TLS headers before the adapter's, which declare the types by forward
reference only:

```c
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/pk.h>
#include <mbedtls/x509_crt.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMbedTlsHandleCredentials.h"
#include "SolidSyslogMbedTlsPemBufferCredentials.h"
#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogStreamSender.h"
```

## The layering

TLS is a Stream wrapped around another Stream. The TLS stream carries the
records; the transport underneath carries the bytes.

```text
StreamSender -> SolidSyslogMbedTlsStream -> your TCP stream -> your TCP/IP stack
                       |
                       +-- a credentials source (your CA, certificate and key)
                       +-- your DRBG
```

The TLS stream borrows its transport, its credentials and the DRBG. It may close
the transport but never destroys any of them.

## Before you wire it

If Mbed TLS already runs on the target for something else, skip this section:
the adapter takes handles you have already built and touches no global state.

Otherwise port Mbed TLS first, following the
[upstream porting guide](https://mbed-tls.readthedocs.io/en/latest/kb/how-to/how-do-i-port-mbed-tls-to-a-new-environment-OS/).
These then fail in ways that are hard to read:

**Seed the DRBG from a source registered as `MBEDTLS_ENTROPY_SOURCE_STRONG`.**
Without one, `mbedtls_entropy_func` never reaches its threshold and every
`mbedtls_ctr_drbg_seed` returns `MBEDTLS_ERR_CTR_DRBG_ENTROPY_SOURCE_FAILED`.
Production entropy is a hardware question: a true random number generator, a
security element, or a board-specific source.

**Call `psa_crypto_init()` after the DRBG is seeded.** Mbed TLS 3.6 routes
TLS 1.3 through PSA; uninitialised, the first handshake state transition returns
`MBEDTLS_ERR_ERROR_GENERIC_ERROR` before any byte reaches the socket.

**With `MBEDTLS_NO_PLATFORM_ENTROPY` defined, give PSA a strong source.**
`mbedtls_entropy_init` registers none of its own, and `psa_crypto_init` fails
with `PSA_ERROR_INSUFFICIENT_ENTROPY`. Define `MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG`
and provide `mbedtls_psa_external_get_random` wrapping the DRBG you seeded, so
PSA and the classic API run off one chain. A target whose randomness already
arrives through a hardware poll can define `MBEDTLS_ENTROPY_HARDWARE_ALT` and
provide `mbedtls_hardware_poll` instead.

## 1. Install an error handler

Every fault below reaches this handler and nothing else. Install it before
anything is created.

```c
static void OnError(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) context;
    /* event->Source names the class that reported, event->Category the kind
       of fault, event->Detail the code. The table at the end of this page
       lists what a first connection can produce. */
}

SolidSyslog_SetErrorHandler(OnError, NULL);
```

## 2. A credentials source

Where the trust anchors, any pinned fingerprints and the client credential come
from. Two sources ship, and neither opens a file, so the adapter runs on targets
built without `MBEDTLS_FS_IO`.

**Handles you have already parsed.** Parse the CA chain with
`mbedtls_x509_crt_parse`, and for mutual TLS the client certificate and
`mbedtls_pk_parse_key` for its key, from wherever the build keeps them. PEM
input to those parsers must be NUL-terminated, with the length counting the
terminator. The handles are yours and must outlive the credentials.

```c
struct SolidSyslogMbedTlsHandleCredentialsConfig handleConfig = {
    .Rng     = &drbg,
    .CaChain = &caChain,
};
struct SolidSyslogMbedTlsCredentials* credentials =
    SolidSyslogMbedTlsHandleCredentials_Create(&handleConfig);
```

**PEM held in memory**, parsed once per connection and released when it ends,
so between connections nothing but your own PEM is in RAM - which may be flash.

```c
static const unsigned char caPem[] = "-----BEGIN CERTIFICATE-----\n...";

struct SolidSyslogMbedTlsPemBufferCredentialsConfig pemConfig = {
    .CaPem = {caPem, sizeof(caPem)},
    .Rng   = &drbg,
};
struct SolidSyslogMbedTlsCredentials* credentials =
    SolidSyslogMbedTlsPemBufferCredentials_Create(&pemConfig);
```

Each length includes the terminating NUL. A string literal's `sizeof` does;
an array from `xxd -i` does not, so give that buffer one byte more and set it. A
length one short is reported rather than left to fail as "not a certificate".
The key must not be encrypted. One PEM-buffer source serves one stream at a
time; wire another for a second stream.

`Rng` is required on both sources: it checks a client key against its
certificate. Create copies the configuration, so set every field before the
call.

**Mutual TLS.** Add both halves of the client credential. One without the other
is reported and the connection continues server-authenticated, so read the
handler rather than assume.

```c
handleConfig.ClientCertChain = &clientCert;
handleConfig.ClientKey       = &clientKey;
/* or */
pemConfig.ClientCertPem = {clientCertPem, sizeof(clientCertPem)};
pemConfig.ClientKeyPem  = {clientKeyPem, sizeof(clientKeyPem)};
```

**Pinning the collector's certificate**, instead of or as well as a CA chain.
Both sources take the same two fields:

```c
static const char* const pins[] = {
    "sha-256:E1:2D:53:2B:7C:6B:8A:29:A2:76:C8:64:36:0B:08:4B:"
    "7A:F1:9E:9D:0C:44:1B:23:5D:87:6E:A0:31:F5:C2:98",
};
handleConfig.PeerFingerprints     = pins;
handleConfig.PeerFingerprintCount = 1;
```

The digest is over the certificate's DER, which is what any certificate tool
prints as its SHA-256 fingerprint; put `sha-256:` in front of it. Hex digits may
be either case. A pin alone authorises, so `CaChain` or `CaPem` may then be
left unset; with both set, both must pass. Across a renewal, list the current
and the next certificate together. Why, and what a `sha-1` pin costs, is under
[TLS obligations](../../tls.md#accept-a-peer-authorised-by-certificate-fingerprint).

## 3. A profile

What a connection is made with - the expected peer identity and the ciphersuite
policy - is asked for at each connection through a callback, so a change takes
effect on the next connection.

```c
static void FillProfile(struct SolidSyslogMbedTlsProfile* profile, void* context)
{
    (void) context;
    profile->ServerName = "syslog.example.com";
}
```

The profile is zeroed before the call, so a field left alone takes the build's
default. `ServerName` is verified against the certificate and sent as SNI. Left
NULL, the peer is identified by a pin where one is configured; with neither, the
peer is chain-authenticated only and a WARNING says so on every connection. `""`
opts out of the name check without the warning. `CipherSuites` is a
0-terminated array of `MBEDTLS_TLS_*` and `MBEDTLS_TLS1_3_*` identifiers that
must stay valid for the connection; leave it alone unless your deployment holds
a policy.

## 4. The stream

```c
struct SolidSyslogMbedTlsStreamConfig tlsConfig = {
    .Transport   = /* your platform's TCP stream */,
    .Sleep       = /* your platform's sleep */,
    .Rng         = &drbg,
    .Credentials = credentials,
    .Profile     = FillProfile,
};
struct SolidSyslogStream* tls = SolidSyslogMbedTlsStream_Create(&tlsConfig);
```

`Transport`, `Sleep`, `Rng` and `Credentials` are required. A NULL is reported
at Create and the Null stream is returned, which delivers nothing.

Two optional pairs:

- `Version` and `VersionContext`: a function returning a number you increment
  when the material, the profile or the pin list change. The sender reads it on
  every record and reconnects when it moves, so a rotation applies without a
  restart. Leave it NULL if nothing changes at runtime.
- `GetHandshakeTimeoutMs` and `HandshakeTimeoutContext`: the per-attempt
  handshake deadline. NULL uses `SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS`.

## 5. The sender

Unchanged from plain TCP. It sees a Stream and does not know it is a TLS one.

```c
struct SolidSyslogStreamSenderConfig senderConfig = {
    .Resolver = resolver,       /* your platform's */
    .Stream   = tls,
    .Address  = address,        /* your platform's */
    .Endpoint = GetEndpoint,    /* fills the host and port */
};
struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderConfig);
```

The expected identity travels with the destination. If `Endpoint` can return a
different collector at runtime, `ServerName` has to change with it: move
`EndpointVersion` and the stream's `Version` together.

## Rotation

With the PEM-buffer source, point the config at the new PEM and increment the
stream's version; the next connection parses it. With the handle source the old
handles are still being read by an open connection, so free and re-parse them
only after it has closed: call `SolidSyslogSender_Disconnect` from the task
that services the library and re-parse when it returns, or do it in a
credentials source's own `Release`. The [Mbed TLS](index.md) page covers the
lock that `Release` may run under.

## Memory

The adapter allocates nothing itself. A session costs what Mbed TLS allocates
under your `mbedtls_config.h`, dominated by the record buffers, whose defaults
are sized for a host. Budget for every session you run concurrently, from the
[upstream sizing guidance](https://mbed-tls.readthedocs.io/). An allocation
that fails in `mbedtls_ssl_setup` is reported as `SESSION_INIT_FAILED`.

## Teardown

Reverse order: the sender, the address, the TLS stream, the credentials, the
transport, then your handles and the DRBG. The stream borrows the credentials
and the transport, and the credentials borrow your handles, so each outlives
what borrows it.

## What the handler sees

`event->Source` is `&SolidSyslogMbedTlsStreamErrorSource`,
`&SolidSyslogMbedTlsHandleCredentialsErrorSource` or
`&SolidSyslogMbedTlsPemBufferCredentialsErrorSource`. `event->Detail` is a code
from `SolidSyslogTlsStreamErrors.h` or `SolidSyslogTlsCredentialsErrors.h`;
the `SOLIDSYSLOG_TLS_STREAM_ERROR_` and `SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_`
prefixes are dropped below. `HANDSHAKE_FAILED` and `INIT_FAILED` are
`SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED` and
`SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED`.

| What happened | Severity | Category | Detail | Then |
|---|---|---|---|---|
| `Transport`, `Sleep`, `Rng` or `Credentials` left NULL | `CRITICAL` | `BAD_CONFIG` | `NULL_TRANSPORT`, `NULL_SLEEP`, `NULL_RNG`, `NULL_CREDENTIALS` | Null stream returned; nothing delivered |
| `Rng` left NULL on a credentials source | `CRITICAL` | `BAD_CONFIG` | `NULL_RNG` | Null credentials returned; every connection refused |
| `mbedtls_ssl_config_defaults` or `mbedtls_ssl_setup` failed | `ERROR` | `INIT_FAILED` | `DEFAULTS_NOT_APPLIED`, `SESSION_INIT_FAILED` | refused; usually memory |
| a PEM length that does not count the NUL | `ERROR` for the CA, `WARNING` for the client | `BAD_CONFIG` | `PEM_NOT_TERMINATED` | CA: refused. Client: continues without it |
| CA PEM will not parse | `ERROR` | `BAD_CONFIG` | `TRUST_ANCHORS_NOT_PARSED` | refused |
| neither a CA chain nor a pin | `ERROR` | `BAD_CONFIG` | `NO_PEER_AUTHORISATION` | refused |
| a PEM-buffer source already serving another stream | `ERROR` | `BAD_CONFIG` | `ALREADY_IN_USE` | refused until that stream closes |
| a pin not in the RFC 5425 form | `ERROR` | `BAD_CONFIG` | `FINGERPRINT_MALFORMED` | refused |
| a pin naming `sha-1` | `WARNING` | `BAD_CONFIG` | `FINGERPRINT_SHA1` | continues |
| no `ServerName` and no pin | `WARNING` | `BAD_CONFIG` | `SERVER_NAME_NOT_SET` | continues, chain-authenticated only |
| `ServerName` begins with a dot, or will not install | `ERROR` | `BAD_CONFIG` | `SERVER_NAME_NOT_APPLIED` | refused |
| half a client credential | `WARNING` | `BAD_CONFIG` | `CLIENT_CREDENTIAL_INCOMPLETE` | continues without it |
| client PEM will not parse | `WARNING` | `BAD_CONFIG` | `CLIENT_CREDENTIAL_NOT_PARSED` | continues without it |
| client key does not match its certificate | `WARNING` | `BAD_CONFIG` | `CLIENT_CREDENTIAL_MISMATCHED` | continues without it |
| collector's chain reaches no anchor | `ERROR` | `HANDSHAKE_FAILED` | `PEER_CERTIFICATE_UNTRUSTED` | refused |
| collector's certificate matches no pin | `ERROR` | `HANDSHAKE_FAILED` | `PEER_FINGERPRINT_MISMATCHED` | refused |
| every pin names a hash the build compiled out | `ERROR` | `HANDSHAKE_FAILED` | `FINGERPRINT_DIGEST_UNAVAILABLE` | refused |
| collector's name does not match `ServerName` | `ERROR` | `HANDSHAKE_FAILED` | `PEER_NAME_MISMATCHED` | refused |
| collector's certificate outside its dates | `ERROR` | `HANDSHAKE_FAILED` | `PEER_CERTIFICATE_EXPIRED`, `PEER_CERTIFICATE_NOT_YET_VALID` | refused |
| collector rejected the device | `ERROR` | `HANDSHAKE_FAILED` | `HANDSHAKE_REJECTED` | refused; check the client credential |
| handshake did not finish in time | `WARNING` | `HANDSHAKE_FAILED` | `HANDSHAKE_TIMEOUT` | retried |

A refused connection is retried on the sender's next pass. With a store
configured, records wait and replay when it succeeds. A successful connection
reports nothing.
