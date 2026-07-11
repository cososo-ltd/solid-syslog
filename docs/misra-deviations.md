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
under [S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367)
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
30–40 character range — `SolidSyslogPlusTcpResolver_Create` is
40, `SolidSyslogPlusTcpTcpStream_Destroy` is 36 — and a few public
storage-size enums sit just below 40 (e.g.
`SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE`, 40). Strict 31-character
distinctness would either collapse identifier pairs that read
identically up to a trailing word
(`SolidSyslogPlusTcpResolver_Create` vs `_Destroy`) into a
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
  [S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).)
- **Review** — The naming scheme itself (see `docs/NAMING.md`,
  Tier 1) builds in a `SolidSyslog` prefix and a `Class_Function`
  shape that makes accidental 63-character collisions extremely
  unlikely. The static-analysis gate exists to catch any that slip in.

### Approval

Project owner — David Cozens. Recorded as the founding entry in this
document under [S10.01](https://github.com/cososo-ltd/solid-syslog/issues/357).

---

## D.002 — Rules 11.2 / 11.3 / 11.5: vtable downcasts + Formatter

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

SolidSyslog accepts two structural pointer conversions that are
identified in code as `SelfFromBase` (vtable) or `(struct X*) storage`
(Formatter). Both are reviewed once here, not per call site.

#### (a) Vtable / opaque-handle downcasts — every pool-allocated class

Every implementation class that participates in a vtable interface
(`SolidSyslogBuffer`, `SolidSyslogSender`, `SolidSyslogStream`,
`SolidSyslogDatagram`, `SolidSyslogStore`, `SolidSyslogMutex`,
`SolidSyslogFile`, `SolidSyslogBlockDevice`, `SolidSyslogAtomicCounter`,
`SolidSyslogResolver`, `SolidSyslogStructuredData`,
`SolidSyslogSecurityPolicy`) carries a `static inline ... *SelfFromBase(...)`
helper that downcasts the public base pointer back to the concrete
implementation struct so vtable methods can reach their own state:

```c
static inline struct SolidSyslogCircularBuffer*
CircularBuffer_SelfFromBase(struct SolidSyslogBuffer* base)
{
    return (struct SolidSyslogCircularBuffer*) base;
}
```

The same structural cast applies to `SolidSyslogAddress` — a pool-
allocated handle whose `struct SolidSyslogAddress` is an incomplete
public type, fully defined per platform as
`struct SolidSyslog{Posix,Winsock,FreeRtos}Address`. Each platform's
`*AddressPrivate.h` carries downcast accessors
(`SolidSyslog<Plat>Address_AsSockaddrIn` / `_AsConstSockaddrIn` /
`_AsFreertosSockaddr` / `_AsConstFreertosSockaddr`) plus a
`HandleFromIndex(size_t)` helper in `*AddressStatic.c` that converts a
pool slot index back to the public handle type. Rule 11.3 fires on
every such cast.

This is the standard OO-in-C "interface pointer back to derived
implementation" cast.

#### (b) `SolidSyslogFormatter` — variable-size stack builder

`SolidSyslogFormatter` is a transient stack-built builder whose backing
storage is sized at the call site via the
`SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(n)` macro. Variable-size means it
cannot fit the fixed-pool pattern used elsewhere in the library — its
lifecycle is fundamentally per-call, not per-class. Rules 11.2 / 11.3
fire on the cast between `SolidSyslogFormatterStorage*` and `struct
SolidSyslogFormatter*`.

#### (c) Third-party callback `void*` arg — `SelfFromArg` and byte-buffer reinterprets

Several wrapped libraries expose callback-style APIs where we register a
function pointer plus an opaque `void*` context that the library passes
back to us when it invokes the callback. mbedTLS's `mbedtls_ssl_set_bio`
hands us back `void* ctx` in `BioSend` / `BioRecv`; lwIP Raw's
`tcp_arg(pcb, self)` hands us back `void* arg` in every `tcp_recv` /
`tcp_err` / `tcp_connected` / `tcp_sent` callback we registered. The
implementation has to cast that `void*` back to the concrete
implementation struct to do any work — Rule 11.5 (advisory) fires on
every such cast.

This is structurally the same OO-in-C downcast as (a) — the library API
is the "base" type (`void*`), our struct is the "derived" type — just
happening at the callback boundary rather than the vtable-method
boundary. Each affected wrapper concentrates the cast in a single
`SelfFromArg`-style helper so the suppression has one site per class,
not one per callback (see `LwipRawTcpStream_SelfFromArg` for the
canonical shape).

A second 11.5 sub-case the library leans on is the
`void* ↔ const uint8_t*` cast in `SolidSyslogUdpSender` when trimming
codepoint-boundary bytes from the caller's `const void*` buffer — a
byte-buffer reinterpretation that crosses the same advisory rule. The
third-party API contract (the public `Send` / `SendTo` interface) is
`void*` for opacity; the byte-level work needs a concrete unit type.

### Rationale

The pool migration under E11 / E24 retired the caller-supplied-storage
pattern for every class that has a Create/Destroy lifecycle, leaving
only the vtable / opaque-handle downcast (which is required by the
OO-in-C interface decoupling) and the one non-pool exception above
(Formatter as a per-call builder). Both would otherwise require either
dynamic allocation (not available on bare-metal / FreeRTOS-static-
allocation / DO-178C-style targets — the library is callable from
boot before any heap exists) or leaking the implementation struct
through the public API (breaks ABI stability and the embedded-friendly
opaque-type design).

### Risk and mitigation

- **Type safety** — For (b) Formatter, a `_Static_assert` immediately
  below the impl definition pins the relationship between the public
  storage type and the private impl struct at build time. An
  integrator who allocates undersized storage is caught at compile
  time. For (a) vtable / opaque-handle downcasts, type safety is
  enforced by the contract that vtable methods are only called via the
  vtable installed in their own `SelfFromBase`-aware implementation;
  the per-platform Address downcast is similarly locked down because
  the pool slot is statically-typed `struct SolidSyslog<Plat>Address`,
  so the cast back from the opaque `struct SolidSyslogAddress*` cannot
  lie. For (c) callback `void*` args, the pointer that goes out via
  the registration call (e.g. `tcp_arg(pcb, self)`,
  `mbedtls_ssl_set_bio(..., self, ...)`) is the same pointer that
  comes back — the library is a pass-through; the cast can only
  succeed against the type the wrapper passed in.
- **Alignment** — Storage types are declared as `intptr_t storage[N]`
  (or a struct of the same shape), giving alignment at least as strict
  as any pointer or scalar the impl contains. The cast is therefore
  well-defined per §6.3.2.3.
- **Static analysis** — These rules are advisory (11.5) or required
  (11.2, 11.3). All current findings are suppressed via
  `misra_suppressions.txt` referencing this section. The pattern is
  reviewed once here, not per call site.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367); scope
