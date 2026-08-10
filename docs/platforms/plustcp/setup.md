# FreeRTOS-Plus-TCP setup

Wiring the networking adapters. [FreeRTOS-Plus-TCP](index.md) covers what they
fill and what they leave to you; the config fields are documented on the
structs themselves.

## What to link

The stack is configured by a header you own, so the adapters compile inside
your target against your `FreeRTOSIPConfig.h`. This platform fills the network
role only, so select it alongside whichever platform supplies your mutex and
clock — the [capability matrix](../index.md) shows which platforms fill those:

```cmake
set(SOLIDSYSLOG_PLATFORMS "PlusTcp;<OsPrimitives>")
target_link_libraries(my_app PRIVATE SolidSyslog SolidSyslog::PlusTcp)
```

`<OsPrimitives>` is whichever platform the [capability matrix](../index.md)
says fills the Mutex and clock roles on your target — see
[naming your platforms](../../build-integration.md#cmake) for how the list is
read.

Bring the stack up and let it acquire an address before creating any sender.

Resolving the collector by name needs DNS compiled into the stack. If you give
the collector a numeric address instead, you need neither DNS nor the resolver
it backs — which is also the more predictable choice where the network cannot
be trusted.

## Wiring a sender

```c
struct SolidSyslogResolver* resolver = SolidSyslogPlusTcpResolver_Create();
struct SolidSyslogAddress*  address  = SolidSyslogPlusTcpAddress_Create();
struct SolidSyslogStream*   stream   = SolidSyslogPlusTcpTcpStream_Create(NULL);

static struct SolidSyslogStreamSenderConfig senderConfig;
senderConfig = (struct SolidSyslogStreamSenderConfig) {0};
senderConfig.Resolver = resolver;
senderConfig.Stream   = stream;
senderConfig.Address  = address;
senderConfig.Endpoint = MyEndpoint;
struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderConfig);
```

For UDP, build a `SolidSyslogPlusTcpDatagram` and a `SolidSyslogUdpSender`
instead. Passing `NULL` to the TCP stream takes the default connect budget.

## Sizing

The stack's socket and buffer limits are yours to set, and they have to cover
what this library creates alongside everything else using the network.
Under-sizing shows up as a failure to send rather than as a crash, so it is
worth counting the instances you create rather than discovering the limit in
the field.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the component
fell back to its Null object, and nothing will be delivered.
