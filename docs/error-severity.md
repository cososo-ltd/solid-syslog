# Error-event severity policy

`SolidSyslog_Error(severity, source, category, detail)` carries a `severity` drawn from
`enum SolidSyslogSeverity` (`SolidSyslogPrival.h`). This document is the single source of
truth for which level an emit site picks. It exists because the ladder is easy to drift:
two faults can sit at the same level that an error handler needs to tell apart.

## The ladder is an *urgency* axis, not a *who-fixes-it* axis

Severity answers "how bad is this right now?", which is what an integrator's installed
handler reacts to (count it, light a GPIO, trip a watchdog, page someone). The orthogonal
"what kind of fault / who must fix it" question is already answered, losslessly, by the other
three axes of the event:

- `Source`: which class emitted it (pointer-identity).
- `Category`: the portable reaction axis (`SolidSyslogErrorCategory.h`).
- `Detail`: the per-class code.

So severity is free to mean urgency, and must not be overloaded to also encode "this is a
config bug." The category already says that.

## Levels

| Level | Meaning | Emitted today |
|---|---|---|
| `EMERGENCY` | - | no (reserved) |
| `ALERT` | - | no (reserved) |
| `CRITICAL` | The library cannot do its job here and the only fix is the engineer who built the device changing code or build (pool sizes, wiring, config structs). | yes |
| `ERROR` | A fault impacting delivery that persists until something deployed changes - a certificate, a key, the network - with no code or build change (rejected cert, missing/short key, a device that cannot open a socket). | yes |
| `WARNING` | Transient / self-healing, or delivered-but-degraded. | yes |
| `NOTICE` | Normal-but-significant: recovery from a down state. | yes |
| `INFORMATIONAL` | - | no (reserved) |
| `DEBUG` | - | no (reserved) |

`EMERGENCY`, `ALERT`, `INFORMATIONAL`, and `DEBUG` are deliberately unused, reserved for
integrator-defined use and possible future events.

## The discriminator: did `<Class>_Create` fall back to the Null object?

The `CRITICAL` / `WARNING` line for setup faults is mechanically checkable:

- `CRITICAL`: the component could not be built. `<Class>_Create` returned the shared Null
  sibling (pool exhausted, or a hard misconfig with no usable fallback). Also: a public-API
  call handed a NULL handle / argument, a caller code bug.
- `WARNING`: the component was built and is delivering, just degraded (soft
  misconfig), or the fault is an environmental / transient delivery condition that may
  clear on its own.

A degraded-but-delivering component stays `WARNING` even though its fix is a code change;
rating it `CRITICAL` would fire a handler's "everything is broken" reaction at a logger that
is, in fact, working. The `BAD_CONFIG` category already tells the integrator a code change is
needed.

`CRITICAL` vs `ERROR` is a *what has to change* distinction. `CRITICAL` is reserved for faults
that clear only with a change to code or build settings: a NULL dependency, a pool sized too
small, a wiring bug. `ERROR` is a fault that persists until something deployed changes, with no
code change: a rejected certificate, a missing or too-short key, an unreachable server. A missing key is provisioned
in the field, not designed in, so it is `ERROR`, not `CRITICAL`.

## When a record is lost

