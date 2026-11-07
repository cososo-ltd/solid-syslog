# Records the logger writes about itself

When the library loses a record, cannot deliver, stops, or refuses a collector's
certificate, it writes a syslog record of its own saying so, and that record travels
the same path as the application's: through the store during an outage, numbered by
`SolidSyslogMetaSd`, delivered when the link returns. A collector therefore learns
which audit-processing failure occurred, not only that records are missing. Reporting
is on by default and the device can turn it off.

The first half of this page specifies the record, for whoever documents the product
the library is built into or writes the rule that consumes it at the collector. The
second half is for the integrator: what reporting costs, how to turn it off, and what
cannot be reported.

## What arrives at the collector

A report is an RFC 5424 record with these fields fixed:

| Field | Value |
|---|---|
| PRI | Facility `5` (`syslog`: messages generated internally by the log system). Severity is the event's own, so PRI is `40` plus the severity: `42` for `CRITICAL`, `43` for `ERROR`, `44` for `WARNING`, `45` for `NOTICE` |
| TIMESTAMP, HOSTNAME, APP-NAME, PROCID | The logger's own, as on every record it formats |
| MSGID | The event's category name, for example `STORE_FROZEN` |
| STRUCTURED-DATA | The elements registered on the logger at `Create`, in their order, then the element below |
| MSG | `SolidSyslog event` |

The element the library adds is `solidsyslog@67478`, under the IANA Private Enterprise
Number registered to Cozens Software Solutions Limited:

```text
[solidsyslog@67478 source="BlockStore" category="STORE_FROZEN" detail="DEVICE_WRITE_FAILED"]
[solidsyslog@67478 source="FatFsFile" category="FILE_IO_FAILED" detail="OPEN_FAILED" native="12"]
```

| PARAM | Value | Present |
|---|---|---|
| `source` | The name of the class that raised the event, without the `SolidSyslog` prefix: `BlockStore`, `CircularBuffer`, `StreamSender`, `MbedTlsStream`, `FatFsFile`, and so on. `SolidSyslog` is the logger itself | Omitted when the event came from an integrator's own `SolidSyslog_Error` call with no named source |
| `category` | The category name, the same string as MSGID | Always |
| `detail` | The name of the raising class's detail code, for example `OPEN_FAILED`, `PEER_CERTIFICATE_UNTRUSTED`, `HELD_RECORD_DROPPED`. A code the class does not name is written as its decimal value instead | Always |
| `native` | The platform's own code for the fault - an errno, an `FRESULT`, a LittleFS error - as a signed decimal | Only when the platform gave one |

From the library's own sources every value is an enumerator name or an integer, so no
record content, path, key or address can appear in the element.

### The kinds reported

