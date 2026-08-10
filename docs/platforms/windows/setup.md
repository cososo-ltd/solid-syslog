# Windows setup

Wiring the Win32 and Winsock adapters. [Windows](index.md) covers what they
fill and what they leave to you; the config fields are documented on the
structs themselves.

## What to link

Win32 and Winsock are stable system interfaces, so the adapters compile
straight into the static library and there is nothing extra to link:

```cmake
set(SOLIDSYSLOG_PLATFORMS "Windows")
```

## Initialise Winsock first

Winsock must be started before any sender is created, and stopped once every
socket is closed. The adapter does not do this for you: Winsock belongs to the
process, not to this library, and a process that already uses sockets has
started it for its own reasons. Startup and cleanup are reference-counted, so
match each successful `WSAStartup` with one `WSACleanup` and let the last one
out do the teardown:

```c
WSADATA wsaData;
(void) WSAStartup(MAKEWORD(2, 2), &wsaData);
/* ... create, use and destroy your senders ... */
(void) WSACleanup();
```

## Wiring a sender

```c
struct SolidSyslogResolver* resolver = SolidSyslogWinsockResolver_Create();
struct SolidSyslogAddress*  address  = SolidSyslogWinsockAddress_Create();
struct SolidSyslogStream*   stream   = SolidSyslogWinsockTcpStream_Create(NULL);

static struct SolidSyslogStreamSenderConfig senderConfig;
senderConfig = (struct SolidSyslogStreamSenderConfig) {0};
senderConfig.Resolver = resolver;
senderConfig.Stream   = stream;
senderConfig.Address  = address;
senderConfig.Endpoint = MyEndpoint;
struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderConfig);
```

For UDP, build a `SolidSyslogWinsockDatagram` and a `SolidSyslogUdpSender`
instead. Tear down in reverse order, before `WSACleanup`.

## The callbacks

`SolidSyslogConfig` takes the clock, hostname and process id as callbacks, and
this platform supplies one of each: `SolidSyslogWindows_GetTimestamp`,
`SolidSyslogWindows_GetHostname` and `SolidSyslogWindows_GetProcessId`.

## Threading

If your application logs from one thread and drains from another, put a
circular buffer between them with `SolidSyslogWindowsMutex` filling the mutex
role. This platform also fills the atomic counter role, so the sequence number
behind gap detection needs no separate component.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the component
fell back to its Null object, and nothing will be delivered.
