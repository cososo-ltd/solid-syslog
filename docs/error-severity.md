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
| `SECURITY_POLICY_KEY_UNAVAILABLE` | `ERROR` | key too short / unavailable: key material is provisioned with the deployment, so it changes without a code change. |
| `SECURITY_POLICY_SEAL_FAILED` / `_OPEN_FAILED` | `ERROR` | runtime crypto operation failed. |
| `BUFFER_BACKEND_FAILED` | `ERROR` | message-queue backend fault, or a record too large for a circular buffer; not split. |
| `RESOLVER_RESOLVE_FAILED` - transient | `WARNING` | DNS may resolve on a later attempt. |
| `RESOLVER_RESOLVE_FAILED` - unsupported family | `ERROR` | the lookup answered in a family the transports cannot send to. Permanent for that destination: it clears when the destination changes to one that resolves in a family the transports send to. Explicit `SOLIDSYSLOG_SEVERITY_ERROR` at the site. |
| `NATIVE_ERROR` | the fault's | carries the platform's own code for the fault its Source raised just before it, at that fault's severity, so a handler filtering by severity keeps or drops the pair together. |
| `FILE_IO_FAILED` | `ERROR` | a file system call failed - read-only media, a full volume, a failing device - and the operation that made it failed with it. Single-sourced via `SOLIDSYSLOG_FILE_IO_FAILED_SEVERITY`. |
| `STORE_WRITE_FAILED` | `ERROR` | the record being written is lost: the device would not take it, or the security policy would not seal it. Each later write tries both again. A record the discard policy turns away raises nothing. Explicit `SOLIDSYSLOG_SEVERITY_ERROR` at the site. |
| `STORE_OPEN_FAILED` | `WARNING` | the store stands and tries the device again on the next write, which reports `STORE_WRITE_FAILED` if it is still failing. Explicit `SOLIDSYSLOG_SEVERITY_WARNING` at the site. |
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