Every record the library loses shows at the collector as a gap in the sequence number,
once a later record arrives. The handler is there to say why, and four rules decide what it is told. The table under
[Policy by category](#policy-by-category) applies them category by category, and each
category's header doc comment is the home of its detail.

1. **A change of state is reported, not each occurrence.** A run of losses raises one
   event when it starts. Nothing more is raised on that path until it next succeeds
   without loss, which re-arms it. There is no matching "restored" event: the collector
   already sees how many records went missing.
2. **Bad data is discarded and the drain carries on. Bad media is not used again.** On
   media that is working, a stored record that fails verification is discarded on its
   own, a corrupt header costs the rest of its block, and the records after either are
   delivered. A block device
   operation that fails freezes the store, which reports once and does not touch the
   device again; delivery continues without store-and-forward.
3. **The faulty component reports the cause; the store or buffer reports the cost.** A
   security policy reports its key or its backend, a File or a stream the platform's
   native code, and the store or buffer the records that were lost.
4. **"Could not check" is not evidence of corruption.** A policy that cannot run on read
   leaves the record in the store to be tried again. It is never taken for a rejection.

A record a discard policy turns away is lost as configured, and the first rule still
applies: a full buffer, a full store under `NEWEST` or `HALT`, and `OLDEST` evicting a
block that still holds unsent records each raise one `WARNING` when the run starts.

## Policy by category

| Category | Severity | Notes |
|---|---|---|
| `POOL_EXHAUSTED` | `CRITICAL` | always: `<Class>_Create` fell back to Null. Single-sourced via `SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY`. |
| `BAD_ARGUMENT` | `CRITICAL` | always: caller code bug. Single-sourced via `SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY`. |
| `BAD_CONFIG` - fatal | `CRITICAL` | `<Class>_Create` fell back to Null. Single-sourced via `SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY`. |
| `BAD_CONFIG` - degraded | `WARNING` | component still constructs and delivers (e.g. MetaSd without a counter, block-too-small, TLS chain-only). Emitted with an explicit `SOLIDSYSLOG_SEVERITY_WARNING` literal at the site, not the macro. |
| `BAD_CONFIG` - refused | `ERROR` | the component stands but this connection attempt fails on material or policy the deployment supplied (trust anchors that will not load, a malformed pin, nothing that authorises the peer). The sender retries on its next pass, so it clears once what was deployed changes. Explicit `SOLIDSYSLOG_SEVERITY_ERROR` at the site. |
| `UNKNOWN_DESTROY` | `WARNING` | benign lifecycle misuse: library keeps working. Single-sourced via `SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY`. |
| `STREAM_CONNECT_FAILED` - local | `ERROR` | the device could not obtain an endpoint, or its stack declined to start the attempt, so no packet was sent. Waiting does not clear it. Single-sourced via `SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY`. |
| `STREAM_CONNECT_FAILED` - remote | `WARNING` | the destination did not answer, or answered with something other than a connection. The next Service pass retries. Single-sourced via `SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY`. |
| `DATAGRAM_NEXT_HOP_UNRESOLVED` | `WARNING` | the next hop did not answer address resolution within the wait, so the record was not handed to the stack. It clears when the next hop answers, and the next send tries again. Raised once per run of failures. Single-sourced via `SOLIDSYSLOG_DATAGRAM_NEXT_HOP_UNRESOLVED_SEVERITY`. |
| `STREAM_OPTION_REFUSED` | `WARNING` | the connection opened, but the stack declined a socket option set on it, so it is less robust than intended. Delivery continues, which is why this is not `ERROR`. Explicit `SOLIDSYSLOG_SEVERITY_WARNING` at the site. |
| `TLS_STREAM_HANDSHAKE_FAILED` - rejected | `ERROR` | cert / protocol: the attempt fails the same way until the peer or the certificate changes. |
| `TLS_STREAM_HANDSHAKE_FAILED` - timeout | `WARNING` | transient: may clear on the next reconnect. |
| `TLS_STREAM_INIT_FAILED` | `ERROR` | a TLS context, its defaults or its session would not initialise, so no connection can be attempted; not split. |
| `SECURITY_POLICY_KEY_UNAVAILABLE` | `ERROR` | key too short / unavailable: key material is provisioned with the deployment, so it changes without a code change. A record that could not be sealed is lost; one that could not be opened stays in the store and is tried again on the next pass. Raised once per run of faults, together with `_SEAL_FAILED` and `_OPEN_FAILED`, re-armed by the next seal or open that reaches a verdict. |
| `SECURITY_POLICY_SEAL_FAILED` / `_OPEN_FAILED` | `ERROR` | the crypto backend failed, with the same consequence for the record and in the same run as `_KEY_UNAVAILABLE`. Neither is a verdict on a record: one that fails verification is reported by the store as `STORE_RECORD_DISCARDED`. |
| `BUFFER_BACKEND_FAILED` | `ERROR` | the buffer's mechanism failed: a message-queue send or receive refused for a reason other than a full queue, or a head record too large for the reader. A failed send is reported once per run of failed sends, separately from a full queue, and re-armed by the next record the queue takes. |
| `BUFFER_RECORDS_LOST` | `WARNING` | a full buffer lost records under the discard policy, working as configured: it refused the record being written (`NEWEST`, `HALT`), or evicted records from its head to make room for it (`OLDEST`). It clears as Service drains the buffer. Raised once per run: a run of refusals is re-armed by the next record the buffer keeps, a run of evictions by the next write that evicts nothing. The logger raises it too, under its own source, when it drops the record it was holding for the sender because the buffer filled behind it under `OLDEST`; once per run, re-armed when a held record is dealt with. Explicit `SOLIDSYSLOG_SEVERITY_WARNING` at the site. |
| `RESOLVER_RESOLVE_FAILED` - transient | `WARNING` | DNS may resolve on a later attempt. |
| `RESOLVER_RESOLVE_FAILED` - unsupported family | `ERROR` | the lookup answered in a family the transports cannot send to. Permanent for that destination: it clears when the destination changes to one that resolves in a family the transports send to. Explicit `SOLIDSYSLOG_SEVERITY_ERROR` at the site. |
| `FILE_IO_FAILED` | `ERROR` | a file system call failed - read-only media, a full volume, a failing device - and the operation that made it failed with it. Single-sourced via `SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY`. |
| `STORE_WRITE_FAILED` | `ERROR` | the security policy would not seal the record being written, so it is lost. Each later write tries again. Raised once per run of such records, re-armed by the next record the store keeps. Explicit `SOLIDSYSLOG_SEVERITY_ERROR` at the site. |
| `STORE_FROZEN` | `ERROR` | the store's device failed, and the store will not use it again until it is created afresh, in practice a restart. Delivery continues without store-and-forward. Raised once. Explicit `SOLIDSYSLOG_SEVERITY_ERROR` at the site. |
| `STORE_RECORD_DISCARDED` | `ERROR` | a stored record was not delivered: its security policy rejected it, or it was corrupt and the rest of its block went with it. The records after it are delivered. Raised once per run of discarded records, re-armed by the next record that reads cleanly. Explicit `SOLIDSYSLOG_SEVERITY_ERROR` at the site. |
| `STORE_RECORDS_LOST` | `WARNING` | the discard policy lost records, working as configured: a full store refused the record being written (`NEWEST`, `HALT`), or evicted a block still holding unsent records (`OLDEST`). Raised once per run: a run of refusals is re-armed by the next record the store keeps, a run of evictions by the next rotation that evicts nothing unsent. Under `HALT` the logger stops at the refusal and writes nothing more, so it is raised once. Explicit `SOLIDSYSLOG_SEVERITY_WARNING` at the site. |
| `SENDER_DELIVERY_FAILED` | `WARNING` | destination outage: recoverable, store-and-forward covers it. |
| `SENDER_DELIVERY_RESTORED` | `NOTICE` | recovery. |

## Single-source severity macros

The universal-lifecycle categories pass their severity through a macro in
`SolidSyslogError.h` rather than a literal at each of the dozens of emit sites, so the policy
cannot drift site-by-site again. `STREAM_CONNECT_FAILED` gets the same treatment for the
same reason even though it is a split: every TCP backend raises the same shared detail
codes from `SolidSyslogTcpStreamErrors.h`, so a literal per site would restate one policy
once per backend. `FILE_IO_FAILED` has one for the same reason: its codes in
`SolidSyslogFileErrors.h` are shared by every File backend. Each of these
macros stands for one level. `BAD_CONFIG` is split: the fatal subset uses a macro, the
degraded subset keeps an explicit `WARNING` literal (the two are genuinely different
severities, so a single macro would be a footgun). Tests assert the concrete expected level
as a literal, never the macro, so a wrong policy value is caught.
