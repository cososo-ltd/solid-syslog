# Posix setup

Wiring the POSIX adapters. [Posix](index.md) covers what they fill and what
they leave to you; the config fields are documented on the structs themselves.

## What to link

POSIX is a stable system interface rather than a header-configured upstream, so
the adapters compile straight into `libSolidSyslog.a`. There is nothing extra to
link:

```cmake
set(SOLIDSYSLOG_PLATFORMS "Posix")
```

The one exception is the message-queue buffer, which needs POSIX message queues
— on glibc that means linking `rt`. If you use the circular buffer instead, you
do not need it.

## Wiring a sender

```c
struct SolidSyslogResolver* resolver = SolidSyslogGetAddrInfoResolver_Create();
struct SolidSyslogAddress*  address  = SolidSyslogPosixAddress_Create();
struct SolidSyslogStream*   stream   = SolidSyslogPosixTcpStream_Create(NULL);

static struct SolidSyslogStreamSenderConfig senderConfig;
senderConfig = (struct SolidSyslogStreamSenderConfig) {0};
senderConfig.Resolver = resolver;
senderConfig.Stream   = stream;
senderConfig.Address  = address;
senderConfig.Endpoint = MyEndpoint;
struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderConfig);
```

For UDP, build a `SolidSyslogPosixDatagram` and a `SolidSyslogUdpSender`
instead; the resolver and address are the same. Passing `NULL` to the TCP
stream takes the default connect budget — supply a config to override it per
instance.

Tear down in reverse order, and destroy everything you created.

## The callbacks

`SolidSyslogConfig` takes the clock, hostname and process id as callbacks
rather than components, and this platform supplies one of each ready to use:
`SolidSyslogPosixClock_GetTimestamp`, `SolidSyslogPosixHostname` and
`SolidSyslogPosixProcessId`. Use them directly, or wrap your own if the values
should come from somewhere other than the operating system.

## Threading

If your application logs from one thread and drains from another, put a
circular buffer between them with `SolidSyslogPosixMutex` filling the mutex
role. Leaving the mutex unfilled is safe only while `Log` and `Service` run on
the same thread.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the component
fell back to its Null object, and nothing will be delivered.
