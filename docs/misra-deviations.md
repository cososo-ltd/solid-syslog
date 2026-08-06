# MISRA C:2012 deviations

SolidSyslog is MISRA-informed, not certified-compliant. The project
adopts a curated subset of MISRA C:2012 rules per tier (see
`docs/NAMING.md` for the tier model). This document records every
deliberate deviation from a rule the project otherwise enforces, and every
suppression that is not a deviation at all.

Those are two different things and each entry says which it is. A **deviation**
is a departure: the code does something the guideline forbids, and the entry
justifies it. A **tool limitation** is not: the code complies, cppcheck-misra
reports a finding anyway, and the entry explains why the report is wrong. Both
are recorded here because a suppression exists either way, and the authorisation
for a suppression belongs in one place. Counting the entries is therefore not a
count of departures — eight of them are.

This register is published as evidence of process, not as a compliance
submission. It exists so that an integrator building SolidSyslog into a
MISRA-constrained product can see where the code knowingly departs from a
guideline, and judge each departure against their own risk posture. It makes no
conformance claim on your behalf, and it is an input to your compliance
documentation rather than a substitute for it.

MISRA and MISRA C are registered trade marks of The MISRA Consortium Limited,
used here for identification only. This project is neither endorsed by nor
affiliated with MISRA.

Each deviation is paired with a matching entry in `misra_suppressions.txt`
(the cppcheck-misra input). The two files are complementary:

| File | Audience | Purpose |
|------|----------|---------|
| `misra_suppressions.txt` | cppcheck-misra | Machine-readable suppressions per rule / file / line |
| `docs/misra-deviations.md` | Reviewers, auditors, integrators | Why each deviation exists, with rationale, scope, approval |

`misra_suppressions.txt` is the authoritative instance-level trace: its entries
are line-specific, so every individual site a deviation authorises appears there
by rule, file and line, and each block back-references the deviation in this
document that authorises it. A finding on a line not listed is not covered by
any deviation and surfaces in CI. The suppressions file was populated
under [S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367)
after the rule subset was curated; before then it carried only a
header comment.

Each entry follows a fixed shape: the guideline and its category, the construct
that deviates, the scope the deviation covers, the engineering rationale, the
residual risk and how it is mitigated, and a named approval with its dates.

Every deviation here is raised and approved by the same person. SolidSyslog is
developed by a one-person consultancy, so the project owner holds the designated
technical authority for these decisions; there is no second reviewer and the
record does not pretend otherwise. An integrator running their own compliance
process should re-review each deviation against their own risk posture rather
than inheriting this approval.

## Guideline text is not reproduced here

