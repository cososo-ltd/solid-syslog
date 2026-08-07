# C11 atomics setup

Wiring the sequence-number source. [C11 atomics](index.md) covers what it fills
and what it leaves to you.

## What to link

`<stdatomic.h>` is part of the language rather than a configured upstream, so
the adapter compiles straight into the static library:

```cmake
set(SOLIDSYSLOG_PLATFORMS "Atomics")
```

The compiler must support C11 atomics. Where a toolchain does not, the
[platform × capability matrix](../index.md) shows which other platforms fill
the same role.

## Wiring it

```c
struct SolidSyslogAtomicCounter* counter = SolidSyslogStdAtomicCounter_Create();
```

Hand the counter to the structured-data element that carries the sequence
number. Create takes no configuration; destroy it when the logger is torn down.

## Why it is worth wiring

The sequence number is what lets a collector notice that records are missing.
It is assigned when a record is raised rather than when it is sent, so a gap
reflects loss anywhere in the pipeline — the buffer, the store, or the
transport — not only on the wire.

Leave the role unfilled and the Null counter stands in, returning the same
value every time. Nothing then reports a problem, and gap detection quietly
proves nothing. Install an error handler so that pool exhaustion is visible
rather than silent.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the counter fell
back to the Null object.
