# MISRA C:2012 deviations

SolidSyslog is **MISRA-informed**, not certified-compliant. The project
adopts a curated subset of MISRA C:2012 rules per tier (see
`docs/NAMING.md` for the tier model). This document records every
deliberate deviation from a rule the project otherwise enforces.

Each deviation is paired with a matching entry in `misra_suppressions.txt`
(the cppcheck-misra input). The two files are complementary:

| File | Audience | Purpose |
|------|----------|---------|
| `misra_suppressions.txt` | cppcheck-misra | Machine-readable suppressions per rule / file / line |
| `docs/misra-deviations.md` | Reviewers, auditors, integrators | Why each deviation exists, with rationale, scope, approval |

Each entry in `misra_suppressions.txt` shall reference the section of
this document that authorises it. The suppressions file was populated
under [S10.06](https://github.com/DavidCozens/solid-syslog/issues/367)
after the rule subset was curated; before then it carried only a
header comment.

The format below is patterned on MISRA's own deviation record template
(MISRA Compliance:2020 §4.2).

---

## D.001 — Rule 5.1 external identifier uniqueness relaxed to 63 characters

### Rule

> **Rule 5.1 (Required)** — External identifiers shall be distinct.

The "distinct" requirement is parameterised by the implementation's
significant-character count for external identifiers. C99 §5.2.4.1
specifies a **minimum** of 31 significant characters in external
identifiers — i.e. a conforming compiler may treat two external
identifiers that agree in the first 31 characters as the same identifier.

### Deviation

SolidSyslog requires external identifiers to be distinct in the first
**63** characters rather than the first 31.

### Scope

- **Strict tier** — `Core/Interface/`, `Core/Source/`,
  `Platform/*/Interface/`
- **Pragmatic tier** — `Platform/*/Source/`

The deviation does not apply to the Consistency-only or Out-of-scope
tiers (rule 5.1 is not enforced there at all).

### Rationale

The C99 31-character limit is a legacy linker artifact from the late
1980s. Every toolchain that SolidSyslog targets — hosted or embedded —
supports external identifiers well in excess of 63 significant
characters:

| Toolchain | External identifier behaviour |
|-----------|-------------------------------|
| GCC (incl. `arm-none-eabi-gcc`)                | No compiler-imposed limit; identifier length is delegated to the target's linker, and all characters are significant on every linker SolidSyslog targets (ld, gold, lld, link.exe). See GCC manual, "Implementation-defined behavior". |
| Clang / LLVM (incl. Arm Compiler 6 / armclang) | Same rule as GCC for external identifiers — no compiler-imposed limit. |
| MSVC 2015+                                     | Documented maximum identifier length **2,047 characters** ([Microsoft Learn — C Identifiers](https://learn.microsoft.com/en-us/cpp/c-language/c-identifiers)). |
| IAR Embedded Workbench                         | C/C++ compiler reference manual documents an identifier limit well above 63 characters in every currently shipping version (verify on the target SKU's compiler reference for ports of SolidSyslog to non-standard SKUs). |

The Tier 1 naming scheme in `docs/NAMING.md` (form
`SolidSyslogClass_Function`) routinely produces identifiers in the
30–40 character range — `SolidSyslogFreeRtosStaticResolver_Create` is
40, `SolidSyslogFreeRtosTcpStream_Destroy` is 36 — and a few public
storage-size enums sit just below 40 (e.g.
`SOLIDSYSLOG_FREERTOSSTATICRESOLVER_SIZE`, 39). Strict 31-character
distinctness would either collapse identifier pairs that read
identically up to a trailing word
(`SolidSyslogFreeRtosStaticResolver_Create` vs `_Destroy`) into a
single name, or force unidiomatic abbreviation throughout the public
API. Neither outcome serves clarity or MISRA's underlying intent
("the reader can tell two identifiers apart"); 63 characters does.

63 was chosen rather than "unlimited" so the project still names a
concrete number that every targeted toolchain comfortably exceeds. It
also matches C99's separate 63-character minimum for **internal**
identifiers (§5.2.4.1) — a single number applies project-wide.

### Risk and mitigation

- **Portability** — Constrained to toolchains that support ≥ 63
  significant characters in external identifiers. The table above
  covers every supported target; adding a new target requires verifying
  this constraint.
- **Tooling** — cppcheck-misra applies its default 31-character
  window for rule 5.1. The deviation only matters when a real
  collision would resolve at 63 characters but not at 31 — at
  which point the project would suppress that specific finding
  with a rationale tying back to this section. Currently no rule
  5.1 collisions occur (0 findings on the current tree), so no
  cppcheck-misra configuration change is required; the
  enforcement window is strictly stricter than the deviation
  allows, which is the safe direction. (Decision recorded under
  [S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).)
- **Review** — The naming scheme itself (see `docs/NAMING.md`,
  Tier 1) builds in a `SolidSyslog` prefix and a `Class_Function`
  shape that makes accidental 63-character collisions extremely
  unlikely. The static-analysis gate exists to catch any that slip in.

### Approval

Project owner — David Cozens. Recorded as the founding entry in this
document under [S10.01](https://github.com/DavidCozens/solid-syslog/issues/357).

---

## D.002 — Rules 11.2 / 11.3 / 11.5: opaque-impl + caller-supplied-storage

### Rule

> **Rule 11.3 (Required)** — A cast shall not be performed between a
> pointer to object type and a pointer to a different object type.
>
> **Rule 11.2 (Required)** — Conversions shall not be performed between
> a pointer to an incomplete type and any other type.
>
> **Rule 11.5 (Advisory)** — A conversion should not be performed from
> pointer to `void` into pointer to object.

### Deviation

SolidSyslog accepts pointer conversions between caller-supplied storage
buffers and the implementation struct that owns the storage. The
conversion takes one of three shapes:

```c
struct SolidSyslogXImpl* impl = (struct SolidSyslogXImpl*) storage;
```

where `storage` is a `SolidSyslogXStorage*` (an opaque public type whose
size is exposed via `SOLIDSYSLOG_X_STORAGE_SIZE`), or where the
conversion goes via `void*` at vtable boundaries.

### Scope

- **Strict tier** — every `_Create` function in `Core/Source/` that
  uses the caller-supplied-storage pattern (≈30 classes).
- **Pragmatic tier** — every platform-specific `_Create` and `Address`
  helper under `Platform/*/Source/`.

### Rationale

The caller-supplied-storage pattern is foundational to SolidSyslog's
embedded-friendly design — every `_Create` takes a pointer to caller
memory of the size advertised by the matching `SOLIDSYSLOG_X_STORAGE_SIZE`
constant, places the implementation struct in that memory, and returns
a pointer of the opaque public type. The alternatives all conflict with
project constraints:

| Alternative | Why rejected |
|-------------|--------------|
| Dynamic allocation (`malloc`) | Not available on bare-metal, FreeRTOS-static-allocation, or DO-178C-style targets. SolidSyslog is callable from boot before any heap exists. |
| Public concrete types | Leaks the implementation through the public API; ties integrators to internal layout and breaks ABI stability. |
| Pass-by-value structs | Doubles the parameter footprint of every `_Create`; breaks the vtable indirection that decouples Core from Platform. |

`Address.h` (Strict tier, opaque `SolidSyslogAddress`) and every
caller-storage class (BlockStore, CircularBuffer, Formatter, the
FreeRTOS/Posix/Windows mutexes and streams, …) hit the same three
rules for the same structural reason — one deviation document
covers all of them.

### Risk and mitigation

- **Type safety** — Each `_Create` is the only place in the library
  that knows the relationship between `SolidSyslogXStorage` and
  `struct SolidSyslogXImpl`. A `_Static_assert` immediately below
  every impl definition pins the relationship at build time:

  ```c
  SOLIDSYSLOG_STATIC_ASSERT(
      sizeof(struct SolidSyslogX) <= sizeof(SolidSyslogXStorage),
      "SOLIDSYSLOG_X_STORAGE_SIZE is too small for struct SolidSyslogX"
  );
  ```

  An integrator who allocates undersized storage is caught at
  compile time, not at runtime.
- **Alignment** — The storage type is declared as
  `intptr_t storage[N]` (or a struct of the same shape), giving
  alignment at least as strict as any pointer or scalar the impl
  contains. The cast is therefore well-defined per §6.3.2.3.
- **Static analysis** — These rules are advisory (11.5) or required
  (11.2, 11.3). All 109 findings are suppressed via
  `misra_suppressions.txt` referencing this section. The pattern is
  reviewed once here, not per call site.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).

---

## D.003 — Rule 5.7: repeating struct tags (no-typedef-struct convention)

### Rule

> **Rule 5.7 (Required)** — A tag name shall be a unique identifier.

cppcheck-misra interprets this strictly — every repeated `struct X`
declaration counts as a non-unique tag, including forward declarations
in headers and the matching definition in source.

### Deviation

SolidSyslog uses `struct SolidSyslogX` directly throughout the public
API and source rather than typedef'ing it (see `docs/NAMING.md`, Tier 1
"No struct typedefs" rule). Each public class therefore necessarily
repeats its tag at every forward-declaration and definition site.

### Scope

- **Strict tier** — every public `struct SolidSyslogX` declared as an
  incomplete type in a header (`SolidSyslogBuffer.h`, `SolidSyslogStore.h`,
  `SolidSyslogFile.h`, etc.) and re-declared with full body in the
  matching source file.
- **Pragmatic tier** — same pattern across all `Platform/*/Source/`
  classes.

### Rationale

The no-typedef-struct convention serves two goals that survive
unchanged from C89 onwards:

1. **Discoverability at the call site.** A reader of
   `SolidSyslog_Create(struct SolidSyslogConfig*)` sees immediately
   that `SolidSyslogConfig` is a struct, not a typedef'd enum, integer,
   or function pointer. Tag-prefixed names act as a one-character type
   marker.
2. **Forward-declaration freedom.** A header that needs to mention
   `struct SolidSyslogX*` does not have to include the header that
   defines the typedef — it just forward-declares the struct. The
   alternative (typedef pulls in the body) creates header dependency
   cycles in the vtable-rich Core.

Both goals depend on the tag being identical in the forward declaration
and in the definition. The repetition is the convention, not a defect.

### Risk and mitigation

- **Genuine name collisions.** Distinct from this deviation: a real
  collision (two different `struct X` definitions with the same tag)
  is a code defect. Rule 5.7 surfaces those too, and the deviation
  scope is therefore limited to "repetition of the *same* tag across
  forward declaration and definition." Per-site review catches
  genuine collisions; the project's `SolidSyslog`-prefix convention
  makes them statistically unlikely.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).

---

## D.004 — Rule 18.4: pointer arithmetic on record buffers

### Rule

> **Rule 18.4 (Advisory)** — The `+`, `-`, `+=` and `-=` operators
> should not be applied to an expression of pointer type.

### Deviation

`Core/Source/RecordStore.c` traverses record buffers using
`uint8_t*` pointer arithmetic to step from the magic-byte header to
the length field to the message body.

### Scope

`Core/Source/RecordStore.c` only. Four call sites.

### Rationale

RecordStore implements a simple binary record layout
(`[magic][length][message]`) that has to be walked byte-by-byte during
read and written byte-by-byte during write. The natural C expression
is pointer arithmetic on `uint8_t*`:

```c
uint8_t* cursor = buffer;
cursor += MAGIC_SIZE;
const uint16_t length = ReadLengthAt(cursor);
cursor += LENGTH_SIZE;
/* cursor now points at the message body */
```

The alternative (array-index arithmetic against the base pointer,
`buffer[MAGIC_SIZE]`, `buffer[MAGIC_SIZE + LENGTH_SIZE]`, …) trades
visually-obvious offsets at the call site for repeated arithmetic on
the index; it produces the same compiled code with less-readable C.
The advisory rule discourages pointer arithmetic to protect against
out-of-bounds drift; the function-local nature of these cursors
(initialised, walked, dropped — never stored or returned) makes that
risk minimal.

### Risk and mitigation

- **Out-of-bounds walking.** Every `cursor += N` advance is paired
  with a bound check against the buffer's known total length. The
  unit tests in `Tests/RecordStoreTest.cpp` cover the boundary
  cases (zero-length record, max-length record, truncated input).
- **Future ports.** The deviation is RecordStore-local; other Core
  classes use array indexing throughout.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).

---

## D.005 — Rule 18.7: flexible array members

### Rule

> **Rule 18.7 (Required)** — Flexible array members shall not be
> declared.

### Deviation

`struct SolidSyslogFormatter` and `struct SolidSyslogCircularBuffer`
each end with a flexible array member that holds the caller-supplied
backing storage:

```c
struct SolidSyslogFormatter
{
    /* … bookkeeping … */
    char buffer[];
};
```

### Scope

Two classes only:

- `Core/Source/SolidSyslogFormatter.c`
- `Core/Source/SolidSyslogCircularBuffer.c`

### Rationale

These two classes implement the variable-size variant of the
caller-supplied-storage pattern (D.002). The integrator declares a
storage buffer of arbitrary size (with a minimum enforced by
`_Static_assert`), and the class lives inside that storage —
bookkeeping fields at the start, payload bytes filling the rest.

The flexible array member is C99's standard mechanism for exactly this
shape (§6.7.2.1 ¶18). The alternatives all regress:

| Alternative | Why rejected |
|-------------|--------------|
| Pointer to separately-allocated payload | Re-introduces dynamic allocation or a second storage parameter. |
| Fixed-size payload (`char buffer[MAX]`) | Forces every integrator to pay for the worst-case footprint. |
| Trailing `char buffer[1]` "struct hack" | Pre-C99 idiom, technically UB; flexible array members exist precisely because the hack was unsafe. |

### Risk and mitigation

- **Compiler support.** All target toolchains support C99 flexible
  array members (gcc, clang, MSVC 2013+, IAR, Keil ARMCC 6). The
  project's CI builds prove this on every push.
- **Allocation surprise.** The `_Static_assert` accompanying each
  flexible-array struct pins the storage-type-to-impl-type
  relationship at build time; an undersized storage allocation is a
  compile error.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).

