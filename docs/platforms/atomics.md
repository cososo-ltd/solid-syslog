# C11 atomics

`Platform/Atomics/` is a portable AtomicCounter built on C11 `<stdatomic.h>` — the
sequenceId source on any target with a C11 compiler, no OS dependency.

Fills the [AtomicCounter](../api/structSolidSyslogAtomicCounter.md) role.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogStdAtomicCounter`](../api/SolidSyslogStdAtomicCounter_8h.md) | atomic counter (`_Atomic uint32_t` CAS) |

## Requirements

A C11 compiler with `<stdatomic.h>`. Windows toolchains without it use
[`SolidSyslogWindowsAtomicCounter`](windows.md) instead.
