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
| `EMERGENCY` | — | no (reserved) |
| `ALERT` | — | no (reserved) |
| `CRITICAL` | The library cannot do its job here and the only fix is the engineer who built the device changing code or build (pool sizes, wiring, config structs). | yes |
| `ERROR` | A fault impacting delivery that needs a human, but is fixable at deploy/runtime by the operator or systems integrator without a code change (rejected cert, missing/short key, server unreachable). | yes |
| `WARNING` | Transient / self-healing, or delivered-but-degraded. | yes |
| `NOTICE` | Normal-but-significant: recovery from a down state. | yes |
| `INFORMATIONAL` | — | no (reserved) |
| `DEBUG` | — | no (reserved) |

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

`CRITICAL` vs `ERROR` is a *who fixes it, and how* distinction. `CRITICAL` is reserved for faults
only the engineer building the device can clear by changing code or build settings: a NULL
dependency, a pool sized too small, a wiring bug. `ERROR` is a fault a human must act on but which
the operator or systems integrator deploying the device can clear without touching code: a
rejected certificate, a missing or too-short key, an unreachable server. A missing key is provisioned
in the field, not designed in, so it is `ERROR`, not `CRITICAL`.

## Policy by category

| Category | Severity | Notes |
|---|---|---|
| `POOL_EXHAUSTED` | `CRITICAL` | always: `<Class>_Create` fell back to Null. Single-sourced via `SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY`. |
| `BAD_ARGUMENT` | `CRITICAL` | always: caller code bug. Single-sourced via `SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY`. |
| `BAD_CONFIG` - fatal | `CRITICAL` | `<Class>_Create` fell back to Null. Single-sourced via `SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY`. |
| `BAD_CONFIG` - degraded | `WARNING` | component still constructs and delivers (e.g. MetaSd without a counter, block-too-small, TLS chain-only). Emitted with an explicit `SOLIDSYSLOG_SEVERITY_WARNING` literal at the site, not the macro. |
| `BAD_CONFIG` - refused | `ERROR` | the component stands but this connection attempt fails on material or policy the deployment supplied (trust anchors that will not load, a malformed pin, nothing that authorises the peer). The sender retries on its next pass, so it clears when an operator fixes what was deployed. Explicit `SOLIDSYSLOG_SEVERITY_ERROR` at the site. |
| `UNKNOWN_DESTROY` | `WARNING` | benign lifecycle misuse: library keeps working. Single-sourced via `SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY`. |
| `STREAM_CONNECT_FAILED` - local | `ERROR` | the device could not obtain an endpoint, or its stack declined to start the attempt, so no packet was sent. It needs a human and will not clear by waiting. Single-sourced via `SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY`. |
| `STREAM_CONNECT_FAILED` - remote | `WARNING` | the destination did not answer, or answered with something other than a connection. The next Service pass retries. Single-sourced via `SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY`. |
| `TLS_STREAM_HANDSHAKE_FAILED` — rejected | `ERROR` | cert / protocol: a human must fix the peer or the cert. |
| `TLS_STREAM_HANDSHAKE_FAILED` — timeout | `WARNING` | transient: may clear on the next reconnect. |
| `TLS_STREAM_INIT_FAILED` | `ERROR` | setup fault needing a human; not split. |
| `SECURITY_POLICY_KEY_UNAVAILABLE` | `ERROR` | key too short / unavailable: provisioned in the field by the operator / systems integrator, fixable without a code change. |
| `SECURITY_POLICY_SEAL_FAILED` / `_OPEN_FAILED` | `ERROR` | runtime crypto operation failed. |
| `BUFFER_BACKEND_FAILED` | `ERROR` | message-queue backend fault; not split. |
| `RESOLVER_RESOLVE_FAILED` | `WARNING` | DNS may resolve on a later attempt. |
| `SENDER_DELIVERY_FAILED` | `WARNING` | destination outage: recoverable, store-and-forward covers it. |
| `SENDER_DELIVERY_RESTORED` | `NOTICE` | recovery. |

## Single-source severity macros

The universal-lifecycle categories pass their severity through a macro in
`SolidSyslogError.h` rather than a literal at each of the dozens of emit sites, so the policy
cannot drift site-by-site again. `STREAM_CONNECT_FAILED` gets the same treatment for the
same reason even though it is a split: every TCP backend raises the same shared detail
codes from `SolidSyslogTcpStreamErrors.h`, so a literal per site would restate one policy
once per backend. Two macros for two levels is not the `BAD_CONFIG` footgun below, which
is one macro standing for levels that differ. `BAD_CONFIG` is split: the fatal subset uses a macro, the
degraded subset keeps an explicit `WARNING` literal (the two are genuinely different
severities, so a single macro would be a footgun). Tests assert the concrete expected level
as a literal, never the macro, so a wrong policy value is caught.