---

## D.006 — Rule 1.4: emergent C language features (C11 `<stdatomic.h>`)

### Rule

> **Rule 1.4 (Required)** — Emergent language features, such as
> non-standard extensions and deprecated features, shall not be used.

cppcheck-misra's interpretation of "emergent" includes C11 features in
projects that otherwise target C99. SolidSyslog's `--std=c11` build flag
is set because two source files need `<stdatomic.h>`; the rest of the
codebase is C99.

### Deviation

`Platform/Atomics/Source/SolidSyslogStdAtomicU32.c` includes
`<stdatomic.h>` and uses `_Atomic uint32_t` plus
`atomic_compare_exchange_strong_explicit`.
`Platform/Windows/Source/SolidSyslogWindowsAtomicU32.c` is the
non-C11 sibling that the Atomics CMake selector switches in on
toolchains without `<stdatomic.h>`.

### Scope

Two source files:

- `Platform/Atomics/Source/SolidSyslogStdAtomicU32.c`
- `Platform/Windows/Source/SolidSyslogWindowsAtomicU32.c`

(The Windows file does not itself use C11 features — it triggers the
1.4 finding because the project compiles at `--std=c11` so cppcheck-misra
treats every TU as a "C11 project" candidate. The deviation is recorded
for both files to keep the suppression list complete.)

