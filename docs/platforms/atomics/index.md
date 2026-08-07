# C11 atomics

`Platform/Atomics/` is a portable AtomicCounter built on C11 `<stdatomic.h>` — the
sequenceId source on any target with a C11 compiler, no OS dependency.

Fills the [AtomicCounter](../../api/structSolidSyslogAtomicCounter.md) role.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogStdAtomicCounter`](../../api/SolidSyslogStdAtomicCounter_8h.md) | atomic counter (`_Atomic uint32_t` CAS) |

## Requirements

A C11 compiler with `<stdatomic.h>`. Where a toolchain lacks it, the
[platform × capability matrix](../index.md) shows which other platforms fill
the AtomicCounter role.

## Security behaviour and obligations

### The sequence wraps, and a collector must expect it

Values run in `[1, 2^31 - 1]` and skip zero on wrap. A long-lived device will
reuse numbers, so collector-side gap detection has to treat wrap as ordinary
rather than as a discontinuity.

### It evidences loss, not origin

`sequenceId` is a plain counter, not a cryptographic construction. It shows that
a record is missing; it does not bind a record to the device that raised it, and
it can be reproduced by anything that can write records.