A category becomes a record when it is one of the kinds below, whichever class raised
it. The library chooses the set; the device chooses only whether reporting is on. The
severity each arrives at, and the rule for when it is raised, are on
[the severity policy page](error-severity.md#policy-by-category), where the
**Reported** column marks the same set.

| Category | `source` | What happened |
|---|---|---|
| `FILE_IO_FAILED` | the file class | A filesystem call failed; `native` carries its code |
| `SECURITY_POLICY_KEY_UNAVAILABLE` | the policy | The key for sealing or opening stored records was not available |
| `SECURITY_POLICY_SEAL_FAILED` | the policy | The cryptographic backend failed to seal a record |
| `SECURITY_POLICY_OPEN_FAILED` | the policy | The cryptographic backend failed to open a record |
| `TLS_STREAM_HANDSHAKE_FAILED` | the TLS stream | The collector's certificate, name or protocol was refused, or the handshake timed out |
| `BUFFER_BACKEND_FAILED` | the buffer | The buffer's mechanism failed for a reason other than being full |
| `BUFFER_RECORDS_LOST` | the buffer, or `SolidSyslog` | A full buffer lost records under the discard policy |
| `STORE_THRESHOLD_CROSSED` | `BlockStore` | The store's used bytes reached its capacity threshold |
| `STORE_RECORDS_LOST` | `BlockStore` | A full store lost records under the discard policy |
| `STORE_RECORD_DISCARDED` | `BlockStore` | A stored record failed verification, or was corrupt, and was discarded |
| `STORE_WRITE_FAILED` | `BlockStore` | A record could not be sealed and so was not stored |
| `STORE_FROZEN` | `BlockStore` | The store's device failed; the store will not use it again until restart |
| `SENDER_RECORDS_LOST` | the sender | The sender discarded a record: no sender was selected for it, or it could not be trimmed to a datagram the path accepts |
| `LOGGER_HALTED` | `SolidSyslog` | The logger stopped under the `HALT` discard policy |
| `SENDER_DELIVERY_FAILED` | the sender | Delivery to the collector failed; a run of failures starts |
| `SENDER_DELIVERY_RESTORED` | the sender | Delivery succeeded after a run of failures |

Every other category stays with the error handler alone. Setup faults
(`BAD_CONFIG`, `POOL_EXHAUSTED`), caller bugs (`BAD_ARGUMENT`, `UNKNOWN_DESTROY`),
and the connection, resolution and next-hop failures behind a delivery failure are not
reported as records.

### Reading the timestamp and the sequence number

A report is made by `SolidSyslog_Service`, and its TIMESTAMP is read when the report is
formatted, not when the fault occurred.

A report enters the stream after the record Service already held and before the
records still in the buffer, and its sequence number is assigned at that moment. Around
a report, sequence numbers are therefore not monotonic in arrival order: the report
carries a higher number than buffered records that arrive after it. Every number is
still used once, so a gap in `sequenceId` that no later arrival fills means a lost
record. A collector that
orders by `sequenceId` places the report after the records it overtook; one that reads
arrival order places it earlier. Both orders are consistent.

A kind that recurs before Service has reported it is reported once, with the most
recent event's detail and native code. The library reports that a kind of failure
happened, not how many times; the sequence number shows the collector how many records
went missing.

## What it means for the device

### Where a report is made

A report is formatted by `SolidSyslog_Service`, on the task that calls it, into the
record slot the logger already holds for the record it is moving. The task that met the
fault does no formatting and no sending: `SolidSyslog_Error` records the event after the
installed handler returns, and that record is a few word-sized stores. Nothing on the
failing path formats or sends, so a fault met inside a sender's `Send` is recorded
without re-entering the sender, and reporting is one level deep.

Each pass deals with the record it was holding, then reports one pending kind at a
time, causes before consequences, and then reads the buffer. A report bypasses the
buffer, so a full buffer never refuses the report of its own loss. From the moment it
is made, a report is an ordinary record: the store keeps it through an outage and across
a restart, the sender delivers it, and `SolidSyslogMetaSd` numbers it.
[`SolidSyslog_Service`](api/SolidSyslog_8h.md#function-solidsyslog_service) states what
happens to a report the store does not keep: it goes to the sender ahead of the stored
records and is held until the sender takes it.

Faults raised before `SolidSyslog_Create`, while the collaborators are being created,
are reported on the first pass of the logger that is then created. A boot-time fault in
the audit function reaches the collector.
[`SolidSyslog_Destroy`](api/SolidSyslogConfig_8h.md#function-solidsyslog_destroy) forgets
every event not yet reported.

### Turning it off

[`SolidSyslogConfig`](api/structSolidSyslogConfig.md) carries `IsEventReportingOn` and
`EventReportingContext`. The function is asked on the Service task each time Service is
about to make a report, so a device can turn reporting off and on while it runs. `NULL`
means on. An event turned down is not lost: it waits for a pass when the answer is true,
and a report already made is not withdrawn. The field's doc comment states what the
pass returns while reporting is off, and how a loop waiting on that return is woken.

Reporting off does not change what the error handler receives.

### What cannot be reported

- **While the logger is halted.** Under the `HALT` discard policy nothing moves, the
  halt included. `LOGGER_HALTED` is reported once the policy answers otherwise and
  Service resumes: after the record it was holding, after the kinds ahead of it in the
  table, and before the buffer. An error handler that stops the device on an `ERROR`
  stops it before the halt is reported, as
  [the category](api/SolidSyslogLoggerCategories_8h.md) describes.
- **Through a Null sender**, which is what `SolidSyslog_Create` wires on a missing
  sender, and what a sender's own `Create` returns on an exhausted pool. It takes every
  record, reports included, and delivers none; the `BAD_CONFIG` or `POOL_EXHAUSTED`
  event that put it there is not a reported kind, so only the error handler sees it.
- **After the device stops.** A report is made on the next Service pass. A fault that
  stops the device before that pass is in the error handler's hands alone.
- **From the fallback logger.** A `SolidSyslog_Create` that fell back to its shared
  null instance holds no record slot and makes no report. The events wait for a logger
  that can.
- **From a passthrough integration that never calls `SolidSyslog_Service`.** Reports
  are made only in Service. How a passthrough integration gets its reports out is
  [open](https://github.com/cososo-ltd/solid-syslog/issues/1135).

### Sizing the store for reports

A report is a record, so it occupies store capacity and counts toward the capacity
threshold. A store sized for the outage with a block to spare holds the reports of that
outage in the ordinary case, one per kind per run of the fault; the next paragraph
gives the case that produces more. A store already too small can lose the
application's records to the reports of the outage it is describing.

A threshold crossing can recur. A block sent or evicted re-arms it, so under the
`OLDEST` policy with the threshold inside the store's last block, each block that refills
crosses it again and writes one report. With blocks that hold a single record, the
report of a crossing can itself cross again while the store sits at the threshold. Size
blocks to hold several records.

A full store that refuses a report under its discard policy raises `STORE_RECORDS_LOST`
for it, and Service sends the report to the sender instead, or holds it while that
refusal has the logger halted; the store's event is raised, but the report is not lost.

### What it costs

Static RAM for the table of pending events: sixteen slots of 16 bytes on a 32-bit
target, 24 on a 64-bit host, shared by every logger in the build. On the Service task's
stack, the formatter's frame and the element while a report is formatted. The task
that calls `SolidSyslog_Log` needs no additional stack: recording an event is a few
stores. The record slot a report is formatted into is the one the logger already holds.

## Where this fits

- [Error handling and severity](error-severity.md): the severity each kind carries and
  the rule for when it is raised; the **Reported** column marks this set.
- [Structured data](structured-data.md): the element API the library's own element is
  built with, and the enterprise-number rules for yours.
- [IEC 62443](iec62443.md) and [CRA](cra.md): the controls this reporting serves.