### Rationale

The atomic counter (`SolidSyslogAtomicCounter`) needs a portable
compare-and-swap primitive across hosted POSIX, hosted Windows, and
FreeRTOS targets. Three primitives are practical:

1. **`<stdatomic.h>`** — the C11 standard. Available on gcc/clang
   universally, on MSVC 2022+, and explicitly selected by the Atomics
   CMake module when `HAVE_STDATOMIC_H` is set.
2. **`InterlockedCompareExchange`** — Win32 API. Selected by the same
   CMake module when `HAVE_WINDOWS_INTERLOCKED` is set (legacy MSVC,
   pre-2022, and MinGW configurations without `<stdatomic.h>`).
3. **Hand-rolled CAS in assembly** — rejected: not portable, requires
   per-target maintenance.

The C11 primitive is the only option that is both portable across all
hosted targets and visible to the cppcheck-misra audit. The Atomics
module makes the choice at configure time; integrators on toolchains
without `<stdatomic.h>` automatically get the Win32 path with no
source-code change.

### Risk and mitigation

- **C99 baseline drift.** The project's C99 baseline is preserved
  everywhere except in these two TUs (and the macros that gate them).
  No C11 feature leaks into public headers.
- **Future C standards.** If C23's deprecation paths affect
  `<stdatomic.h>`, the Atomics module is the single migration point.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).

