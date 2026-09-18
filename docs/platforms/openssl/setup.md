# OpenSSL setup

Wiring `SolidSyslogOpenSslStream` so a `SolidSyslogStreamSender` delivers
RFC 5425 syslog over TLS. What any TLS stream must do is under
[TLS obligations](../../tls.md); what this pack needs and reports is on the
[OpenSSL](index.md) page. Every config field is documented on its struct. This
page is the order to wire them in.

## What you need

- OpenSSL 3.0 or later on the include and link path.
- A platform supplying the TCP stream, the address and the resolver. The
  [capability matrix](../index.md) says which fills each role on your target,
  and that platform's setup page shows how to create them.
- A `SolidSyslogSleepFunction`. The handshake polls and sleeps between polls.
  Your platform pack supplies one, or wrap your OS sleep in one line.

```cmake
set(SOLIDSYSLOG_PLATFORMS "OpenSsl;<Network>")
```

[Naming your platforms](../../build-integration.md#cmake) covers how the list
is read. OpenSSL is a system library, so the adapter compiles into
`libSolidSyslog.a` with no separate target to link;
[adding it to your build](../../build-integration.md) covers Make and IDE
builds.

```c
#include "SolidSyslogError.h"
#include "SolidSyslogOpenSslPemFileCredentials.h"
#include "SolidSyslogOpenSslStream.h"
#include "SolidSyslogStreamSender.h"
```

## The layering

TLS is a Stream wrapped around another Stream. The TLS stream carries the
records; the transport underneath carries the bytes.

```text
StreamSender -> SolidSyslogOpenSslStream -> your TCP stream -> socket
```

The TLS stream borrows its transport. It may close it but never destroys it, so
create the transport first and destroy it last.

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
from. The shipped source names them by file path; OpenSSL opens and parses the
files itself on every connection.

```c
static struct SolidSyslogOpenSslPemFileCredentialsConfig credentialsConfig;
credentialsConfig = (struct SolidSyslogOpenSslPemFileCredentialsConfig) {0};
credentialsConfig.CaBundlePath = "/etc/ssl/collector-ca.pem";

struct SolidSyslogOpenSslCredentials* credentials =
    SolidSyslogOpenSslPemFileCredentials_Create(&credentialsConfig);
```

Create copies the configuration, so set every field before the call. The path
strings, and the pin array below, are yours and must outlive the credentials.

**Mutual TLS.** Add both halves of the client credential. One without the other
is reported and the connection continues server-authenticated, so read the
handler rather than assume.

```c
credentialsConfig.ClientCertChainPath = "/etc/ssl/device-chain.pem";
credentialsConfig.ClientKeyPath       = "/etc/ssl/device-key.pem";
```

**Pinning the collector's certificate**, instead of or as well as a CA bundle:

```c
static const char* const pins[] = {
    "sha-256:E1:2D:53:2B:7C:6B:8A:29:A2:76:C8:64:36:0B:08:4B:"
    "7A:F1:9E:9D:0C:44:1B:23:5D:87:6E:A0:31:F5:C2:98",
};
credentialsConfig.PeerFingerprints     = pins;
credentialsConfig.PeerFingerprintCount = 1;
```

`openssl x509 -noout -fingerprint -sha256 -in collector.pem` prints the digest;
put `sha-256:` in front of it. Hex digits may be either case. A pin alone
authorises, so `CaBundlePath` may then be NULL; with both set, both must pass.
Across a renewal, list the current and the next certificate together. Why, and
what a `sha-1` pin costs, is under
[TLS obligations](../../tls.md#accept-a-peer-authorised-by-certificate-fingerprint).

## 3. A profile

What a connection is made with - the expected peer identity and the cipher
policy - is asked for at each connection through a callback, so a change takes
effect on the next connection.

```c
static void FillProfile(struct SolidSyslogOpenSslProfile* profile, void* context)
{
    (void) context;
    profile->ServerName = "collector.example.net";
}
```

The profile is zeroed before the call, so a field left alone takes OpenSSL's
default. `ServerName` is verified against the certificate and sent as SNI. Left
NULL, the peer is identified by a pin where one is configured; with neither, the
peer is chain-authenticated only and a WARNING says so on every connection. `""`
opts out of the name check without the warning. `CipherList` and `CipherSuites`
are OpenSSL's two cipher lists, for TLS 1.2 and TLS 1.3 respectively; leave both
alone unless your deployment holds a policy.

## 4. The stream

```c
struct SolidSyslogStream* transport = /* your platform's TCP stream */;

static struct SolidSyslogOpenSslStreamConfig tlsConfig;
tlsConfig = (struct SolidSyslogOpenSslStreamConfig) {0};
tlsConfig.Transport   = transport;
tlsConfig.Sleep       = /* your platform's sleep */;
tlsConfig.Credentials = credentials;
tlsConfig.Profile     = FillProfile;

struct SolidSyslogStream* tls = SolidSyslogOpenSslStream_Create(&tlsConfig);
```

`Transport`, `Sleep` and `Credentials` are required. A NULL is reported at
Create and the Null stream is returned, which delivers nothing.

Two optional pairs:

- `Version` and `VersionContext`: a function returning a number you increment
  when the files, the profile or the pin list change. The sender reads it on
  every record and reconnects when it moves, so a rotation applies without a
  restart. Leave it NULL if nothing changes at runtime.
- `GetHandshakeTimeoutMs` and `HandshakeTimeoutContext`: the per-attempt
  handshake deadline. NULL uses `SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS`.

## 5. The sender

Unchanged from plain TCP. It sees a Stream and does not know it is a TLS one.

```c
static struct SolidSyslogStreamSenderConfig senderConfig;
senderConfig = (struct SolidSyslogStreamSenderConfig) {0};
senderConfig.Resolver = resolver;      /* your platform's */
senderConfig.Stream   = tls;
senderConfig.Address  = address;       /* your platform's */
senderConfig.Endpoint = GetEndpoint;   /* fills the host and port */

struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderConfig);
```

The expected identity travels with the destination. If `Endpoint` can return a
different collector at runtime, `ServerName` has to change with it: move
`EndpointVersion` and the stream's `Version` together.

## Rotation

Put the new files in place and increment the stream's version. Nothing is held
between connections, and the shipped source reads the paths afresh each time,
so nothing has to be freed.

## Teardown

Reverse order: the sender, the address, the TLS stream, the credentials, then
the transport. The stream borrows the credentials and the transport, so both
outlive it.

## What the handler sees

`event->Source` is `&SolidSyslogOpenSslStreamErrorSource` or
`&SolidSyslogOpenSslPemFileCredentialsErrorSource`. `event->Detail` is a code
from `SolidSyslogTlsStreamErrors.h` or `SolidSyslogTlsCredentialsErrors.h`;
the `SOLIDSYSLOG_TLS_STREAM_ERROR_` and `SOLIDSYSLOG_TLS_CREDENTIALS_ERROR_`
prefixes are dropped below. `HANDSHAKE_FAILED` is
`SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED`.

| What happened | Severity | Category | Detail | Then |
|---|---|---|---|---|
| `Transport`, `Sleep` or `Credentials` left NULL | `CRITICAL` | `BAD_CONFIG` | `NULL_TRANSPORT`, `NULL_SLEEP`, `NULL_CREDENTIALS` | Null stream returned; nothing delivered |
| CA bundle will not load | `ERROR` | `BAD_CONFIG` | `TRUST_ANCHORS_NOT_LOADED` | connection refused |
| neither a CA bundle nor a pin | `ERROR` | `BAD_CONFIG` | `NO_PEER_AUTHORISATION` | refused |
| a pin not in the RFC 5425 form | `ERROR` | `BAD_CONFIG` | `FINGERPRINT_MALFORMED` | refused |
| a pin naming `sha-1` | `WARNING` | `BAD_CONFIG` | `FINGERPRINT_SHA1` | continues |
| `CipherList` or `CipherSuites` selects nothing | `ERROR` | `BAD_CONFIG` | `CIPHER_POLICY_REJECTED` | refused |
| no `ServerName` and no pin | `WARNING` | `BAD_CONFIG` | `SERVER_NAME_NOT_SET` | continues, chain-authenticated only |
| `ServerName` begins with a dot, or will not install | `ERROR` | `BAD_CONFIG` | `SERVER_NAME_NOT_APPLIED` | refused |
| half a client credential | `WARNING` | `BAD_CONFIG` | `CLIENT_CREDENTIAL_INCOMPLETE` | continues without it |
| client certificate or key will not load | `WARNING` | `BAD_CONFIG` | `CLIENT_CREDENTIAL_NOT_INSTALLED` | continues without it |
| client key does not match its certificate | `WARNING` | `BAD_CONFIG` | `CLIENT_CREDENTIAL_MISMATCHED` | continues without it |
| collector's chain reaches no anchor | `ERROR` | `HANDSHAKE_FAILED` | `PEER_CERTIFICATE_UNTRUSTED` | refused |
| collector's certificate matches no pin | `ERROR` | `HANDSHAKE_FAILED` | `PEER_FINGERPRINT_MISMATCHED` | refused |
| every pin names a hash the build cannot compute | `ERROR` | `HANDSHAKE_FAILED` | `FINGERPRINT_DIGEST_UNAVAILABLE` | refused |
| collector's name does not match `ServerName` | `ERROR` | `HANDSHAKE_FAILED` | `PEER_NAME_MISMATCHED` | refused |
| collector's certificate outside its dates | `ERROR` | `HANDSHAKE_FAILED` | `PEER_CERTIFICATE_EXPIRED`, `PEER_CERTIFICATE_NOT_YET_VALID` | refused |
| collector rejected the device | `ERROR` | `HANDSHAKE_FAILED` | `HANDSHAKE_REJECTED` | refused; check the client credential |
| handshake did not finish in time | `WARNING` | `HANDSHAKE_FAILED` | `HANDSHAKE_TIMEOUT` | retried |

A refused connection is retried on the sender's next pass. With a store
configured, records wait and replay when it succeeds. A successful connection
reports nothing.
