# Authoring custom structured data

RFC 5424 lets a message carry **structured data** — one or more `SD-ELEMENT`s, each a
named bag of `PARAM="value"` pairs:

```
[example@32473 detail="Hello World"]
```

SolidSyslog ships the three standard elements (`meta`, `timeQuality`, `origin`). This guide
is for adding **your own** element — an access-control event, a device reading, a config
change — with the library's safe authoring API.

You write only the *content*. The library owns the *framing*: the brackets, the `SD-ID`
spelling, the `PARAM-NAME="..."` punctuation, and the RFC 5424 §6.3.3 value escaping. A
custom element therefore cannot produce malformed structured data, and a value can never
break out of its quotes — whatever bytes you hand it stay inside the `"..."`.

The complete worked example below lives at
[`Bdd/Targets/Common/BddTargetCustomSd.c`](../Bdd/Targets/Common/BddTargetCustomSd.c).

## The two writer types

Authoring uses two opaque, stack-transient writers — you never touch a raw buffer:

| Type | You get it from | What it does |
|---|---|---|
| `SolidSyslogSdElement` | handed to your `Format` callback | Opens/closes one element: `_Begin(name, enterprise)`, `_Param(name)`, `_End`. Owns the `SD-ID` and `PARAM-NAME` syntax. |
| `SolidSyslogSdValue` | `SolidSyslogSdElement_Param(...)` returns one | Writes one `PARAM` value: `_String`, `_BoundedString`, `_Uint32`. Applies the §6.3.3 escaping. |

A value producer is only ever handed a `SolidSyslogSdValue*` — it physically cannot open a
parameter or reach the framing.

## Implementing a custom SD

A custom SD is a `SolidSyslogStructuredData` — a one-function vtable. Implement `Format`,
which the library calls with an `SD-ELEMENT` writer when it builds a message:

```c
#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStructuredDataDefinition.h"

static void ExampleSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    (void) base; /* stateless here — see "Carrying data" below */

    SolidSyslogSdElement_Begin(element, "example", 32473U);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "detail"), "Hello World");
    SolidSyslogSdElement_End(element);
}

static struct SolidSyslogStructuredData exampleSd = {ExampleSd_Format};
```

`Begin` → `Param`/value → `End`. Emit as many parameters as you like between `Begin` and
`End`; emit more than one element by repeating the `Begin…End` cycle.

### Carrying instance or per-call data

`Format` receives the SD object itself as `base`, so a stateful SD reads its data through
`base`. Embed the vtable as the first member and downcast:

```c
struct ExampleSd
{
    struct SolidSyslogStructuredData base; /* must be first */
    const struct Event* event;            /* whatever your element reports */
};

static void ExampleSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    const struct ExampleSd* self = (const struct ExampleSd*) base;
    SolidSyslogSdElement_Begin(element, "example", 32473U);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "user"), self->event->User);
    SolidSyslogSdElement_End(element);
}
```

The library never allocates your SD — it lives in your storage (static, stack, or your own
pool). It only needs to stay valid for the duration of the log call.

## SD-IDs and enterprise numbers

`_Begin(name, enterpriseNumber)` builds the `SD-ID`:

- **Enterprise number `0`** → an IANA-registered name, emitted verbatim: `[meta …]`.
- **Non-zero** → a private `name@number`: `[example@32473 …]`. The number is your
  IANA Private Enterprise Number (PEN); `32473` is the reserved example/documentation PEN.

Register for a PEN through IANA before shipping a private `SD-ID`; until then `32473` is the
correct placeholder.

Keep your `SD-ID` and `PARAM-NAME`s within RFC 5424 §6.3.2 — printable US-ASCII excluding
`=`, `]`, `"`, and space. You write these as compile-time constants, so this is yours to get
right. The library bounds each name to 32 bytes and a `NULL` name suppresses the element (or
skips that parameter), but an otherwise-valid name is emitted as written.

## Registering your SD

There are two ways to attach a custom SD, and they compose — per-instance elements emit
first, then per-message ones.

**Per-instance — on every message.** Put it in the config `Sd[]` array at create time:

```c
struct SolidSyslogStructuredData* sds[] = {metaSd, &exampleSd};
struct SolidSyslogConfig config = { /* … */ .Sd = sds, .SdCount = 2 };
```

**Per-message — on one call.** Pass it to `SolidSyslog_LogWithSd`, which attaches the array
to just that message after the per-instance set:

```c
struct SolidSyslogStructuredData* sds[] = {&exampleSd};
SolidSyslog_LogWithSd(handle, &message, sds, 1);
```

`SolidSyslog_Log(handle, &message)` is exactly `SolidSyslog_LogWithSd(handle, &message, NULL, 0)`.
A `NULL` array entry is skipped, so you can leave conditional elements out without a
placeholder.

### Reentrancy

A stateful SD reads its data through `base` while `Format` runs. Because a log call formats
synchronously, the object only has to be valid across the call — but **don't share one
mutable SD object across threads that log concurrently**: if one thread repoints
`self->event` while another is mid-`Format`, the second thread emits the wrong data. Give
each call site its own SD instance, or keep the object immutable. A stateless SD (like the
first example) is never affected.

## Values and escaping

Write values with `SolidSyslogSdValue`:

- `_String(value, source)` — a NUL-terminated string.
- `_BoundedString(value, source, maxLength)` — at most `maxLength` bytes.
- `_Uint32(value, number)` — decimal digits.

The library applies the RFC 5424 §6.3.3 escaping for you (`"`, `\`, and `]` are
backslash-escaped) and validates UTF-8 — you pass the raw value and the receiver gets it
back unchanged. Output is bounded by the message buffer, so a value can never overrun it.

## What the library owns, and what you own

| The library owns | You own |
|---|---|
| Brackets, `SD-ID`, `PARAM-NAME="..."` punctuation | The element and parameter *names* |
| §6.3.3 value escaping + UTF-8 validation | The parameter *values* |
| Name length bound + `NULL`-name suppression | Valid name characters (§6.3.2) |
| Buffer bounding | Registering the SD (`Config.Sd[]` or `LogWithSd`), and its storage/lifetime |
