# OpenSSL setup

Wiring `SolidSyslogTlsStream` over a plain TCP stream so a
`SolidSyslogStreamSender` delivers RFC 5425 syslog over TLS. Read
[OpenSSL](index.md) first for what the adapter guarantees and what it leaves to
you — this page is the mechanics.

## What you need

OpenSSL 3.0 or later on the include and link path. With CMake, selecting the
platform is enough:

```cmake
set(SOLIDSYSLOG_PLATFORMS "Posix;OpenSsl")
```

OpenSSL is a stable system API rather than a header-configured upstream, so the
adapter compiles straight into `libSolidSyslog.a` and there is no separate
target to link. [Adding it to your build](../../build-integration.md) covers the
Make and IDE routes.

## The layering

TLS is a Stream wrapped around another Stream. The TLS adapter carries the
records; the transport underneath carries the bytes, and it can be any Stream —
here the POSIX TCP one.

```text
StreamSender → SolidSyslogTlsStream → SolidSyslogPosixTcpStream → socket
```

The TLS stream **borrows** its transport. It may close it, but it never destroys
it: the transport is yours to create and to destroy, and it must stay valid
until `SolidSyslogTlsStream_Destroy`.

## Wiring it

```c
struct SolidSyslogStream* transport = SolidSyslogPosixTcpStream_Create(NULL);

static struct SolidSyslogTlsStreamConfig tlsConfig;
tlsConfig = (struct SolidSyslogTlsStreamConfig) {0};
tlsConfig.Transport = transport;
tlsConfig.Sleep = SolidSyslogPosixSleep;      /* required — no fallback */
tlsConfig.CaBundlePath = "/etc/ssl/collector-ca.pem";
tlsConfig.ServerName = "collector.example.net";

struct SolidSyslogStream* tls = SolidSyslogTlsStream_Create(&tlsConfig);
```

Zero-initialise the config before filling it. Every field you leave NULL is a
documented default, and a struct you have not cleared is not.

For mutual TLS, add the client credential — both fields or neither, since
supplying one without the other is rejected at `Open`:

```c
tlsConfig.ClientCertChainPath = "/etc/ssl/device-chain.pem";
tlsConfig.ClientKeyPath       = "/etc/ssl/device-key.pem";
```

Then the sender, unchanged from the plain-TCP case — it sees a Stream and does
not know or care that it is a TLS one:

```c
static struct SolidSyslogStreamSenderConfig senderConfig;
senderConfig = (struct SolidSyslogStreamSenderConfig) {0};
senderConfig.Resolver = resolver;
senderConfig.Stream   = tls;
senderConfig.Address  = SolidSyslogPosixAddress_Create();
senderConfig.Endpoint = GetEndpoint;
struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderConfig);
```

Tear down in reverse order: sender, address, TLS stream, then the transport you
created.

## Timeouts

The handshake is bounded. `GetHandshakeTimeoutMs` gives a per-attempt deadline
in milliseconds; leaving it NULL uses the `SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS`
tunable, which defaults to 5000. The `Sleep` callback is what the bounded retry
waits on between `WANT_READ` / `WANT_WRITE` polls, which is why it has no
default — the library will not pick a blocking primitive on your behalf.

## Checking it works

The BDD suite wires exactly this, against a real syslog-ng collector, in
[`Bdd/Targets/Common/BddTargetTlsSender_OpenSsl_PosixTcp.c`](../../../Bdd/Targets/Common/BddTargetTlsSender_OpenSsl_PosixTcp.c),
for both the server-authenticated and mutual-TLS cases. It is the reference to
read against your own wiring.

Failures report through the error handler rather than silently — install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you. A `CRITICAL` at create time means the stream fell
back to the Null object and nothing will be delivered.