narrowed under
[S11.11](https://github.com/cososo-ltd/solid-syslog/issues/414) once
every Create-lifecycle class moved off caller-supplied storage onto the
pool allocator; further narrowed under
[S24.07](https://github.com/cososo-ltd/solid-syslog/issues/418) once
Address itself moved onto per-platform pool classes — the casts are
now the same OO-in-C downcast that authorised (a), not a separate
caller-supplied-storage exception.

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
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.005 — Rule 18.7: flexible array members

### Rule

> **Rule 18.7 (Required)** — Flexible array members shall not be
> declared.

### Deviation

`struct SolidSyslogFormatter` ends with a flexible array member that
holds the caller-supplied backing storage:

```c
struct SolidSyslogFormatter
{
    /* … bookkeeping … */
    char buffer[];
};
```

### Scope

One class only:

- `Core/Source/SolidSyslogFormatter.c`

`SolidSyslogCircularBuffer` used to share this shape but moved off
it under E11 (S11.01) — its instance struct now holds an external
ring pointer rather than a trailing FAM.

### Rationale

The Formatter implements the variable-size variant of the
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
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.006 — Rule 11.8: `const` qualification under field access of `const struct*`

### Rule

> **Rule 11.8 (Required)** — A cast shall not be performed that
> removes any `const` or `volatile` qualification from the type
> pointed to by a pointer.

### Deviation

Two distinct site categories trigger this rule:

1. **Field-access "false positive" (15 sites)** — reading a non-const
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

   The same pattern recurs in
   `SolidSyslogMessageFormatter_Format(const struct
   SolidSyslogMessageFormatterContext* context)` (5 sites — `context->Clock`,
   `GetHostname`, `GetAppName`, `GetProcessId`, `Sd`), which reads its
   read-only context exactly as `_Create` reads its config. The `const`
   is deliberate (the formatter must not mutate the context); keeping it
   and accepting the false positive is preferred over weakening the
   signature to silence the tool.

2. **Platform-API const-strip (2 sites)** —

   **(a)** `Platform/Windows/Source/SolidSyslogWinsockTcpStream.c`:

   ```c
   return select(nfds, readfds, writefds, exceptfds, (struct timeval*) timeout);
   ```

   Winsock's `select()` declares its timeout parameter as
   `struct timeval*` (non-const) where POSIX `select()` declares it as
   `const struct timeval*`. The seam keeps the SolidSyslog side
   const-correct and forces the const-strip down to the platform-API
   boundary. The code carries a comment explaining the cast.

   **(b)** `Platform/LwipRaw/Source/SolidSyslogLwipRawDatagram.c`:

   ```c
   /* SolidSyslogDatagram_SendTo's buffer is const void*; lwIP's
    * pbuf->payload is plain void*. Assignment forces the const-strip. */
   p->payload = (void*) buffer;
   ```

   `SolidSyslogDatagram_SendTo` takes the caller's buffer as
   `const void*` — the contract is read-only inside the library.
   lwIP's `struct pbuf::payload` is declared `void*` (no `const`
   variant in the lwIP headers); `udp_sendto` only reads the
   payload (the PBUF_REF zero-copy contract — see handoff design
   decision #1) but the field type does not encode that. Assigning
   our `const void*` parameter to lwIP's `void*` field strips the
   qualifier at the platform-API boundary, same shape as the
   Winsock `select()` site above. Alternatives considered and
   rejected: changing the public `SendTo` signature to `void*`
   (breaks const-correctness for every other backend); copying
   the buffer to PBUF_RAM (defeats the zero-copy point of PBUF_REF
   and doubles per-send pool pressure).

### Scope

- **Strict tier** — 15 field-access sites: 8 in `Core/Source/SolidSyslog.c`
  (the `SolidSyslog_Install*` functions reading `config->` pointer
  fields), 5 in `Core/Source/SolidSyslogMessageFormatter.c`
  (`SolidSyslogMessageFormatter_Format` reading `context->Clock`,
  `GetHostname`, `GetAppName`, `GetProcessId`, `Sd`), 1 in
  `Core/Source/BlockSequence.c`
  (`BlockSequence_IsReadBlockFullyDrained` passing
  `blockSequence->BlockDevice` to `SolidSyslogBlockDevice_Size`), and
  1 in `Core/Source/SolidSyslogBlockStoreStatic.c`
  (`BlockStore_ResolveSecurityPolicy` accepting
  `config->SecurityPolicy`).
- **Pragmatic tier** — 2 sites: 1 in
  `Platform/Windows/Source/SolidSyslogWinsockTcpStream.c` (the
  `select()` timeout cast); 1 in
  `Platform/LwipRaw/Source/SolidSyslogLwipRawDatagram.c` (the
  lwIP `pbuf->payload` field cast).

### Rationale

The field-access sites are not genuine const violations under the C
standard; reorganising the code to avoid the cppcheck-misra
false-positive would either drop the outer `const` qualifier on
`*config` / `*blockSequence` / `*config` (the wrong direction) or
introduce a no-op `const_cast`-style explicit cast that the tool would
still flag. A site-local deviation is the honest record.

The two platform-API sites are the canonical "forced by an external
interface" pattern MISRA itself calls out in the deviation guidance for
rule 11.8. Both upstream declarations (Microsoft's `select()` timeout,
lwIP's `pbuf::payload`) are fixed by their vendors; the SolidSyslog seam
keeps the const-correctness contract on the caller's side of the
boundary.

### Risk and mitigation

- **Genuine const-strip drift.** A new const-strip elsewhere in the
  codebase would surface as a fresh 11.8 finding, not be silently
  absorbed by the existing suppressions — the suppressions are
  line-specific.
- **Platform-API sites.** Both the Winsock and lwIP casts are
  documented at the call site and listed individually here; any new
  const-strip at a platform boundary surfaces as a fresh 11.8 finding
  rather than being absorbed by glob.

### Approval

Project owner — David Cozens. Recorded under
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.007 — Rule 21.10: transitive `<wchar.h>` via `<time.h>`

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
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.008 — Rule 21.6: `<stdio.h>` for `SEEK_SET` / `SEEK_END` only

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
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.009 — Rules 2.4 / 5.7: anonymous `enum` used as named-constant container

### Rules

> **Rule 2.4 (Advisory)** — A project should not contain unused tag
> declarations.
> **Rule 5.7 (Required)** — A tag name shall be a unique identifier.

cppcheck-misra interprets an anonymous `enum { ... };` declaration
(no enum tag, no `typedef`) two ways:

- as a "tag declared but unused" (2.4) — the enumerators are used
  as named constants but the enum type itself is never referenced;
- as a non-unique tag (5.7) — every anonymous `enum` shares the
  same empty tag identifier, so the second and subsequent ones
  collide.

Both findings originate from the same syntactic shape — the
anonymous-`enum` named-constant idiom — and are covered by a single
deviation here.

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
`Platform/*/Source/`. Adding inline-suppress comments at every
site would add visual noise next to a project-wide intentional
idiom — listing them in `misra_suppressions.txt` under this
deviation keeps the source clean.

**Suppression-file layout.** 5.7 anonymous-enum suppressions
were historically grouped in the D.003 block (struct-tag
repetition) because D.003 was the original 5.7 deviation; per-group
conformance stories migrate them into the D.009 block as they
review their cluster (S10.16 moved the Senders-cluster ones).
Until every group has run, both blocks contain rule 5.7 lines
and the deviation that authorises each is determined by the kind
of identifier the cppcheck-misra finding lands on (struct tag →
D.003; anonymous enum → D.009).

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
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.010 — Rule 20.10: `#` stringification in the `SOLIDSYSLOG_STATIC_ASSERT` polyfill

### Rule

> **Rule 20.10 (Advisory)** — The `#` and `##` preprocessor operators
> should not be used.

### Deviation

`Core/Source/SolidSyslogMacros.h` defines `SOLIDSYSLOG_STATIC_ASSERT`. The
native C++/C11 expansions need a string-literal message, so the macro
stringifies its `msg` argument via the standard two-step `#`-operator idiom:

```c
#define SOLIDSYSLOG_STATIC_ASSERT_STRING_INNER(s) #s                    /* <- # */
#define SOLIDSYSLOG_STATIC_ASSERT_STRING(s)       SOLIDSYSLOG_STATIC_ASSERT_STRING_INNER(s)
#if defined(__cplusplus)
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg)      static_assert((cond), SOLIDSYSLOG_STATIC_ASSERT_STRING(msg))
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg)      _Static_assert((cond), SOLIDSYSLOG_STATIC_ASSERT_STRING(msg))
#else
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg)      extern char SolidSyslogStaticAssertViolated[(cond) ? 1 : -1]
#endif
```

The `#s` operator is the deviation. The C99 fallback (`extern char` array) uses
no preprocessor operators — a fixed name suffices because identical extern
declarations in one translation unit are compatible, so no `__LINE__` pasting
(and no `##`) is required.

### Scope

`Core/Source/SolidSyslogMacros.h` only. One line — the
`SOLIDSYSLOG_STATIC_ASSERT_STRING_INNER` definition.

### Rationale

C++ `static_assert` and C11 `_Static_assert` are the standard compile-time
assertion primitives, and the project compiles at `--std=c11` (so the
`_Static_assert` branch is what every normal and CI build — including the
cppcheck-misra lane — compiles). Their message argument is a string literal,
and there is no way to convert an arbitrary identifier-shaped message into one
without `#`. The alternatives all regress:

| Alternative | Why rejected |
|-------------|--------------|
| Hard-coded literal message in the macro | Loses per-site context — every assertion would report the same generic string. |
| Force every caller to pass a string literal | Spreads strings across the call sites and gives up the per-site identifier form some files already use. |
| Drop the message argument entirely | Loses readability at the assertion site. |

The advisory rule's intent is to discourage opaque token games. The two-step
stringify idiom here is the standard, documented C preprocessor pattern and has
been since C89; it is neither opaque nor novel.

### Risk and mitigation

- **Single-site exposure.** The deviation is the macro definition
  itself, line-specific. Any future `#`/`##` use elsewhere would
  surface as a fresh 20.10 finding, not absorbed by this suppression.
- **Elimination path.** If the project ever drops the `msg` parameter
  (or moves entirely to inline `_Static_assert((cond), "literal")` at
  call sites), the deviation can be retired.

### Approval

Project owner — David Cozens. Recorded under
[S10.10](https://github.com/cososo-ltd/solid-syslog/issues/375).

---

## D.011 — Rule 2.5: public API macros consumed outside the cppcheck-misra scope

### Rule

> **Rule 2.5 (Advisory)** — A project should not contain unused macro
> definitions.

### Deviation

`Core/Interface/SolidSyslogCircularBuffer.h` declares one function-like
macro — `SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES` — that integrator code
uses to size caller-supplied ring memory. cppcheck-misra runs only over
the Strict tier (`Core/Source/`) and Pragmatic tier (`Platform/*/Source/`);
the actual consumers live under `Tests/` (Consistency-only tier) and
`Bdd/Targets/` (Out of scope) and are therefore invisible to the
checker.

### Scope

`Core/Interface/SolidSyslogCircularBuffer.h` — one macro definition.

A future per-component sweep may surface similar findings on other
public API macros (per the tier model, MISRA enforcement does not cross
into `Tests/` or `Bdd/`). When that happens, the deviation extends to
those files; the rule still catches genuinely-unused macros inside the
scanned scope.

### Rationale

The macros *are* used — by integrators in `Tests/` and `Bdd/Targets/`.
Verified by `grep` over the tree:

```text
Tests/SolidSyslogCircularBufferTest.cpp         — RING_BYTES
Tests/SolidSyslogBlockStoreDrainOrderingTest.cpp — RING_BYTES
Bdd/Targets/Windows/BddTargetWindows.c           — RING_BYTES
Bdd/Targets/FreeRtos/main.c                      — RING_BYTES
```

The macro is part of the public API; integrators use it to size
the caller-supplied ring buffer in bytes, derived from a maximum
message count (e.g. `uint8_t ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(4)]`
allocates enough bytes for four full-size messages plus their
length headers).

The alternatives all regress:

| Alternative | Why rejected |
|-------------|--------------|
| Inline `cppcheck-suppress misra-c2012-2.5` at each macro | Inline suppressions are weaker by MISRA Compliance:2020 §4.2 (rationale scattered, not centrally auditable). Project preference is structural deviations in this document. |
| Widen the cppcheck-misra scan to include `Tests/` | Tests are the Consistency-only tier per E10's tier model; running MISRA there is out of scope by design. |
| Move the macros into `Core/Source/` | Public API by definition lives under `Core/Interface/`. Moving them would break the audience-segregated header layout. |

### Risk and mitigation

- **Genuinely unused public macros.** A future public-API macro that
  is *truly* unused (no integrator consumer either) would still be a
  defect; this deviation is line-specific, so a new unused macro
  surfaces as a fresh 2.5 finding rather than being silently absorbed.
- **Elimination path.** If the cppcheck-misra scan is ever widened to
  include `Tests/` and `Bdd/Targets/` (unlikely under the current tier
  model), the suppressions become unnecessary and can be removed.

### Approval

Project owner — David Cozens. Recorded under
[S10.10](https://github.com/cososo-ltd/solid-syslog/issues/375).

---

## D.012 — Rule 8.9: file-scope `static const` referenced from a file-scope enum + one function

### Rule

> **Rule 8.9 (Advisory)** — An object should be defined at block scope
> if its identifier only appears in a single function.

### Deviation

`Core/Source/SolidSyslogFileBlockDevice.c:20` declares
`static const char FILE_EXTENSION[] = ".log"`. The constant is the
single source of truth for the on-disk filename extension and is
referenced from two places in the translation unit:

1. The file-scope enum at line 25 — `sizeof(FILE_EXTENSION) - 1U`
   contributes to `FILENAME_SUFFIX`, which in turn computes
   `MAX_PREFIX_LENGTH` (an integer constant expression consumed by
   the formatter at the call site).
2. `FileBlockDevice_FormatBlockFilename` at line 214 — both the
   bytes pointer and the runtime length are derived from the same
   constant.

cppcheck-misra's 8.9 tracker counts only function-scope references.
The file-scope enum reference at line 25 is invisible to it, so it
sees a single function reference (line 214) and reports the constant
as having "block-scope-only" usage — even though moving it into the
function would break the enum's compile-time `sizeof()` evaluation.

### Scope

`Core/Source/SolidSyslogFileBlockDevice.c:20` — one declaration.

A future per-component sweep may surface this shape on other
file-scope `static const` objects whose identifier is used by a
file-scope enum's `sizeof()`/value initialiser **and** exactly one
function. When that happens, the deviation extends to those files;
the rule still catches genuinely single-function-scoped objects.

### Rationale

The constant cannot move to block scope without regressing the code.
Three alternatives were considered and rejected:

- Inlining the literal at the enum site and at the call site would
  introduce a second copy of `".log"`, violating DRY for what is
  effectively a single on-disk format invariant.
- Promoting the constant's dependents from enum entries to file-scope
  `static const size_t` (verified experimentally during S10.18) does
  **not** satisfy the rule — the tracker treats file-scope references
  uniformly, so a new `static const size_t FILENAME_SUFFIX` trips a
  *second* 8.9 finding for the same reason. The fix path amplifies
  the problem rather than resolving it.
- Promoting to `#define FILE_EXTENSION ".log"` would side-step rule
  8.9 (macros are not objects) at the cost of introducing a string
  macro where the codebase otherwise uses `static const`. Rejected
  to keep the file-scope-const idiom consistent across the tree.

Summary:

| Alternative | Why rejected |
|-------------|--------------|
| Inline `cppcheck-suppress misra-c2012-8.9` at the declaration | Inline suppressions are weaker by MISRA Compliance:2020 §4.2 (rationale scattered, not centrally auditable). Project preference is structural deviations in this document. |
| Inline the `".log"` literal at both use sites | DRY violation for a single-source-of-truth on-disk constant. |
| Promote the dependent enum entries to `static const size_t` | Verified to **not** satisfy 8.9; instead surfaces a second false positive on the new constant. |
| Promote to `#define FILE_EXTENSION ".log"` | Introduces a string macro inconsistent with the file-scope-const pattern used elsewhere in storage code. |

### Risk and mitigation

- **Genuinely single-function-scoped constants.** A new file-scope
  `static const` whose identifier really appears in only one
  function would still be a defect; this deviation is line-specific,
  so a fresh 8.9 finding elsewhere still surfaces in CI.
- **Elimination path.** If a future cppcheck-misra version teaches
  its 8.9 tracker to count file-scope-initialiser references, the
  suppression becomes unnecessary and can be removed.

### Approval

Project owner — David Cozens. Recorded under
[S10.18](https://github.com/cososo-ltd/solid-syslog/issues/430).

---

## D.013 — Rule 11.5: `void*` ↔ `unsigned char*` at third-party byte-buffer API boundaries

### Rule

> **Rule 11.5 (Advisory)** — A conversion should not be performed from
> pointer to `void` into pointer to object.

### Deviation

`SolidSyslogStream::Send` takes `const void*` and `SolidSyslogStream::Read`
takes `void*` — the project-wide byte-buffer contract used by every
Stream implementation. Some third-party C libraries (notably mbedTLS)
type their byte buffers as `const unsigned char*` / `unsigned char*`
rather than `void*`. The implementation cast bridging the two is
unavoidable at the API boundary:

```c
int rc = mbedtls_ssl_write(&self->SslContext, (const unsigned char*) buffer, size);
```

Rule 11.5 fires on each such adapter cast.

### Scope

`Platform/MbedTls/Source/SolidSyslogMbedTlsStream.c` — two sites
(`MbedTlsStream_Send`, `MbedTlsStream_Read`).

The deviation extends to any future Stream / Datagram / hash / MAC
implementation that wraps a byte-typed third-party C API
(`unsigned char*` rather than `void*`). The OpenSSL adapter
(`Platform/OpenSsl/Source/SolidSyslogTlsStream.c`) does **not** fall
under this deviation — `SSL_write` / `SSL_read` take `void*` and so no
cast is needed.

### Rationale

The alternatives all regress:

| Alternative | Why rejected |
|-------------|--------------|
| Refactor `SolidSyslogStream::Send`/`Read` to use `unsigned char*` | Public-API ABI change that propagates to every Stream implementation (Posix TCP, Winsock TCP, FreeRTOS TCP, OpenSSL TLS, mbedTLS TLS, NullStream) and every Stream caller (`SolidSyslogStreamSender`). The `void*` byte-buffer contract is the conventional C idiom for transport interfaces and matches POSIX `send`/`recv`, OpenSSL `SSL_write`/`SSL_read`, etc. Changing it for the sake of one third-party API's typing choice is the wrong direction. |
| Copy through an `unsigned char` scratch buffer per call | Runtime cost on the hot send/receive path; adds a fixed-size scratch or a stack-allocated VLA in a critical-path function. Defeats the zero-copy intent of the Stream contract. |
| Inline `cppcheck-suppress misra-c2012-11.5` at each site | Inline suppressions are weaker by MISRA Compliance:2020 §4.2 (rationale scattered, not centrally auditable). Project preference is structural deviations in this document. |

The cast is well-defined: `unsigned char` may alias any object type
(C99 §6.5 ¶7), so reinterpreting a `void*` byte buffer as
`unsigned char*` and back is a no-op at the abstract-machine level.

### Risk and mitigation

- **Alignment** — Both representations are byte-addressed; no
  alignment promotion occurs. The cast targets `unsigned char*`, which
  has the weakest alignment requirement of any object pointer.
- **Type safety** — The caller-supplied buffer originates as a
  contiguous byte sequence (typically the formatted syslog record);
  treating it as `unsigned char*` at the third-party API boundary is
  the same byte sequence under a different pointer type.
- **Elimination path** — A future revision of the Stream API that
  adopts `unsigned char*` directly would retire this deviation.
  Tracked as a possible E10-successor refactor, not scheduled.

### Approval

Project owner — David Cozens. Recorded under
[S10.20](https://github.com/cososo-ltd/solid-syslog/issues/437).

## D.014 — Rule 8.7: public-API `SolidSyslogErrorSource` objects (retired)

**Retired in S12.26.** This deviation covered the crypto-policy
`SolidSyslogErrorSource` objects, which rule 8.7 flagged because the
`Xxx_Report` wrapper confined every emission to the source's own `*Messages.c`
— a single translation unit. S12.26 decoupled error text from the library
(deleting the `*Messages.c` message tables) and unwound the `_Report` wrapper,
so each source is now defined in its class's vtable TU and referenced from both
that TU's emit sites and its `*Static.c` lifecycle code — genuinely cross-TU,
exactly the resolution this deviation's "Risk and mitigation" anticipated.
cppcheck-misra reports no 8.7 finding for any error source; the suppression
lines were removed.

Recorded under [S17.02](https://github.com/cososo-ltd/solid-syslog/issues/493),
retired under [S12.26](https://github.com/cososo-ltd/solid-syslog/issues/507).