---

## D.007 — Rule 11.8: `const` qualification under field access of `const struct*`

### Rule

> **Rule 11.8 (Required)** — A cast shall not be performed that
> removes any `const` or `volatile` qualification from the type
> pointed to by a pointer.

### Deviation

Two distinct site categories trigger this rule:

1. **Field-access "false positive" (10 sites)** — reading a non-const
   pointer field through a `const struct*` parameter:

   ```c
   void SolidSyslog_Create(const struct SolidSyslogConfig* config)
   {
       InstallBuffer(config->buffer);  /* config->buffer has type
                                          struct SolidSyslogBuffer*, not
                                          const-qualified */
       …
   }
   ```

   Per C11 §6.5.2.3 ¶3, the result of `->` is the type of the named
   member; `config->buffer` evaluates to `struct SolidSyslogBuffer *`
   with no `const` qualifier on the pointed-to object. cppcheck-misra
   nonetheless flags the field access as a const-strip — it tracks the
   outer `const` on `*config` rather than the type of the member
   expression. The accepted interpretation in the MISRA community is
   that 11.8 applies to *pointer casts*, not to member-access yielding
   a non-`const`-qualified pointer rvalue.

2. **Platform-API const-strip (1 site)** —
   `Platform/Windows/Source/SolidSyslogWinsockTcpStream.c:82`:

   ```c
   return select(nfds, readfds, writefds, exceptfds, (struct timeval*) timeout);
   ```

   Winsock's `select()` declares its timeout parameter as
   `struct timeval*` (non-const) where POSIX `select()` declares it as
   `const struct timeval*`. The seam keeps the SolidSyslog side
   const-correct and forces the const-strip down to the platform-API
   boundary. The code carries a comment explaining the cast.

