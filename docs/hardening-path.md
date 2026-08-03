# Building up the protection you need

Neither the EU Cyber Resilience Act (CRA) nor IEC 62443 mandates syslog; instead they
state capabilities — auditable events, integrity, confidentiality, availability,
non-repudiation — and leave the realisation to you. IEC 62443 certifies systems rather
than components, and CRA obligations attach to the manufacturer and are scoped by the
product's documented intended use. Two devices running this library can need very
different things from it.

This page is a path, not a design to copy. Start from the system you already have, add
one capability at a time, and stop where your threat model, your requirements and your
resources say to stop.

## The worked integration

Every code snippet and every figure below comes from a worked integration, published in
full:

- [solid-syslog-example](https://github.com/cososo-ltd/solid-syslog-example) — consumed with CMake.
- [solid-syslog-example-make](https://github.com/cososo-ltd/solid-syslog-example-make) — the same integration, consumed with Make.

The baseline is constructed to carry the third-party platform code a real-world device
would already have — an RTOS, a network stack, and cryptography. A device that needs
SolidSyslog will already have a network stack, and if its threat analysis calls for
syslog over TLS it is likely to need TLS for other protocols too. The costs stated are
the costs of integrating SolidSyslog with that baseline.

The baseline runs FreeRTOS on a QEMU Cortex-M3 with lwIP, FatFs and Mbed TLS, built as
C11. The integration is coded twice — once with CMake, once with Make — so the build
process is shown in detail either way.

The same integration path applies to any combination of platforms. Every platform
dependency — network stack, TLS library, filesystem, OS primitives, clock — is injected,
so you bring what you already run.

**Read the figures as indications.** They are derived from the example code and rounded up.
What a stage costs on yours depends on your components, your configuration and tuning,
your compiler and its options, and what your application already links. The example
repositories carry the exact figures, the diff that produced each, and the reasoning
behind it.

## How to read this

The order of the stages is a logical way to integrate syslog in small steps, each one
adding further capability and security. It is not prescriptive, however: you may choose
to skip UDP and go straight to TCP, or integrate structured data earlier or later.

Each stage indicates what it costs to integrate SolidSyslog. That cost includes the
application code, the SolidSyslog components, and the incremental cost within platforms
already integrated. When TLS is added, for instance, we include the cost of an
additional TLS connection rather than the cost of the whole TLS library.

---

## Stage 1 — Link it in

This stage looks only at how to link SolidSyslog into your application, without calling
any of it. There are no resource costs; it is broken out for clarity.

| CMake | Makefile | IDE / manifest |
|---|---|---|
| Name your platforms in `SOLIDSYSLOG_PLATFORMS`, then link `SolidSyslog` plus a `SolidSyslog::<Platform>` target for each header-configured upstream | Set `SOLIDSYSLOG_PLATFORMS` and `include solidsyslog.mk`; it hands back source lists and include sets, and defines no rules | Configure with your platform list and build the `manifest` target; paste the generated file and include lists into your project |

For now you need only the core and a network [platform](platforms/index.md).

## Stage 2 — Make failures visible

Install the error handler with
[`SolidSyslog_SetErrorHandler`](api/SolidSyslogError_8h.md#function-solidsyslog_seterrorhandler)
before any other call into SolidSyslog. The handler reports many misconfiguration
errors, and can save significant time whilst integrating.

```c
SolidSyslog_SetErrorHandler(OnSyslogError, NULL);
```

This is not only an integration aid. The handler is the seam into your product's own
error and health reporting, and it stays valuable at run time: the sender raises an
edge-triggered warning when the collector becomes unreachable and a notice when delivery
recovers, and the ring buffer raises an error for a record too large to enqueue. Route
it wherever your device already routes faults.

For more detail see [error handling and severity](error-severity.md).

**Cost.** Flash ~400 B, RAM ~10 B.

## Stage 3 — Create the logger, wired to nothing

Create the logger with both collaborators absent, deliberately, and read what the
handler prints.

```c
struct SolidSyslogConfig config = {
    .Buffer = NULL,
    .Sender = NULL,
};

struct SolidSyslog* logger = SolidSyslog_Create(&config);
```

No `<Class>_Create` fails or returns `NULL` — a missing collaborator is substituted with
its Null object and reported — so the only evidence is what the handler says:

```text
[syslog] CRITICAL SolidSyslog bad-config (detail 1)
[syslog] CRITICAL SolidSyslog bad-config (detail 2)
```

Doing it in this order is the point. Wire everything at once and see nothing, and you
cannot tell a working logger from a silent one. Seeing the faults first, then watching
them go quiet as each collaborator arrives, is the difference between believing it works
and knowing.

A convention worth adopting now: `NULL` as a parameter means "not supplied" and is
reported, while a collaborator you have deliberately done without is passed as its Null
object. The library distinguishes the two, and so should anyone reading your wiring
later.

**Cost.** Flash ~650 B, RAM ~175 B.

## Stage 4 — A first record on the wire

This is the simplest configuration that sends a syslog message. Constructing the logger
with [`SolidSyslog_Create`](api/SolidSyslogConfig_8h.md#function-solidsyslog_create)
happens once, typically during device startup. Using `SolidSyslogPassthroughBuffer` with
a `SolidSyslogUdpSender` means that when
[`SolidSyslog_Log`](api/SolidSyslog_8h.md#function-solidsyslog_log) sends a message, it
is formatted immediately and passed straight to the `SolidSyslogUdpSender`, which sends
it over UDP from the same task.

```c
struct SolidSyslogUdpSenderConfig senderConfig = {
    .Resolver = SolidSyslogLwipRawResolver_Create(),
    .Datagram = SolidSyslogLwipRawDatagram_Create(),
    .Address  = SolidSyslogLwipRawAddress_Create(),
    .Endpoint = CollectorEndpoint,
};
struct SolidSyslogSender* sender = SolidSyslogUdpSender_Create(&senderConfig);

struct SolidSyslogConfig config = {
    .Buffer = SolidSyslogPassthroughBuffer_Create(sender),
    .Sender = sender,
    .Store  = SolidSyslogNullStore_Get(),
};

struct SolidSyslog* logger = SolidSyslog_Create(&config);
```

The logging itself is done as shown below. In this configuration SolidSyslog is **NOT**
reentrant and should only be called from a single task.

```c
struct SolidSyslogMessage message = {
    .Facility  = SOLIDSYSLOG_FACILITY_LOCAL0,
    .Severity  = SOLIDSYSLOG_SEVERITY_INFORMATIONAL,
    .MessageId = "BOOT",
    .Msg       = "device started",
};
SolidSyslog_Log(logger, &message);
```

What arrives is a valid RFC 5424 record any collector will parse:

```text
<134>1 - - - - BOOT - device started
```

Timestamp, hostname, app-name and process-id are the RFC's nil value. The record is
valid without them; filling them in is the next stage, with a cost of its own.

**When you need it.** Every device needs this much. The question is whether UDP is
enough: it drops records silently, and anyone on the path can read them. If either
matters, take stage 9 as well and treat UDP as a stepping stone.

**Cost.** Flash ~3.7k, RAM ~200 B.

## Stage 5 — Name it and time it

The record so far carries no timestamp and no device name. Fill the RFC 5424 header
fields from what the device already has: the clock it can read, the address on its
interface, and its own name.

```c
struct SolidSyslogConfig config = {
    /* ... */
    .Clock       = SyslogFields_Clock,
    .GetHostname = SyslogFields_Hostname,
    .GetAppName  = SyslogFields_AppName,
};
```

```text
<134>1 2026-07-31T18:49:13.360000Z 10.0.2.15 my-device - BOOT - device started
```

Two details are worth getting right. Zero the timestamp struct before filling it, so a
clock that cannot answer fails the library's validation and is emitted as the nil value
rather than as a wrong time. And where a device has no resolvable name, RFC 5424 §6.2.4
allows its address in the HOSTNAME field instead.

PROCID stays nil here — a bare-metal image has no process to identify — and so does
STRUCTURED-DATA, until the next stage.

**When you need it.** As soon as more than one device reports to the collector, or a
record's time will be relied on. Everything the later stages add — a sequence number,
the clock's quality, the device's own identity — builds on these fields rather than
replacing them.

**Cost.** Flash ~400 B, no RAM.

## Stage 6 — Number the records

Add the first structured-data element,
[`SolidSyslogMetaSd`](api/SolidSyslogMetaSd_8h.md), carrying a sequence number. Elements
are supplied to the logger as an array and read on every record, so they must outlive
the call that creates the logger.

```c
struct SolidSyslogMetaSdConfig metaConfig = {
    .Counter = SolidSyslogStdAtomicCounter_Create(),
};
sd[0] = SolidSyslogMetaSd_Create(&metaConfig);

struct SolidSyslogConfig config = {
    /* ... */
    .Sd      = sd,
    .SdCount = 1U,
};
```

```text
... BOOT [meta sequenceId="1"] device started
```

The sequence number is incremented once per record formatted, not once per record
delivered. A record that never arrives therefore leaves a gap in the sequence rather
than no trace at all, which is why it is worth adding before any buffering or storage
that could drop one. Instrument first, then introduce the failure mode.

Unlike a header field, an SD PARAM has no nil value: one that is unset is omitted
entirely rather than written as `-`.

The example uses `SolidSyslogStdAtomicCounter`. If your toolchain has no atomics you
must supply your own counter, observing the initialisation and roll-over constraints RFC
5424 §7.3.1 places on the field: it starts at one and never reports zero. If you log
from more than one task, that counter must increment atomically.

**When you need it.** If anyone needs to know that records have gone missing.

**Cost.** Flash ~950 B, RAM ~65 B.

## Stage 7 — Size the record

Set `SOLIDSYSLOG_MAX_MESSAGE_SIZE` to fit your own records rather than taking the
library's default. Anything longer is truncated rather than dropped. RFC 5424 §6.1 says
a receiver should accept 2048 octets; over UDP, RFC 5426 §3.2 guarantees only 480.

This is the first of several compile-time limits you can override, and the override has
to reach Core, the platform sources and your own code alike — see
[tunables](build-integration.md#tunables), which covers the mechanism and the way it is
most often got wrong.

The cost is all RAM, and not where it looks. The record is built on the stack of
whichever task calls `SolidSyslog_Log`, so the cap is the largest single demand the
formatter makes of that task — setting it is what moves the record there. In the example
the logging task was still at the RTOS minimum inherited from the baseline and had to
grow to hold it.

**Cost.** No flash. RAM ~1.5k, almost all of it stack depth on the logging task.

## Stage 8 — Decouple logging from sending

A [`SolidSyslogCircularBuffer`](api/SolidSyslogCircularBuffer_8h.md) between
`SolidSyslog_Log` and the sender, drained by a service task calling
[`SolidSyslog_Service`](api/SolidSyslog_8h.md#function-solidsyslog_service).
`SolidSyslog_Log` formats, enqueues and returns; the service task does the I/O.

```c
static uint8_t s_ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(SYSLOG_BUFFER_RECORDS)];

.Buffer = SolidSyslogCircularBuffer_Create(SolidSyslogFreeRtosMutex_Create(), s_ring, sizeof(s_ring)),
```

This separates logging an event from sending it, and that is the point.
`SolidSyslog_Log` becomes safe to call from any number of tasks, and potentially cheap
enough to call from the place the event actually happens rather than from somewhere
convenient later. Nothing that logs waits on the network.

**When you need it.** Once logging and sending are decoupled, the buffer has to absorb
however many events can be logged before it is next serviced. It also makes logging from
multiple tasks safe.

**Cost.** Flash ~750 B, RAM ~3.7k — the ring plus the service task's stack. You choose
the ring size, and stages 13 and 14 revisit it once the record has grown.

The mutex comes from your RTOS, so add its platform to the list from stage 1.

## Stage 9 — Detect loss where it happens

UDP to TCP, by putting a [`SolidSyslogStreamSender`](api/SolidSyslogStreamSender_8h.md)
over a TCP stream. The network retransmits rather than dropping, and a send fails when
the collector is gone instead of succeeding into a void. Records are framed by octet
count per RFC 6587.

```c
struct SolidSyslogStreamSenderConfig senderConfig = {
    .Resolver = SolidSyslogLwipRawResolver_Create(),
    .Stream   = SolidSyslogLwipRawTcpStream_Create(&tcpConfig),
    .Address  = SolidSyslogLwipRawAddress_Create(),
    .Endpoint = CollectorEndpoint,
};
struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderConfig);
```

With stage 6 this completes the loss story: the transport detects loss where it happens,
and the sequence number reveals afterwards anything the transport could not. It is also
what makes the delivery-failed and delivery-restored events from stage 2 meaningful.

**When you need it.** If the device must know that delivery is failing — to raise an
alarm, to fall back, to start storing. Over UDP it never finds out.

**Cost.** Flash ~550 B, RAM ~200 B.

> RFC 6587 is Historic, and the IESG recommends TLS (stage 16) over plain TCP for new
> deployments. Plain TCP is here for collectors you do not control, and as the step
> that gives storage somewhere to spool before cryptography arrives.

## Stage 10 — Say how far to trust the clock

Add [`SolidSyslogTimeQualitySd`](api/SolidSyslogTimeQualitySd_8h.md), and give `MetaSd`
an uptime source alongside its counter.

```c
struct SolidSyslogMetaSdConfig metaConfig = {
    .Counter      = SolidSyslogStdAtomicCounter_Create(),
    .GetSysUpTime = SolidSyslogFreeRtosSysUpTime_Get,   /* new */
};
sd[1] = SolidSyslogTimeQualitySd_Create(SyslogTimeQuality);
```

```text
... BOOT [meta sequenceId="1" sysUpTime="243"][timeQuality tzKnown="1" isSynced="0"] device started
```

Time quality states how far the clock can be trusted, which matters when comparing
events from different devices. Ideally every device on the network is time-synchronised;
this element tells the collector whether that is so, and how closely. The uptime
alongside it distinguishes a reboot from a counter wrap when the sequence restarts at
one.

It lands here, immediately before the store, for a reason. Until now a record reached
the collector shortly after it was raised, so its timestamp and its arrival time were
much the same. Store-and-forward breaks that: a record can arrive hours later. The
device should say what its clock is actually worth before that assumption goes.

**When you need it.** If events from this device will be ordered against events from
others, or if a record's timestamp will be relied on after a delay.

**Cost.** Flash ~300 B, RAM ~25 B.

## Stage 11 — Survive an outage

Spool to a [`SolidSyslogBlockStore`](api/SolidSyslogBlockStore_8h.md). The service task
drains the ring into storage and sends from there, so a failed send costs a retry rather
than the record.

```c
struct SolidSyslogBlockStoreConfig storeConfig = {
    .BlockDevice    = SolidSyslogFileBlockDevice_Create(SolidSyslogFatFsFile_Create(), SYSLOG_STORE_PREFIX, 0U),
    .MaxBlocks      = SYSLOG_STORE_BLOCKS,
    .DiscardPolicy  = SOLIDSYSLOG_DISCARD_POLICY_OLDEST,
    .SecurityPolicy = SolidSyslogCrc16Policy_Create(),
};
```

Three decisions come with it: how much you can store, which is capacity on the medium
rather than RAM; what happens when it fills — discard oldest, discard newest, or halt;
and whether you want warning before that point, via the capacity-threshold callback.

The checksum here is a checksum, not tamper-evidence. It catches a truncated write or
bit-rot; anyone who can edit a stored record can recompute it. What it buys is knowing a
record came back the way it went in, which is the prerequisite for spooling at all.
Making stored records tamper-*evident* is stage 17.

**When you need it.** What is your audit-loss budget? If the collector is unreachable
for an hour, is losing that hour acceptable? Must records survive a reboot?

**Cost.** Flash ~4k, RAM ~1.2k — handles and one record buffer, not capacity.
**Upstream:** files on your filesystem. Both the number of files and the size of each are
tunable.

## Stage 12 — Say who sent it

Name the device in the record with
[`SolidSyslogOriginSd`](api/SolidSyslogOriginSd_8h.md) — the software, its version, and
your enterprise number.

```c
struct SolidSyslogOriginSdConfig originConfig = {
    .Software     = SYSLOG_SOFTWARE,
    .SwVersion    = SYSLOG_SW_VERSION,
    .EnterpriseId = SYSLOG_ENTERPRISE_ID,
};
sd[3] = SolidSyslogOriginSd_Create(&originConfig);
```

This lands after storage, and the reason is the point. While records went straight out,
"who sent this" was implied by the connection they arrived on. Once records can replay
hours later that stops being true, and the record has to say so itself.

**When you need it.** If records will be correlated across devices, replayed after a
delay, or relayed through anything.

**Cost.** Flash ~400 B, RAM ~50 B.

> Enterprise number 32473 is reserved by RFC 5612 for documentation. A shipping
> product must use its own enterprise number, registered with IANA.

## Stage 13 — Raise the message cap

Three elements put the record at 245 bytes, but `sequenceId` and `sysUpTime` are both
32-bit counters. At full width the same record is 261 bytes — past a 256-byte cap and
into truncation. Double the cap to 512.

The cost accounts for itself exactly: the ring is eight records at the cap plus a
two-byte length prefix each, so it grows 2,048 bytes, and the store keeps one record
buffer that grows by 256.

**Cost.** Flash ~15 B, RAM ~2.3k.

## Stage 14 — Shrink the ring

The ring is sized in records, so doubling the cap doubled its cost. Four records is
enough to absorb a burst logged while the service task is sending; the store, not the
ring, is what holds a backlog.

Taken with stage 13 the pair costs ~250 bytes of RAM. The ring itself ends up slightly
smaller than before the cap moved — four records at 512 is less than eight at 256 — so
what is actually being paid for is the store's single record buffer, which follows the
cap and cannot be halved.

**Cost.** No flash. RAM ~2k **returned**.

## Stage 15 — State the device's own address

Add the `ip` PARAM to the origin element, sourced from the same interface address the
HOSTNAME field reports.

```c
struct SolidSyslogOriginSdConfig originConfig = {
    /* ... as stage 12 ... */
    .GetIpCount = SyslogOriginIpCount,
    .GetIpAt    = SyslogOriginIpAt,
};
```

A relay or NAT between the device and the collector rewrites the address the collector
observes. `ip` is what the device says about itself, and that survives the hop. The
PARAM is repeatable per RFC 5424 §7.2, so the library asks for a count and then one
value per index rather than taking a single string.

**When you need it.** If anything sits between the device and the collector — a relay,
a gateway, or NAT — and the source address the collector sees can no longer be trusted
to identify the device.

**Cost.** Flash ~400 B, no RAM.

## Stage 16 — Trust the channel

Wrap the byte stream in TLS — here
[`SolidSyslogMbedTlsStream`](api/SolidSyslogMbedTlsStream_8h.md), layered over the TCP
stream from stage 9. The device verifies the collector against a trust anchor it already
holds, so records can be read only by that collector and cannot be altered in transit.
The collector is authenticated to the device; the device is not yet authenticated to the
collector.

```c
struct SolidSyslogMbedTlsStreamConfig tlsConfig = {
    .Transport  = SolidSyslogLwipRawTcpStream_Create(&tcpConfig),
    .Sleep      = SyslogSleep,
    .Rng        = DeviceCertStore_Rng(),
    .CaChain    = DeviceCertStore_CaChain(),
    .ServerName = SYSLOG_COLLECTOR_HOST,
};

.Stream = SolidSyslogMbedTlsStream_Create(&tlsConfig),
```

**When you need it.** If the log path crosses a network you do not control, or if
someone reading records in transit would learn something they should not. Also if the
device needs to know it is talking to the real collector rather than to whatever
answered on that address.

**Cost.** Flash ~700 B. RAM ~28k on the example device — but only ~600 B of that is
the library; the rest is a second concurrent TLS session and the deeper stack the
handshake needs.

**Upstream is where the real cost lives.** If your device does not already run TLS,
the library and its trust material will dominate everything on this page — expect an
order of magnitude or more above the adapter that drives it, and read the footprint
guidance for the specific TLS library you are considering, because it varies enormously
with the features and ciphersuites you enable. If your device *already* holds a TLS
session for something else — a cloud connection, OTA, a vendor framework — you paid that
long ago, and this stage adds the adapter and a second session. That is the case the
worked integration measures, and the common one on a device with a reason to care about
audit logging.

## Stage 17 — Protect what is at rest

Replace the CRC-16 from stage 11 with a keyed HMAC. The checksum told you a record came
back the way it went in; the HMAC tells you nobody has changed it since. An edit made
without the key fails verification, so stored records become tamper-*evident* rather
than merely intact.

```c
struct SolidSyslogMbedTlsHmacSha256PolicyConfig hmacConfig = {.GetKey = SyslogStoreKey};

.SecurityPolicy = SolidSyslogMbedTlsHmacSha256Policy_Create(&hmacConfig),
```

Key custody, rotation and provisioning are yours; the library consumes a key you supply
and never stores one. See [at-rest cryptography](security/at-rest-cryptography.md).

**When you need it.** If an attacker could reach the medium — removable, unattended, or
stealable — and stored records must be provably unaltered.

**Cost.** Flash ~350 B, RAM ~20 B. **Upstream:** a crypto primitive, which your TLS
library already provides and has already linked if you took stage 16. Reaching for
at-rest protection *without* TLS is where this stage carries an upstream cost of its
own.

## Stage 18 — State the protection in force

A private structured-data element reports the transport in use and the at-rest policy
protecting stored records:

```text
[logPipeline@32473 transport="tls" atRest="hmac-sha256"]
```

The standard elements say what any device can say; a private one says what only your
product knows. A collector can use it to confirm a record really did arrive over TLS and
really was sealed at rest, and to alert on a device whose pipeline has weakened.

Those are the values in force at this stage. They change as the remaining stages land —
`transport="mtls"` at stage 19, `atRest="aes-256-gcm"` at stage 20 — which is the point:
derive both from the handles the device actually holds, not from what you intended to
configure. A credential that failed to load leaves the device less protected than its
configuration suggests, and an element claiming protection that is not in force is worse
than no element at all, because that claim is exactly what a collector is watching for.

**When you need it.** If a collector has to verify the protection a record travelled and
rested under, rather than assume it. It is also the cheapest way to detect a fleet
member whose pipeline has silently degraded.

**Cost.** Flash ~150 B, RAM negligible.

## Stage 19 — Prove which device sent it

Server-authenticated TLS proves the collector is genuine. It does not tell the collector
which device it is talking to: any client the collector will admit can connect, and the
device name in the record is a claim rather than a proof. Adding a client certificate
and its key makes the handshake mutual, so the receiver authenticates the device
cryptographically.

```c
struct SolidSyslogMbedTlsStreamConfig tlsConfig = {
    /* ... as stage 16 ... */
    .ClientCertChain = DeviceCertStore_ClientChain(),
    .ClientKey       = DeviceCertStore_ClientKey(),
};
```

Both must be set. Supplying one and not the other silently leaves the connection
server-authenticated rather than failing, which is exactly the weakening an auditor
would look for — which is why the pipeline element in stage 18 reports what is actually
in force rather than what was intended.

**What it authenticates is the TLS peer.** If the device connects to the collector
directly, that is the device. If anything terminates the connection in between — a
relay, a gateway, a broker — then the collector authenticates that hop, not the device
behind it, and the device's identity in the record is once again a claim. TLS protects
each hop; it does not carry provenance across one. On a control network with a relay
between the device and the SIEM, which is a common industrial shape, this stage buys you
an authenticated first hop rather than end-to-end attribution.

Where that matters, `OriginSd` from stages 12 and 15 is what the device says about
itself and survives the hop, and the collector's trust in it rests on the relay. The
[threat model](security/threat-model.md) states this limitation and the related replay
exposure in full.

**When you need it.** When the receiver has to authenticate the device rather than take
its word — a collector that requires client certificates, or a deployment where the
device reaches the collector directly and origin has to be provable. It is not a free
upgrade. Mutual TLS needs a certificate per device, somewhere safe to keep the private
key, and an issuing and revocation process behind both. A device that keeps its key in
readable flash gains the appearance of attribution without the substance. Where devices
are already identified at another layer, or where there is no device PKI to build on,
server-authenticated TLS is an honest place to stop.

**Cost.** Flash ~70 B, RAM ~2k. Presenting a certificate costs almost no code; the RAM
is the TLS library's working buffer growing to carry the client certificate through the
handshake. Note that the example device already holds a client certificate for its own
broker session, so this is the cost of *using* credentials, not of provisioning them — a
device reaching for mutual TLS from a standing start also has to store and parse them.

## Stage 20 — Encrypt what is at rest

Replace the HMAC from stage 17 with authenticated encryption. Tamper-evidence proves a
stored record was not altered; it does nothing to stop anyone reading it. Authenticated
encryption does both — the body is encrypted, the record header is authenticated as
associated data, and the nonce and tag travel in the trailer.

```c
struct SolidSyslogMbedTlsAesGcmPolicyConfig gcmConfig = {.GetKey = SyslogStoreKey, .Rng = rng};

.SecurityPolicy = SolidSyslogMbedTlsAesGcmPolicy_Create(&gcmConfig),
```

These are separate decisions, and the second is not implied by the first. A device that
only needs to prove records were not altered can stop at stage 17. The store key does
not change: its name says what it protects, not which algorithm protects it, so
escalating the policy needs no new key provisioned.

GCM needs a fresh nonce per record, so the policy takes the device's RNG as well as the
key. That is the only wiring difference from the HMAC policy.

**When you need it.** If a disk that leaves the device would give something away —
records naming users, addresses, process values, or anything else you would not publish.

**Cost.** Flash ~150 B, RAM negligible. Almost free if you took stage 16, because a
device negotiating a GCM ciphersuite for TLS has already linked the same primitive.

---

## Where to go next

- [Adding it to your build](build-integration.md): the build detail behind the nods on this page, plus the tunables.
- [Structured data](structured-data.md): authoring the evidence elements, and your own.
- [Error handling and severity](error-severity.md): reading the events from stage 2.
- [Compliance in one page](overview.md): what CRA and IEC 62443 ask of an audit-logging function.
- [IEC 62443 guide](iec62443.md) and the [RFC compliance matrix](rfc-compliance.md).
- [Threat model](security/threat-model.md): the division of responsibility this page assumes.
