# wolfSSL setup

Wiring the wolfSSL TLS stream so a `SolidSyslogStreamSender` delivers over TLS,
and the at-rest policies so a `SolidSyslogBlockStore` seals every record it
keeps and verifies it on replay. What this pack needs and reports is on the
[wolfSSL](index.md) page. Every config field is documented on its struct. This
page is the order to wire them in.

## What to link

```cmake
set(SOLIDSYSLOG_PLATFORMS "WolfSsl;<Network>;<File>;<OsPrimitives>")
target_link_libraries(my_app PRIVATE SolidSyslog SolidSyslog::WolfSsl)
```

The placeholders are whichever platforms the [capability matrix](../index.md)
says fill the rest of what your build needs. See
[naming your platforms](../../build-integration.md#cmake) for how the list is
read.

## What you need

- wolfSSL built your way, with its include directory on your target. If it was
  built against a `user_settings.h`, define `WOLFSSL_USER_SETTINGS` on your
  target as you do for the library, so the adapter reads the same file.
- For TLS, a byte-transport `SolidSyslogStream` to carry the records - the
  TCP stream of whichever network platform the [capability
  matrix](../index.md) gives you - a millisecond sleep, and the trust anchors
  as PEM in memory.
- For AES-GCM, a `WC_RNG` you have initialised with `wc_InitRng`, which must
  outlive the policy.
- A key accessor: a `SolidSyslogKeyFunction` that copies the key into the
  buffer it is handed and reports its length. The policy fetches the key for
  every seal and open and wipes the copy afterwards, so the accessor is where
  the key lives, and a secure element or a key store sits behind it.

The adapter sources compile in your target, so add them to your build as
[adding it to your build](../../build-integration.md) describes.

```c
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/random.h>

#include "SolidSyslogBlockStore.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogWolfSslAesGcmPolicy.h"
#include "SolidSyslogWolfSslHmacSha256Policy.h"
#include "SolidSyslogWolfSslPemBufferCredentials.h"
#include "SolidSyslogWolfSslStream.h"
```

Include `wolfssl/options.h` before any wolfSSL header in your own sources when
your library came from `./configure`; the adapter does the same inside.

## 1. Install an error handler

Every fault below reaches `SolidSyslog_SetErrorHandler` and nowhere else. Install
one before creating anything, or a bad configuration degrades to a Null object,
so the store seals nothing or the sender has no stream to send over, and the
only report of it has already gone by.

## 2. Bring the library up

```c
wolfSSL_Init();
```

Before the first connection, and `wolfSSL_Cleanup` after the last; the stream
calls neither. Who owns the two calls, and why, is on the
[wolfSSL](index.md#requirements) page.

## 3. The transport

```c
struct SolidSyslogStream* tcp = SolidSyslogPosixTcpStream_Create(NULL);
```

Any `SolidSyslogStream` that moves bytes and answers would-block with 0 serves;
the stream opens it at each connection and closes it at each close. The caller
keeps it until the TLS stream is destroyed.

## 4. The credentials

```c
static const unsigned char CA_PEM[] = "-----BEGIN CERTIFICATE-----\n...";

static const struct SolidSyslogWolfSslPemBufferCredentialsConfig credentialsConfig = {
    {CA_PEM, sizeof CA_PEM - 1U}, /* CaPem */
    {NULL, 0U},                   /* ClientCertPem */
    {NULL, 0U},                   /* ClientKeyPem */
    NULL,                         /* PeerFingerprints */
    0U                            /* PeerFingerprintCount */
};
struct SolidSyslogWolfSslCredentials* credentials =
    SolidSyslogWolfSslPemBufferCredentials_Create(&credentialsConfig);
```

The length is the PEM's own; no terminator is needed. Supply `ClientCertPem`
and `ClientKeyPem` together for mutual TLS, or neither. The buffers are read at
every connection, so they must outlive the stream; rotating them in place and
moving the stream's version, below, is how material changes without a restart.

Create copies each buffer's address and length, so material you will rotate
lives in writable memory and its length cannot change afterwards. Size each
buffer for the largest PEM it will hold, give that whole size as its length,
and fill what the PEM does not use with NULs. To rotate, write the new PEM over
the buffer, clear the bytes after it to NUL, and move the version. The
[wolfSSL](index.md#credentials-come-from-a-credentials-source) page says which
task makes the change.

**Pinning the collector's certificate**, instead of or as well as a CA:

```c
static const char* const PINS[] = {
    "sha-256:E1:2D:53:2B:7C:6B:8A:29:A2:76:C8:64:36:0B:08:4B:"
    "7A:F1:9E:9D:0C:44:1B:23:5D:87:6E:A0:31:F5:C2:98",
};

static const struct SolidSyslogWolfSslPemBufferCredentialsConfig pinnedConfig = {
    {NULL, 0U}, /* CaPem - none: the pin alone authorises */
    {NULL, 0U}, /* ClientCertPem */
    {NULL, 0U}, /* ClientKeyPem */
    PINS,       /* PeerFingerprints */
    1U          /* PeerFingerprintCount */
};
```

A pin is the digest of the collector's DER certificate, which any certificate
tool prints as its SHA-256 fingerprint; put `sha-256:` in front of it, in
either case. A collector with no PKI - a self-signed certificate - is
authorised by its pin alone, on any wolfSSL build. With a CA as well, the
collector must satisfy both, and how the pin is then checked depends on your
build, as the [wolfSSL](index.md#what-it-depends-on-in-your-build) page says.
Across a renewal, list the current certificate and the next together. Why, and
what a `sha-1` pin costs, is under
[TLS obligations](../../tls.md#accept-a-peer-authorised-by-certificate-fingerprint).

## 5. The profile

```c
static void Profile(struct SolidSyslogWolfSslProfile* profile, void* context)
{
    (void) context;
    profile->ServerName = "collector.example.com";
}
```

Asked at every connection. The name is what the collector's certificate must
carry and what is sent as the server name indication; an address literal is
matched against the certificate's `iPAddress` entry instead. A single-label
name is one wolfSSL will not take, and the connection is refused at Open as
`SERVER_NAME_NOT_APPLIED`; declare the collector by its fully qualified name.
Leaving `ServerName` NULL connects on the chain alone and warns on every
connection, unless a pin names the collector; `""` does the same without the
warning, for a private CA that issues to no name.

`CipherList` is the ciphersuite policy, one colon-separated list for both TLS
versions; leave it NULL to offer what the build enables. The
[wolfSSL](index.md#what-a-connection-is-made-with) page gives the form and
what wolfSSL does with a name it does not know.

## 6. The stream and the sender

```c
static const struct SolidSyslogWolfSslStreamConfig streamConfig = {
    tcp,                     /* Transport */
    credentials,             /* Credentials */
    SolidSyslogPosix_Sleep,  /* Sleep */
    NULL, NULL,              /* GetHandshakeTimeoutMs, HandshakeTimeoutContext */
    NULL,                    /* Heap */
    Profile, NULL,           /* Profile, ProfileContext */
    NULL, NULL               /* Version, VersionContext */
};
struct SolidSyslogStream* tls = SolidSyslogWolfSslStream_Create(&streamConfig);
```

`Heap` is the hint wolfSSL's own allocator takes on a build with
`WOLFSSL_STATIC_MEMORY`; other builds ignore it. `Version` is a function that
returns a number you move whenever the credentials or the profile change; the
sender reconnects on its next pass and the new material is read. NULL says the
configuration never changes.

Wire `tls` into a `SolidSyslogStreamSenderConfig` as its `Stream`, with the
collector's endpoint, and the sender into the logger. The sender opens the
stream when it has a record to deliver and reopens it after any failure.

## 7. A key accessor

```c
static bool GetKey(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut)
{
    (void) context;
    if (capacity < 32U)
    {
        return false;
    }
    /* Copy the 32-byte key from wherever it is kept. */
    *keyLengthOut = 32U;
    return true;
}
```

The HMAC policy takes a key of 32 to `SOLIDSYSLOG_MAX_HMAC_KEY_SIZE` bytes and
treats a shorter one as unavailable. The AES-GCM policy takes exactly 32.

## 8. The policy

```c
static WC_RNG rng;
wc_InitRng(&rng);

static const struct SolidSyslogWolfSslAesGcmPolicyConfig aesConfig = {GetKey, NULL, &rng};
struct SolidSyslogSecurityPolicy* policy = SolidSyslogWolfSslAesGcmPolicy_Create(&aesConfig);
```

or, for integrity without confidentiality,

```c
static const struct SolidSyslogWolfSslHmacSha256PolicyConfig hmacConfig = {GetKey, NULL};
struct SolidSyslogSecurityPolicy* policy = SolidSyslogWolfSslHmacSha256Policy_Create(&hmacConfig);
```

Create returns the shared Null policy on a NULL field or an exhausted pool, and
reports which. Wire the handle into `SolidSyslogBlockStoreConfig.SecurityPolicy`.

## Memory

The stream and the credentials source are pool slots, sized by
`SOLIDSYSLOG_TLS_STREAM_POOL_SIZE` and `SOLIDSYSLOG_TLS_CREDENTIALS_POOL_SIZE`.
What a connection costs is wolfSSL's: a `WOLFSSL_CTX` and a `WOLFSSL` built at
Open from the allocator your build uses, on the `Heap` hint if it reads one,
and freed at Close. Nothing of one connection survives into the next.

The policies allocate nothing themselves. Each seal and open builds one
wolfCrypt context on the stack and frees it before returning, so the cost is
what wolfCrypt's `Hmac` or `Aes` needs under your build. The key copy lives on
the stack for the one call and is wiped with `wc_ForceZero`.

## Teardown

Destroy the sender before the stream, the stream before the credentials and the
transport, and call `wolfSSL_Cleanup` after the last of them. Destroying a
stream that is still open closes it, which sends close_notify and releases the
credentials.

Destroy the store before the policy, and the policy before freeing the RNG
with `wc_FreeRng`. The policy borrows the RNG and the store borrows the policy.

## What the handler sees

### From the stream and the credentials

`event->Source` is `&SolidSyslogWolfSslStreamErrorSource` or
`&SolidSyslogWolfSslPemBufferCredentialsErrorSource`. `event->Detail` is a code
from `SolidSyslogTlsStreamErrors.h` or `SolidSyslogTlsCredentialsErrors.h`; the
`SOLIDSYSLOG_TLS_STREAM_ERROR_` and `SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_`
prefixes are dropped below. A failed Open is reported once per run of the same
detail, because the sender retries it on every service pass; a different
detail, or a connection in between, reports again.

| What happened | Severity | Category | Detail | Then |
|---|---|---|---|---|
| a config field the stream cannot work without is NULL | `CRITICAL` | `BAD_CONFIG` | `NULL_CONFIG`, `NULL_TRANSPORT`, `NULL_SLEEP`, `NULL_CREDENTIALS` | Null stream returned; the sender delivers nothing |
| the pool is full | `CRITICAL` | `POOL_EXHAUSTED` | `POOL_EXHAUSTED` | Null stream returned |
| Destroy of a handle not from the pool | `WARNING` | `UNKNOWN_DESTROY` | `UNKNOWN_DESTROY` | ignored |
| Open on a stream already open | `CRITICAL` | `BAD_ARGUMENT` | `ALREADY_OPEN` | refused; the session held is untouched |
| wolfSSL would not build the context or the session | `ERROR` | `TLS_STREAM_INIT_FAILED` | `CONTEXT_INIT_FAILED`, `SESSION_INIT_FAILED` | the attempt fails; the sender retries |
| wolfSSL would not take the TLS 1.2 floor or a key-size floor | `ERROR` | `TLS_STREAM_INIT_FAILED` | `DEFAULTS_NOT_APPLIED` | as above |
| the credentials installed neither anchors nor a pin | `ERROR` | `BAD_CONFIG` | `NO_PEER_AUTHORISATION` | the attempt fails before any handshake |
| a pin not in the RFC 5425 form | `ERROR` | `BAD_CONFIG` | `FINGERPRINT_MALFORMED` | the attempt fails before any handshake |
| a pin naming `sha-1` | `WARNING` | `BAD_CONFIG` | `FINGERPRINT_SHA1` | connects |
| anchors and a pin together, on a build that cannot check both | `ERROR` | `BAD_CONFIG` | `PINS_WITH_ANCHORS_UNSUPPORTED` | the attempt fails before any handshake |
| the profile named no peer and no pin names it | `WARNING` | `BAD_CONFIG` | `SERVER_NAME_NOT_SET` | connects, chain-authenticated only |
| wolfSSL would not take the name or the indication | `ERROR` | `BAD_CONFIG` | `SERVER_NAME_NOT_APPLIED` | the attempt fails |
| wolfSSL knows none of the suites the cipher policy names | `ERROR` | `BAD_CONFIG` | `CIPHER_POLICY_REJECTED` | the attempt fails before any handshake |
| wolfSSL ran out of memory at any of those | `ERROR` | that step's category | `LIBRARY_OUT_OF_MEMORY` | the attempt fails |
| the peer's certificate matches no pin | `ERROR` | `TLS_STREAM_HANDSHAKE_FAILED` | `PEER_FINGERPRINT_MISMATCHED` | refused; the sender retries |
| no pin names a hash the build can compute, or no certificate was kept to digest | `ERROR` | `TLS_STREAM_HANDSHAKE_FAILED` | `FINGERPRINT_DIGEST_UNAVAILABLE` | as above |
| the peer's chain reaches no anchor, or fails a check the stream does not name | `ERROR` | `TLS_STREAM_HANDSHAKE_FAILED` | `PEER_CERTIFICATE_UNTRUSTED` | as above |
| the peer's certificate does not carry the expected name or address | `ERROR` | `TLS_STREAM_HANDSHAKE_FAILED` | `PEER_NAME_MISMATCHED` | as above |
| the peer's certificate is outside its validity period | `ERROR` | `TLS_STREAM_HANDSHAKE_FAILED` | `PEER_CERTIFICATE_EXPIRED`, `PEER_CERTIFICATE_NOT_YET_VALID` | as above |
| the peer or the protocol refused, and no check of the stream names it | `ERROR` | `TLS_STREAM_HANDSHAKE_FAILED` | `HANDSHAKE_REJECTED` | as above |
| the handshake did not finish within the budget | `WARNING` | `TLS_STREAM_HANDSHAKE_FAILED` | `HANDSHAKE_TIMEOUT` | as above |
| credentials config NULL, or a pin count with no list or a NULL pin | `CRITICAL` | `BAD_CONFIG` | `NULL_CONFIG`, `NULL_PEER_FINGERPRINT` | Null credentials returned |
| the trust anchors would not parse, or the buffer has no extent | `ERROR` | `BAD_CONFIG` | `TRUST_ANCHORS_NOT_PARSED` | the attempt fails |
| one half of the client credential | `WARNING` | `BAD_CONFIG` | `CLIENT_CREDENTIAL_INCOMPLETE` | connects without it |
| the client credential would not parse, or has no extent | `WARNING` | `BAD_CONFIG` | `CLIENT_CREDENTIAL_NOT_PARSED` | connects without it |
| wolfSSL ran out of memory holding the client credential | `WARNING` | `BAD_CONFIG` | `CLIENT_CREDENTIAL_NOT_INSTALLED` | connects without it |

A session that fails after the handshake - a write wolfSSL refuses, a close the
collector sent - is not reported by the stream: it closes itself and the
sender's delivery events carry the fact.

### From the policies

`event->Source` is `&SolidSyslogWolfSslHmacSha256PolicyErrorSource` or
`&SolidSyslogWolfSslAesGcmPolicyErrorSource`. `event->Detail` is a code from
`SolidSyslogHmacSha256PolicyErrors.h` or `SolidSyslogAesGcmPolicyErrors.h`; the
`SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_` and `SOLIDSYSLOG_AES_GCM_POLICY_ERROR_`
prefixes are dropped below.

| What happened | Severity | Category | Detail | Then |
|---|---|---|---|---|
| config NULL, `GetKey` NULL, or `Rng` NULL for AES-GCM | `CRITICAL` | `BAD_CONFIG` | `BAD_CONFIG` | Null policy returned; the store seals nothing |
| the pool is full | `CRITICAL` | `POOL_EXHAUSTED` | `POOL_EXHAUSTED` | Null policy returned |
| Destroy of a handle not from the pool | `WARNING` | `UNKNOWN_DESTROY` | `UNKNOWN_DESTROY` | ignored |
| `GetKey` returned false, or an AES key not 32 bytes | `ERROR` | `SECURITY_POLICY_KEY_UNAVAILABLE` | `KEY_UNAVAILABLE` | the seal fails and nothing is stored; an open could not run and the record waits |
| an HMAC key shorter than 32 bytes or longer than the buffer | `ERROR` | `SECURITY_POLICY_KEY_UNAVAILABLE` | `KEY_TOO_SHORT` | as above |
| wolfCrypt failed to compute the HMAC | `ERROR` | `SECURITY_POLICY_SEAL_FAILED` or `SECURITY_POLICY_OPEN_FAILED` | `HMAC_FAILED` | as above |
| the RNG would not supply a nonce | `ERROR` | `SECURITY_POLICY_SEAL_FAILED` | `NONCE_FAILED` | the seal fails and nothing is stored |
| wolfCrypt failed to encrypt or decrypt | `ERROR` | `SECURITY_POLICY_SEAL_FAILED` or `SECURITY_POLICY_OPEN_FAILED` | `ENCRYPT_FAILED`, `DECRYPT_FAILED` | as above |

A record that fails to verify, because it was altered or sealed under another
key, is rejected without a report: that verdict is the policy doing its job,
and the store reports the loss. A run of faults is reported once, and the next
seal or open that reaches a verdict re-arms it.