### Scope

- **Strict tier** — 10 field-access sites in `Core/Source/SolidSyslog.c`
  (`InstallBuffer` and siblings, 8 sites),
  `Core/Source/BlockSequence.c:438`, `Core/Source/SolidSyslogBlockStore.c:62`.
- **Pragmatic tier** — 1 site, `SolidSyslogWinsockTcpStream.c:82`.

### Rationale

The field-access sites are not genuine const violations under the C
standard; reorganising the code to avoid the cppcheck-misra
false-positive would either drop the outer `const` qualifier on
`*config` / `*blockSequence` / `*config` (the wrong direction) or
introduce a no-op `const_cast`-style explicit cast that the tool would
still flag. A site-local deviation is the honest record.

The Winsock site is the canonical example MISRA itself calls out
("forced by an external interface") in the deviation guidance for
rule 11.8. The Windows-side declaration is fixed by Microsoft; the
SolidSyslog seam keeps the const-correctness contract on the caller's
side of the boundary.

### Risk and mitigation

- **Genuine const-strip drift.** A new const-strip elsewhere in the
  codebase would surface as a fresh 11.8 finding, not be silently
  absorbed by the existing suppressions — the suppressions are
  line-specific.
- **Platform-API site.** The Winsock cast is the only one of its
  kind in the codebase and is documented at the call site.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).

---

## D.008 — Rule 21.10: transitive `<wchar.h>` via `<time.h>`

### Rule

> **Rule 21.10 (Required)** — The Standard Library time and date
> functions shall not be used. (cppcheck-misra also flags
> `<wchar.h>` inclusion under this rule.)

### Deviation

Three POSIX platform sources include `<time.h>` for `struct timespec`
and `clock_gettime`/`nanosleep`:

- `Platform/Posix/Source/SolidSyslogPosixClock.c`
- `Platform/Posix/Source/SolidSyslogPosixSleep.c`
- `Platform/Posix/Source/SolidSyslogPosixSysUpTime.c`

On glibc, `<time.h>` transitively includes `<wchar.h>` (via
`bits/types/struct_tm.h` and the `__wchar_t` family in `bits/types.h`).
cppcheck-misra reports the transitive inclusion as a direct 21.10
violation in each of the three TUs.

### Scope

`Platform/Posix/Source/` — three files. The deviation does not apply
to Windows or FreeRTOS sources, which use their own platform clocks
and do not include `<time.h>`.

### Rationale

The POSIX time and sleep wrappers exist precisely to provide
SolidSyslog's clock and sleep abstractions on POSIX targets. They
must include `<time.h>` to use `clock_gettime` / `nanosleep` /
`struct timespec`. None of the three files use any function or type
from `<wchar.h>`; the transitive inclusion is glibc-specific and
unavoidable on this platform.

### Risk and mitigation

- **Direct `<wchar.h>` use.** A future direct `#include <wchar.h>`
  in any of these files would not be absorbed by the per-file
  suppression — only line-1 `<time.h>` is suppressed.
- **Non-glibc POSIX targets.** musl, Bionic and BSDs do not pull
  `<wchar.h>` from `<time.h>`; the suppression is harmless on those
  targets (it suppresses a finding that does not occur).

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).

---

## D.009 — Rule 21.6: `<stdio.h>` for `SEEK_SET` / `SEEK_END` only

### Rule

> **Rule 21.6 (Required)** — The Standard Library input/output
> functions shall not be used.

### Deviation

`Platform/Windows/Source/SolidSyslogWindowsFile.c` includes `<stdio.h>`
solely to obtain the `SEEK_SET` and `SEEK_END` constants used by
`_lseeki64` (declared in `<io.h>`). No `<stdio.h>` function or type
(`FILE`, `fopen`, `printf`, …) is referenced.

