# C11 atomics setup

Wiring the sequence-number source. [C11 atomics](index.md) covers what it fills
and what it leaves to you.

## What to link

`<stdatomic.h>` is part of the language rather than a configured upstream, so
the adapter compiles straight into the static library:

```cmake
set(SOLIDSYSLOG_PLATFORMS "StdAtomic;<Network>;<OsPrimitives>")
```

This platform fills the AtomicCounter role only; the placeholders are whichever
platforms the [capability matrix](../index.md) says fill the rest of what your
build needs, and one you do not need comes out. See
[naming your platforms](../../build-integration.md#cmake) for how the list is
read.

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

An exhausted counter pool and an unconfigured counter are different failures,
and they are worth telling apart.

Exhaust the pool and `Create` hands back the Null counter, which returns 1 for
every record. The meta element is still emitted, so you still get `sysUpTime`
and `language`; only the sequence stops distinguishing records. Exhaustion is
reported through the error handler at `Create`, so it is visible rather than
something to discover later.

Leave the counter out of the meta element's config altogether and the element
itself does not build: `SolidSyslogMetaSd_Create` reports a `WARNING` and falls
back to the Null structured data, so no meta element is attached at all — no
`sequenceId`, and no `sysUpTime` either.

Both are safe in the sense that logging continues, but only the first still
carries the metadata.

## When it does not work

Failures report through the error handler rather than silently. Install one
before you start, and read [error severity](../../error-severity.md) for what
each level is telling you — a `CRITICAL` at create time means the counter fell
back to the Null object.
