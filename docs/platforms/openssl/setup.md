# OpenSSL setup

Wiring `SolidSyslogOpenSslStream` so a `SolidSyslogStreamSender` delivers RFC 5425
syslog over TLS. The [TLS obligations](../../tls.md) page covers what any TLS
stream must do, [OpenSSL](index.md) what this one needs and where it falls short of that, and
the config fields are documented on the struct itself. This page is the wiring.

## What you need

OpenSSL 3.0 or later on the include and link path, and a platform supplying the
TCP stream underneath — the [capability matrix](../index.md) shows which fill
that role.

```cmake
set(SOLIDSYSLOG_PLATFORMS "OpenSsl;<Network>")
```

`<Network>` is whichever platform the [capability matrix](../index.md) says
fills that role on your target — see
[naming your platforms](../../build-integration.md#cmake) for how the list is
read.

OpenSSL is a stable system API rather than a header-configured upstream, so the
adapter compiles straight into `libSolidSyslog.a` and there is no separate
target to link. [Adding it to your build](../../build-integration.md) covers the
Make and IDE routes.

## The layering

TLS is a Stream wrapped around another Stream. The TLS adapter carries the
records; the transport underneath carries the bytes, and it can be any Stream.

```text
StreamSender → SolidSyslogOpenSslStream → your TCP stream → socket
```

The TLS stream **borrows** its transport. It may close it, but it never destroys
it: the transport is yours to create and to destroy, and it must stay valid
until `SolidSyslogOpenSslStream_Destroy`.

## Wiring it

```c
/* Your TCP stream and sleep, from the platform that supplies them. */
struct SolidSyslogStream* transport = CreateTcpStream();

static struct SolidSyslogOpenSslStreamConfig tlsConfig;
tlsConfig = (struct SolidSyslogOpenSslStreamConfig) {0};
tlsConfig.Transport = transport;
tlsConfig.Sleep = MySleep;                    /* required — no fallback */
tlsConfig.CaBundlePath = "/etc/ssl/collector-ca.pem";
tlsConfig.ServerName = "collector.example.net";

struct SolidSyslogStream* tls = SolidSyslogOpenSslStream_Create(&tlsConfig);
```

Zero-initialise the config before filling it.

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
senderConfig.Address  = CreateAddress();      /* your platform's Address */
senderConfig.Endpoint = GetEndpoint;
struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderConfig);
```

Tear down in reverse order: sender, address, TLS stream, then the transport you
created.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the stream fell
back to the Null object, and nothing will be delivered.