### Scope

`Platform/Windows/Source/SolidSyslogWindowsFile.c` only. One line —
the `#include <stdio.h>` directive.

### Rationale

On MSVC, the `_lseeki64` function takes a "whence" parameter whose
constants (`SEEK_SET = 0`, `SEEK_CUR = 1`, `SEEK_END = 2`) are
defined exclusively in `<stdio.h>`. `<io.h>` declares `_lseeki64`
itself but does not define the constants; `<sys/stat.h>` does not
define them either. The three options considered:

| Option | Trade-off |
|--------|-----------|
| `#define SEEK_SET 0` ourselves | Hard-codes the MSVC ABI; fragile if the toolchain ever changes the values. |
| Inclusion of `<stdio.h>` | Pulls in the entire stdio API surface, but we use only two integer constants. |
| Pass numeric literals (`0`, `2`) | Loses readability at the call site. |

Inclusion of `<stdio.h>` is the lowest-risk option; the project's
banned-API policy already forbids `printf`/`scanf`/etc. and the
clang-tidy `bugprone-unsafe-functions` family catches accidental
use. The deviation is narrow and visible.

### Risk and mitigation

- **Accidental stdio use.** A grep over `WindowsFile.c` for
  `printf|scanf|fopen|FILE` proves the negative; CI's clang-tidy
  step catches future accidental use of banned stdio APIs across
  the whole tree.
- **Cross-platform consistency.** POSIX `SolidSyslogPosixFile.c`
  uses `<unistd.h>` for the same constants and does not need this
  deviation.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).

---

## D.010 — Rule 2.4: anonymous `enum` used as named-constant container

### Rule

> **Rule 2.4 (Advisory)** — A project should not contain unused tag
> declarations.

cppcheck-misra interprets an anonymous `enum { ... };` declaration
(no enum tag, no `typedef`) as a "tag declared but unused" — the
enumerators are used as named constants but the enum type itself
is never referenced.

### Deviation

SolidSyslog uses the anonymous-`enum` idiom across the codebase as
a portable mechanism for declaring named integer constants in
header and source scope:

```c
enum
{
    SOLIDSYSLOG_UDP_DEFAULT_PORT = 514, /* RFC 5426 */
    SOLIDSYSLOG_TCP_DEFAULT_PORT = 601, /* RFC 6587 §3.2 */
};
```

There are approximately 31 such declarations across `Core/` and
`Platform/*/Source/`. cppcheck-misra surfaces a subset (currently
6) under rule 2.4 once the rule 5.7 suppressions (D.003) take
effect; before then, the 5.7 check absorbs the same sites and
masks the 2.4 finding.

### Scope

- **Strict tier** — every anonymous-`enum` constants block in
  `Core/Interface/` and `Core/Source/`.
- **Pragmatic tier** — every anonymous-`enum` constants block in
  `Platform/*/Source/`.

### Rationale

`enum { CONST = N };` is the project's preferred way to introduce
named integer constants for two reasons:

1. **Type-safety vs. `#define`.** Enum constants are first-class
   integers with proper compile-time evaluation; `#define`
   constants are token substitutions and the project's clang-tidy
   `cppcoreguidelines-macro-usage` rule discourages them outside
   the very small surface of true preprocessor macros (e.g.
   `SOLIDSYSLOG_STATIC_ASSERT`, `SOLIDSYSLOG_X_STORAGE_SIZE`).
2. **Local scoping.** A block-scope `enum { ... };` introduces
   constants visible only inside the function or file, with no
   global namespace pollution.

The anonymous form (no tag) is correct because the enum *type*
is never needed — only the enumerator *values*. Adding a tag
solely to satisfy rule 2.4 would create an unused identifier
that would itself need a suppression, and the tag-named enum
would not be substitutable for any other type.

### Risk and mitigation

- **Constant collision.** Two enums declaring the same enumerator
  name collide at compile time (a duplicate-identifier error from
  the compiler). This deviation does not relax that compiler check.
- **Missing values.** A typo in a constant name is caught at
  compile time. The enum has no runtime cost.
- **Future tag-need.** If a particular constant set ever needs
  the enum *type* (e.g. to type a function parameter), the
  anonymous form is upgraded to a named-tag form on a per-case
  basis; the deviation does not preclude this.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/DavidCozens/solid-syslog/issues/367).