Each entry identifies its guideline by number and category and then describes
**the construct in SolidSyslog that deviates** — not what the guideline says.
MISRA C:2012 is copyrighted and not redistributable, so its rule text,
amplification and examples are omitted deliberately rather than by oversight.
Read them in your own licensed copy, available from
[misra.org.uk](https://misra.org.uk/); Appendix A lists every guideline with its
category.

Nothing is lost by this. A deviation record exists to show that the project
understood the guideline and reasoned about the risk of departing from it, and a
precise description of our own code demonstrates that better than a restated
headline an assessor already has in front of them.

## Language edition for clause references

Unless otherwise stated, all clause references are to ISO/IEC 9899:1999 (C99).
C99 is the conformance baseline — the edition the source is written against and
claims to be valid under. It is not the same thing as the build configuration:
the default build selects C11 (`CMAKE_C_STANDARD` is 11 unless overridden), and
the `c99` preset verifies the baseline. Where a deviation concerns a code path
that only a later edition compiles, that entry names the applicable edition and
cites both.

Paragraph numbering differs between editions — §6.7.2.1 renumbered when C11
added anonymous structure and union members — so a C11 paragraph number read
against a C99 copy lands on the wrong sentence. That is why the edition is fixed
here rather than left to the reader.

---

## D.001 — Rule 5.1 external identifier uniqueness relaxed to 63 characters

### Guideline

**MISRA C:2012 Rule 5.1** — Required.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Deviation — the code departs from the guideline.

### Deviation

SolidSyslog requires external identifiers to be distinct within their first
63 characters rather than their first 31.

Thirty-one is the floor the C language sets: C99 §5.2.4.1 guarantees
only 31 significant characters in an external identifier, so a conforming
compiler is permitted to treat two identifiers that agree that far as the same
identifier. The project asserts a longer guarantee than the language requires,
and states the number rather than leaving it implicit.

Every toolchain this project builds and tests on resolves external identifiers
well past 63 characters, and the table under *Rationale* below records what
each of them documents. Yours may not be among them: if you build SolidSyslog
with a different compiler or linker, confirm its significant-character limit
before relying on this deviation.

### Scope

- **Strict tier** — `Core/Interface/`, `Core/Source/`,
  `Platform/*/Interface/`
- **Pragmatic tier** — `Platform/*/Source/`

The deviation does not apply to the Consistency-only or Out-of-scope
tiers (rule 5.1 is not enforced there at all).

### Rationale

The C99 31-character limit is a legacy linker artifact from the late
1980s. No toolchain SolidSyslog targets imposes anything close to it:

| Toolchain | External identifier behaviour |
|-----------|-------------------------------|
| GCC (incl. `arm-none-eabi-gcc`)                | No compiler-imposed limit; identifier length is delegated to the target's linker, and all characters are significant on every linker SolidSyslog targets (ld, gold, lld, link.exe). See GCC manual, "Implementation-defined behavior". |
| Clang / LLVM (incl. Arm Compiler 6 / armclang) | Same rule as GCC for external identifiers — no compiler-imposed limit. |
| MSVC                                           | Documented maximum identifier length **2,047 characters** ([Microsoft Learn — C Identifiers](https://learn.microsoft.com/en-us/cpp/c-language/c-identifiers)). CI builds with the `windows-latest` toolchain; older MSVC releases are not tested. |
| IAR Embedded Workbench, Keil ARMCC 6            | Not built in CI. Identifier limits are documented per compiler SKU; confirm against your SKU's reference at port time. |

The Tier 1 naming scheme in `docs/NAMING.md` (form
`SolidSyslogClass_Function`) routinely produces identifiers in the
30–40 character range — `SolidSyslogPlusTcpResolver_Create` is
40, `SolidSyslogPlusTcpTcpStream_Destroy` is 36 — and a few public
storage-size enums sit just below 40 (e.g.
`SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE`, 40). Strict 31-character
distinctness would either collapse identifier pairs that read
identically up to a trailing word (`SolidSyslogPlusTcpResolver_Create`
vs `SolidSyslogPlusTcpResolver_Destroy`) into a single name, or force
unidiomatic abbreviation throughout the public
API. Neither outcome serves clarity, and neither preserves the
distinguishability the guideline exists to protect. Sixty-three characters
preserves both.

63 was chosen rather than "unlimited" so the project still names a
concrete number that every targeted toolchain comfortably exceeds. It
also matches C99's separate 63-character minimum for internal
identifiers (§5.2.4.1) — a single number applies project-wide.

### Risk and mitigation

- **Portability** — Constrained to toolchains that support ≥ 63
  significant characters in external identifiers. The table above states
  which of those are proven by CI and which rest on documentation;
  adding a target requires verifying the constraint on it.
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

Raised and approved 2026-05-14 by the project owner, David Cozens. Recorded as
the founding entry in this document under
[S10.01](https://github.com/cososo-ltd/solid-syslog/issues/357).

---

## D.002 — Rules 11.2 / 11.3 / 11.5: vtable downcasts + Formatter

### Guidelines

- **MISRA C:2012 Rule 11.2** — Required.
- **MISRA C:2012 Rule 11.3** — Required.
- **MISRA C:2012 Rule 11.5** — Advisory.

**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Deviation — the code departs from the guideline.

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
(`SolidSyslog<Plat>Address_AsSockaddrIn` /
`SolidSyslog<Plat>Address_AsConstSockaddrIn` /
`SolidSyslog<Plat>Address_AsFreertosSockaddr` /
`SolidSyslog<Plat>Address_AsConstFreertosSockaddr`) plus a
`HandleFromIndex(size_t)` helper in `*AddressStatic.c` that converts a
pool slot index back to the public handle type. Rule 11.3 fires on
every such cast, and rule 11.2 fires alongside it here but not on the
other vtable classes: `struct SolidSyslogAddress` is deliberately an
incomplete type in the public header, so these conversions involve a
pointer to an incomplete type as well as to a different object type.

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
codepoint-boundary bytes from the caller's `const void*` buffer, a
byte-buffer reinterpretation that crosses the same advisory rule. The
third-party API contract (the public `Send` / `SendTo` interface) is
`void*` for opacity; the byte-level work needs a concrete unit type.

### Scope

- **Strict tier** — `Core/Source/`: the `SelfFromBase` helpers on every vtable
  class, and the Formatter storage cast of sub-case (b). 14 sites.
- **Pragmatic tier** — `Platform/*/Source/`: the same `SelfFromBase` shape in
  each adapter, the per-platform Address downcasts, and the callback `void*`
  casts of sub-case (c). 66 sites, across the Atomics, FatFs, FreeRtos,
  LwipRaw, MbedTls, OpenSsl, PlusFat, PlusTcp, Posix and Windows packs.

80 line-specific suppressions in total — 12 against rule 11.2, 57 against 11.3
and 11 against 11.5. The deviation does not extend to `Tests/` or `Bdd/`, where
these rules are not enforced.

A new class added to any vtable role inherits this shape, and its suppressions
belong in this block; adding them is a review step, not an automatic
consequence, because the reviewer has to confirm the new site really is the
`SelfFromBase` pattern and not a different conversion wearing the same name.

### Rationale

Every class with a Create/Destroy lifecycle uses the static pool
allocator rather than caller-supplied storage, so the only structural
pointer conversions are the vtable / opaque-handle downcast (required by
the OO-in-C interface decoupling) and the one non-pool exception above
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
- **Validity of the conversion, sub-cases (a) and (c)** — These do not
  rest on an alignment argument at all. The public base struct is the
  first member of the concrete struct, and §6.7.2.1 ¶13 guarantees that
  a pointer to a structure object, suitably converted, points to its
  initial member and back again. The address is the same address by
  definition, so no alignment question arises. Sub-case (c) is the same
  guarantee reached through the library's own `void*` round trip: the
  pointer that comes back is the one that went out.
- **Alignment, sub-case (b) only** — The Formatter is the case where
  alignment is the load-bearing argument, because its storage is a
  caller-declared array rather than a struct whose first member is the
  base. Storage is declared as `intptr_t storage[N]` (or a struct of the
  same shape), giving alignment at least as strict as any pointer or
  scalar the impl contains.
- **Static analysis** — These rules are advisory (11.5) or required
  (11.2, 11.3). All current findings are suppressed via
  `misra_suppressions.txt` referencing this section. The pattern is
  reviewed once here, not per call site.

### Approval

Raised 2026-05-14, approved 2026-05-15 by the project owner, David Cozens. Recorded under
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

### Guideline

**MISRA C:2012 Rule 5.7** — Required.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Tool limitation — the code complies; cppcheck-misra reports a finding regardless.

cppcheck-misra interprets Rule 5.7 strictly — every repeated `struct X`
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

Raised 2026-05-14, approved 2026-05-15 by the project owner, David Cozens. Recorded under
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.004 — Rule 18.4: pointer arithmetic on record buffers (retired)

**Retired in S10.19.** This deviation authorised `uint8_t*` pointer arithmetic in
`Core/Source/RecordStore.c`, where four field-offset helpers walked the
`[magic][length][message]` record layout by adding an offset to a base address.
S10.19 rewrote them to take the address of an indexed element (`&base[OFFSET]`)
instead. That is the same address by definition, but rule 18.4 fires on the `+`,
`-`, `+=` and `-=` operators specifically rather than on subscripting, so the
finding no longer arises and the deviation had nothing left to authorise.
cppcheck-misra reports no 18.4 finding in `RecordStore.c`; the suppression was
removed from `misra_suppressions.txt` at the same time.

The entry is kept, rather than the number reused, so the register has no gaps and
a reader of an older revision can still resolve D.004. Raised 2026-05-14 and
approved 2026-05-15 by the project owner, David Cozens, alongside the other
founding entries; retired 2026-05-23 under
[#436](https://github.com/cososo-ltd/solid-syslog/pull/436).

---

## D.005 — Rule 18.7: flexible array members

### Guideline

**MISRA C:2012 Rule 18.7** — Required.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Deviation — the code departs from the guideline.

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

`SolidSyslogCircularBuffer` does not use this shape — its instance
struct holds an external ring pointer rather than a trailing FAM.

### Rationale

The Formatter implements the variable-size variant of the
caller-supplied-storage pattern (D.002). The calling translation unit
declares a storage buffer of arbitrary size (with a minimum enforced by
`_Static_assert`), and the class lives inside that storage —
bookkeeping fields at the start, payload bytes filling the rest.

The flexible array member is C99's standard mechanism for exactly this
shape (§6.7.2.1 ¶16). The alternatives all regress:

| Alternative | Why rejected |
|-------------|--------------|
| Pointer to separately-allocated payload | Re-introduces dynamic allocation or a second storage parameter. |
| Fixed-size payload (`char buffer[MAX]`) | Forces every integrator to pay for the worst-case footprint. |
| Trailing `char buffer[1]` "struct hack" | Pre-C99 idiom, technically UB; flexible array members exist precisely because the hack was unsafe. |

### Risk and mitigation

- **Compiler support.** GCC, Clang and the ARM cross-compilers accept
  the construct as the C99 feature it is, and CI compiles it on every
  push. MSVC compiles it too, but reports C4200 — it treats a trailing
  unsized array as a nonstandard extension — so the build carries
  `/wd4200` for this construct specifically (see `CMakeLists.txt`);
  without it, `/WX` would fail the Windows lane. IAR and Keil ARMCC 6
  are not built in CI, so support there rests on their documentation
  and is confirmed at port time rather than per push.
- **Allocation surprise.** The `_Static_assert` accompanying each
  flexible-array struct pins the storage-type-to-impl-type
  relationship at build time; an undersized storage allocation is a
  compile error.

### Approval

Raised 2026-05-14, approved 2026-05-15 by the project owner, David Cozens. Recorded under
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.006 — Rule 11.8: `const` qualification under field access of `const struct*`

### Guideline

**MISRA C:2012 Rule 11.8** — Required.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** both kinds, which is why they share an entry — category 1
below is a tool limitation, category 2 is a genuine deviation.

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

   §6.5.2.3 ¶4 governs this, and it has to be read whole. The result of
   `->` is the named member's type; and where the left operand is a
   pointer to a qualified type, the result carries the so-qualified
   version of that type. That second clause looks at first like it works
   against us. It does not. The member's type here is
   `struct SolidSyslogBuffer *`, so the so-qualified version is
   `struct SolidSyslogBuffer * const` — the qualification attaches to the
   pointer, not to the object the pointer designates. Passing it by value
   to `InstallBuffer` copies the pointer, and a top-level qualifier on a
   copied value is discarded.

   So no qualification is removed from the pointed-to type, and no cast
   is performed. cppcheck-misra flags the access anyway, tracking the
   outer `const` on `*config` rather than the type of the member
   expression. The project assesses this as a tool limitation rather
   than a departure from the guideline, and records it here so the
   assessment is visible rather than silent.

   The same pattern recurs in
   `SolidSyslogMessageFormatter_Format(const struct
   SolidSyslogMessageFormatterContext* context)` (5 sites — `context->Clock`,
   `GetHostname`, `GetAppName`, `GetProcessId`, `Sd`), which reads its
   read-only context exactly as `<Class>_Create` reads its config. The `const`
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
   payload — that is the `PBUF_REF` zero-copy contract, set out under
   [Datagram — pbuf strategy](integrating-lwip.md#datagram--pbuf-strategy)
   — but the field type does not encode that. Assigning
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
  `Core/Source/SolidSyslogBlockSequence.c`
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

The two platform-API sites are the standard case of a const-correct interior
forced to strip qualification at a fixed third-party API boundary. Both
upstream declarations (Microsoft's `select()` timeout,
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

Raised 2026-05-14, approved 2026-05-15 by the project owner, David Cozens. Recorded under
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.007 — Rule 21.10: transitive `<wchar.h>` via `<time.h>`

### Guideline

**MISRA C:2012 Rule 21.10** — Required.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Tool limitation — the code complies; cppcheck-misra reports a finding regardless.

cppcheck-misra also raises this rule for `<wchar.h>` inclusion, which is what
brings the construct below into scope.

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

Raised 2026-05-14, approved 2026-05-15 by the project owner, David Cozens. Recorded under
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.008 — Rule 21.6: `<stdio.h>` for `SEEK_SET` / `SEEK_END` only

### Guideline

**MISRA C:2012 Rule 21.6** — Required.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Deviation — the code departs from the guideline.

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

Raised 2026-05-14, approved 2026-05-15 by the project owner, David Cozens. Recorded under
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.009 — Rules 2.4 / 5.7: anonymous `enum` used as named-constant container

### Guidelines

- **MISRA C:2012 Rule 2.4** — Advisory.
- **MISRA C:2012 Rule 5.7** — Required.

**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Tool limitation — the code complies; cppcheck-misra reports a finding regardless.

cppcheck-misra interprets an anonymous `enum { ... };` declaration
(no enum tag, no `typedef`) two ways:

- under 2.4 it reports the enum tag as unused — the enumerators are
  used as named constants but the enum type itself is never referenced;
- under 5.7 it reports the tag as non-unique — every anonymous `enum`
  shares the same empty tag identifier, so the second and subsequent
  ones collide.

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

There are 45 such declarations across 44 files in `Core/` and
`Platform/`. Adding inline-suppress comments at every
site would add visual noise next to a project-wide intentional
idiom — listing them in `misra_suppressions.txt` under this
deviation keeps the source clean.

**Suppression-file layout.** Two deviations authorise rule 5.7 findings, and
which one applies is decided by the identifier the finding lands on: a repeated
struct tag is D.003, an anonymous enum is D.009. That is a standing convention,
not a transitional state — both kinds of finding exist permanently in this
codebase, so both blocks permanently carry rule 5.7 lines. Each block in
`misra_suppressions.txt` is headed by the deviation that authorises its entries,
so the mapping is explicit per line rather than inferred.

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
is never needed, only the enumerator *values*. Adding a tag
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

Raised 2026-05-14, approved 2026-05-15 by the project owner, David Cozens. Recorded under
[S10.06](https://github.com/cososo-ltd/solid-syslog/issues/367).

---

## D.010 — Rule 20.10: `#` stringification in the `SOLIDSYSLOG_STATIC_ASSERT` polyfill

### Guideline

**MISRA C:2012 Rule 20.10** — Advisory.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Deviation — the code departs from the guideline.

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

`SOLIDSYSLOG_STATIC_ASSERT` selects one of three expansions on
`__cplusplus` / `__STDC_VERSION__`, and all three are compiled: C++
`static_assert` in the CppUTest harnesses, C11 `_Static_assert` in the default
build (`CMAKE_C_STANDARD` is 11 unless overridden), and the C99
negative-array-size fallback under the `c99` preset, which is the pre-release
check that the portable surface is still C99 — see
[local checks](local-checks.md). The deviation is confined to the two
string-literal forms; only they need the message stringified, and the C99
fallback uses no preprocessor operator at all.

C++ `static_assert` and C11 `_Static_assert` are the standard compile-time
assertion primitives for their editions. Their message argument is a string
literal, and there is no way to convert an arbitrary identifier-shaped message
into one without `#`. The alternatives all regress:

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

Raised 2026-05-15, approved 2026-05-16 by the project owner, David Cozens. Recorded under
[S10.10](https://github.com/cososo-ltd/solid-syslog/issues/375).

---

## D.011 — Rule 2.5: public API macros consumed outside the cppcheck-misra scope

### Guideline

**MISRA C:2012 Rule 2.5** — Advisory.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Deviation — the code departs from the guideline.

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

This entry authorises that one macro and no other. Per the tier model, MISRA
enforcement does not cross into `Tests/` or `Bdd/`, so a future sweep may
surface the same shape on another public API macro. That does not extend this
deviation automatically: each new instance is reviewed on its merits and either
amends this entry with the file named, or is raised as its own. Until then the
rule still catches genuinely unused macros inside the scanned scope.

### Rationale

The macro *is* used by integrators in `Tests/` and `Bdd/Targets/`.
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
| Inline `cppcheck-suppress misra-c2012-2.5` at the macro | **Project preference.** Deviations are recorded structurally in this document so the rationale is centrally auditable rather than scattered across call sites. |
| Widen the cppcheck-misra scan to include `Tests/` | Tests are the Consistency-only tier per E10's tier model; running MISRA there is out of scope by design. |
| Move the macro into `Core/Source/` | Public API by definition lives under `Core/Interface/`. Moving it would break the audience-segregated header layout. |

### Risk and mitigation

- **Genuinely unused public macros.** A future public-API macro that
  is *truly* unused (no integrator consumer either) would still be a
  defect; this deviation is line-specific, so a new unused macro
  surfaces as a fresh 2.5 finding rather than being silently absorbed.
- **Elimination path.** If the cppcheck-misra scan is ever widened to
  include `Tests/` and `Bdd/Targets/` (unlikely under the current tier
  model), the suppressions become unnecessary and can be removed.

### Approval

Raised 2026-05-15, approved 2026-05-16 by the project owner, David Cozens. Recorded under
[S10.10](https://github.com/cososo-ltd/solid-syslog/issues/375).

---

## D.012 — Rule 8.9: file-scope `static const` referenced from a file-scope enum + one function

### Guideline

**MISRA C:2012 Rule 8.9** — Advisory.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Tool limitation — the code complies; cppcheck-misra reports a finding regardless.

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

This entry authorises that one declaration and no other. A future sweep may
surface the same shape elsewhere — a file-scope `static const` whose identifier
is read by a file-scope enum initialiser and exactly one function. Each such
instance is reviewed on its merits and either amends this entry with the file
named, or is raised as its own; it is not covered by this record until that
happens. The rule still catches genuinely single-function-scoped objects.

### Rationale

The constant cannot move to block scope without regressing the code.
Three alternatives were considered and rejected:

- Inlining the literal at the enum site and at the call site would
  introduce a second copy of `".log"`, violating DRY for what is
  effectively a single on-disk format invariant.
- Promoting the constant's dependents from enum entries to file-scope
  `static const size_t` (verified experimentally during S10.18) does
  not satisfy the rule — the tracker treats file-scope references
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
| Inline `cppcheck-suppress misra-c2012-8.9` at the declaration | **Project preference.** Deviations are recorded structurally in this document so the rationale is centrally auditable rather than scattered across call sites. |
| Inline the `".log"` literal at both use sites | DRY violation for a single-source-of-truth on-disk constant. |
| Promote the dependent enum entries to `static const size_t` | Verified to not satisfy 8.9; instead surfaces a second false positive on the new constant. |
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

Raised and approved 2026-05-22 by the project owner, David Cozens. Recorded under
[S10.18](https://github.com/cososo-ltd/solid-syslog/issues/430).

---

## D.013 — Rule 11.5: `void*` ↔ `unsigned char*` at third-party byte-buffer API boundaries

### Guideline

**MISRA C:2012 Rule 11.5** — Advisory.
**Rule text:** not reproduced (see [above](#guideline-text-is-not-reproduced-here)).
**Classification:** Deviation — the code departs from the guideline.

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

A future Stream, Datagram, hash or MAC implementation wrapping a byte-typed
third-party C API (`unsigned char*` rather than `void*`) will meet the same
boundary, but is not covered by this record until reviewed and added to it —
or given its own entry. The OpenSSL adapter
(`Platform/OpenSsl/Source/SolidSyslogTlsStream.c`) does not fall
under this deviation — `SSL_write` / `SSL_read` take `void*` and so no
cast is needed.

### Rationale

The alternatives all regress:

| Alternative | Why rejected |
|-------------|--------------|
| Refactor `SolidSyslogStream::Send`/`Read` to use `unsigned char*` | Public-API ABI change that propagates to every Stream implementation (Posix TCP, Winsock TCP, FreeRTOS TCP, OpenSSL TLS, mbedTLS TLS, NullStream) and every Stream caller (`SolidSyslogStreamSender`). The `void*` byte-buffer contract is the conventional C idiom for transport interfaces and matches POSIX `send`/`recv`, OpenSSL `SSL_write`/`SSL_read`, etc. Changing it for the sake of one third-party API's typing choice is the wrong direction. |
| Copy through an `unsigned char` scratch buffer per call | Runtime cost on the hot send/receive path; adds a fixed-size scratch or a stack-allocated VLA in a critical-path function. Defeats the zero-copy intent of the Stream contract. |
| Inline `cppcheck-suppress misra-c2012-11.5` at each site | **Project preference.** Deviations are recorded structurally in this document so the rationale is centrally auditable rather than scattered across call sites. |

The cast is well-defined: `unsigned char` may alias any object type
(§6.5 ¶7), so reinterpreting a `void*` byte buffer as
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

Raised and approved 2026-05-23 by the project owner, David Cozens. Recorded under
[S10.20](https://github.com/cososo-ltd/solid-syslog/issues/437).

---

## D.014 — Rule 8.7: public-API `SolidSyslogErrorSource` objects (retired)

**Retired in S12.26.** This deviation covered the crypto-policy
`SolidSyslogErrorSource` objects, which rule 8.7 flagged because the
`<Class>_Report` wrapper confined every emission to the source's own `*Messages.c`,
a single translation unit. S12.26 decoupled error text from the library
(deleting the `*Messages.c` message tables) and unwound the `<Class>_Report` wrapper,
so each source is now defined in its class's vtable TU and referenced from both
that TU's emit sites and its `*Static.c` lifecycle code, genuinely cross-TU —
which is the resolution the entry's risk analysis anticipated before it was
collapsed to this note; see the revision prior to retirement for that text.
cppcheck-misra reports no 8.7 finding for any error source; the suppression
lines were removed.

Raised and approved 2026-05-31 by the project owner, David Cozens, under
[S17.02](https://github.com/cososo-ltd/solid-syslog/issues/493); retired
2026-06-03 under
[S12.26](https://github.com/cososo-ltd/solid-syslog/issues/507). The story
numbers run backwards because they are numbered by epic rather than
chronologically — E12 was elaborated after E17 — so read the dates, not the
labels, for the order of events.
