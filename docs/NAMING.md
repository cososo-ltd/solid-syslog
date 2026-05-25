# Naming conventions

This document defines the identifier naming rules for SolidSyslog. The rules
reconcile three constraints:

1. **MISRA C:2012 uniqueness rules** (5.1 through 5.9) — non-negotiable
   within the strict scope (see Scope below).
2. **Clean Code principles** — identifier length proportional to scope; no
   lazy abbreviations.
3. **Readability at the call site** — a reader should be able to tell, from
   the name alone, roughly where an identifier comes from.

Naming is one slice of the project's coding standard. Rules unrelated to
identifier shape (single-exit-point, parenthesisation, `0U` literal suffixes,
etc.) live in the MISRA standard and `docs/misra-deviations.md`, which are
the authoritative references.

The scheme is enforced by review and by static analysis, with a clean
split between the two tools so they cannot disagree on the same name:

- **clang-tidy** is the sole authority on naming *shape*. The
  `readability-identifier-naming` check understands linkage, scope, and
  the distinction between macros, typedefs, tags and ordinary identifiers.
  Per-directory `.clang-tidy` files implement the tier model below.
  Note: `readability-identifier-naming` enforces case style + prefix +
  suffix per identifier kind, but does **not** support a positive
  must-match regex. The `SolidSyslogClass_Function` shape past the
  `SolidSyslog` prefix is therefore enforced by review and by
  cppcheck-misra rule 5.1 distinctness, not by clang-tidy directly.
- **cppcheck-misra** is the sole authority on naming *uniqueness*. The
  MISRA addon surfaces rules 5.1, 5.2, 5.4, 5.6, 5.7, 5.8 and 5.9
  violations — pattern matching alone cannot detect these.

cppcheck's `naming` addon is deliberately **not** used: it does less than
clang-tidy on every axis we care about, and a second tool checking the
same conditions creates the risk of contradictory verdicts.

---

## Scope

The rules apply with different strictness across the tree:

| Tier | Naming | MISRA | Directories |
|------|--------|-------|-------------|
| **Strict** | Full Tier 1–4 | Full chosen subset | `Core/Interface/`, `Core/Source/`, `Platform/*/Interface/` |
| **Pragmatic** | Tier 1–4 applied; local names that mirror third-party APIs (e.g. `mqd_t mq`, `FIL file`, `SOCKET sock`) and parameters in adapter wrappers are exempt; 5.3 shadowing relaxed when the shadowed name is a third-party identifier | Full chosen subset; per-file deviations documented when third-party APIs force them | `Platform/*/Source/` |
| **Consistency-only** | New code follows conventions; CppUTest macro outputs used as-is; test fakes drop the `SolidSyslog` prefix; no rename sweep of existing test code | Excluded | `Tests/` |
| **Out of scope** | Not enforced | Not enforced | `Bdd/`, `ci/`, `docs/`, `.github/`, `.devcontainer/` |

Enforcement gates (clang-tidy + cppcheck-misra) run per tier with different
rule sets.

---

## MISRA C:2012 rules in play

| Rule | Applies to | Constraint |
|------|------------|------------|
| 5.1  | External identifiers | Distinct in the first **63** characters (project deviation — C99 mandates only 31; see `docs/misra-deviations.md`) |
| 5.2  | Identifiers in the same scope and name space | Distinct in the first 63 characters |
| 5.3  | Inner-scope identifiers | Shall not hide an outer-scope identifier |
| 5.4  | Macro identifiers | Unique within the first 63 characters |
| 5.5  | Macros vs other identifiers | No reuse of the same name |
| 5.6  | Typedef names | Unique across the program |
| 5.7  | Tag names | Unique across the program |
| 5.8  | Identifiers with external linkage | Unique |
| 5.9 (advisory) | Identifiers with internal linkage | Should be unique |

The scheme below satisfies all of these by construction. Rule 5.9 is advisory
in MISRA but treated as required here.

---

## Tier 1 — External linkage (public API)

**Form:** `SolidSyslog<Class>_<Function>` for class-scoped operations,
or `SolidSyslog_<Function>` for whole-library operations where the
library *itself* is the class. `SolidSyslog<Class>` for exported types
and tag names.

```c
/* Class-scoped functions — operate on a specific module.
 * Parameter naming follows the this-pointer rule (Tier 3): `base` when
 * the declared type is the abstract base struct, `self` otherwise. */
void SolidSyslogBuffer_Read(struct SolidSyslogBuffer* base, ...);
int  SolidSyslogTransport_Send(struct SolidSyslogTransport* base, ...);

/* Whole-library functions — operate on the library instance. The
   library is the class; there's nothing more specific to insert.
   Restricted to the small set of API entry points an integrator
   actually calls at the top level: Create / Destroy / Log / Service /
   SetErrorHandler / Error. */
struct SolidSyslog* SolidSyslog_Create(struct SolidSyslogConfig* config);
void                SolidSyslog_Destroy(struct SolidSyslog* self);
void                SolidSyslog_Log(struct SolidSyslog* self, ...);
void                SolidSyslog_Service(struct SolidSyslog* self);
void                SolidSyslog_SetErrorHandler(SolidSyslogErrorHandler handler, void* context);
void                SolidSyslog_Error(SolidSyslogSeverity severity, const char* message);

/* Tag names — note: tag, not typedef. See "No struct typedefs" below. */
struct SolidSyslogBuffer
{
    /* ... */
};

struct SolidSyslogSecurityPolicy
{
    /* ... */
};

/* Public enum constants — SCREAMING_SNAKE with the project prefix.
 * Single rule for tagged and anonymous enums tree-wide. */
enum SolidSyslogSeverity
{
    SOLIDSYSLOG_SEVERITY_EMERGENCY = 0,
    SOLIDSYSLOG_SEVERITY_ALERT     = 1,
    /* ... */
};
```

The `SolidSyslog` prefix is the library's namespace. The `<Class>_`
portion (when present) identifies the module. The function name
follows in PascalCase. The "whole-library" form is **not** an
exception — both shapes are first-class Tier 1; the difference is
whether the operation lives on a specific class or on the library
itself.

**Applies to:** any identifier declared in a header under `Core/Interface/`
or `Platform/*/Interface/`, plus any identifier with external linkage
declared in a `.c` file.

---

## Tier 2 — Internal linkage (file-scope `static`)

**Form:** `Class_Function` for static functions, `Class_Variable` for
file-scope static variables and constants, **bare `PascalCase` for
file-scope struct tags that are never exported**. PascalCase
throughout. No `SolidSyslog` prefix at any Tier 2 site — the file
itself is the namespace.

```c
/* Inside Buffer.c */
static int  Buffer_AppendRecord(struct SolidSyslogBuffer* base, ...);
static void Buffer_ResetCursor(struct SolidSyslogBuffer* base);

static const struct SolidSyslogSecurityPolicy Buffer_DefaultPolicy = { /* ... */ };
static size_t                                 Buffer_ActiveInstanceCount;

/* File-scope helper structs that exist only to give a small group of
   fields a name within this .c file. Not visible to other TUs; not
   declared in any header. The tag is local to the file, so it does
   not carry the SolidSyslog namespace prefix. */
struct EscapedContext
{
    struct SolidSyslogFormatter* Formatter;
    const char*                  Source;
    size_t                       SourcePos;
    /* ... */
};

struct OpenHandle
{
    struct SolidSyslogFile* File;
    size_t                  BlockIndex;
    bool                    Open;
};
```

Rationale:

- Uniqueness across the library is achieved by the `Class_` prefix on
  Tier 2 functions and variables — `Buffer_AppendRecord` cannot
  collide with `Transport_AppendRecord` — which satisfies advisory
  rule 5.9. File-scope struct tags rely on internal-linkage scoping
  to the file (`static` storage classes for any objects of the type),
  which gives them the same uniqueness guarantee without needing a
  prefix.
- The visible difference from Tier 1 is the missing `SolidSyslog`
  prefix, which signals "internal" at the call site without comment.
  Both tiers use PascalCase on both sides of the underscore (or, for
  Tier 2 tags, bare PascalCase), so static helpers and public
  functions read consistently.
- One class per translation unit is the norm; if a `.c` file contains
  helpers for two classes, use both prefixes accordingly.

### Picking the `Class_` prefix from the filename

For source files matching `SolidSyslog<X>.c`, the Tier 2 class prefix
is `<X>_` — the filename with the `SolidSyslog` library namespace
stripped. So `SolidSyslogTlsStream.c` → `TlsStream_*`,
`SolidSyslogPlusTcpTcpStream.c` → `PlusTcpTcpStream_*`,
`SolidSyslogWinsockTcpStream.c` → `WinsockTcpStream_*`. Files whose
basename already drops the library prefix (e.g. `BlockSequence.c`,
`RecordStore.c`) use the basename as the class verbatim →
`BlockSequence_*`, `RecordStore_*`.

The strip-only rule is mechanical and predictable; short-shorthand
prefixes (e.g. `Tls_`, `FrTcp_`, `WinTcp_`) were considered and
rejected — every file would need a hand-picked prefix, the choice
would be hard to predict at a call site, and the convention would
be harder to enforce going forward.

#### The `SolidSyslog.c` exception

`Core/Source/SolidSyslog.c` is the one file where the strip rule
yields an empty prefix (the file *is* the library namespace).
Statics in this file use **`SolidSyslog_<Function>`** — the same
shape as Tier 1 whole-library API entry points (`SolidSyslog_Log`,
`SolidSyslog_Service`, etc.). Linkage (`static`) distinguishes them
at definition; collision risk is zero because only one file can
ever be named `SolidSyslog.c`.

### When a Tier 2 tag DOES carry the `SolidSyslog` prefix

The implementation struct that corresponds to a Tier 1 opaque type
shares the public tag name verbatim. For example, `struct SolidSyslog`
is declared opaquely in `SolidSyslog.h` (Tier 1) and defined
concretely in `SolidSyslog.c`. The .c-side definition is technically
Tier 2 by linkage (it's where the struct's layout lives), but the
tag name is fixed by the Tier 1 public declaration. This is the
opaque-impl pattern; the .c-side use of the tag is not free to
choose its own name.

---

## Tier 3 — Function parameters and block-scope locals

**Form:** lowerCamelCase, no prefix, descriptive but compact. Domain
abbreviations (TLS, UDP, TCP, CRC, RFC, MQ, FAT) are permitted as words.

```c
int Buffer_AppendRecord(struct SolidSyslogBuffer* base,
                        const uint8_t*            record,
                        size_t                    recordLength)
{
    size_t bytesWritten   = 0U;
    size_t bytesRemaining = recordLength;
    /* ... */
}
```

Constraints:

- **Rule 5.3 (no shadowing).** Locals must not share a name with any
  file-scope static, any function parameter, or any enclosing block local.
  In practice this means avoiding generic names like `buffer`, `policy`,
  `state`, `count` inside a file whose statics use those words. Prefer
  `targetBuffer`, `activePolicy`, `currentState`, `recordCount`.
- **No single-letter identifiers.** Use short domain words instead:
  `index`, `count`, `cursor`, `byte`, `next`, `prev`. A loop over records
  is `for (size_t index = 0U; index < recordCount; ++index)`, not
  `for (i = ...)`.
- **No lazy abbreviations.** `buffer` not `buf`, `message` not `msg`,
  `configuration` not `cfg`, `pointer` not `ptr`. Distinguish lazy
  abbreviations from domain terms — the latter are the real names of
  things and should be used unmodified. Domain terms include:
    - RFC field names from specs the library implements (RFC 5424
      `MSG` / `MSGID` / `PRIVAL` / `BOM` / `SD` / `PROCID`).
    - Protocol and technology shorthands (`mq`, `crc`, `tls`, `tcp`,
      `udp`, `ip`, `dns`).
    - POSIX / Win32 idioms that mirror third-party signatures (`fd`,
      `errno`, `pid`, `sock`) — see also the Pragmatic-tier exemption
      in the Scope table for parameter locals in adapter wrappers
      (`buf` / `len` in `send` / `recv` wrappers, `attr` for
      `struct mq_attr`, etc.).
- **No pointer Hungarian.** Never prefix pointer variables with `p`/`P`
  or suffix with `Ptr`. Pointer-ness is visible from the declaration.
- **Booleans.** Predicates and boolean variables use `isX`, `hasX`,
  or `canX` shapes (`isValid`, `hasUnsent`, `canSend`).
- **Out-parameters.** Output parameters use the `outX` prefix
  (`SolidSyslogBuffer_Initialise(..., struct SolidSyslogBuffer** outBuffer)`).

### This-pointer parameters

The first parameter of a method-shaped function — the "this-pointer" — uses
one of two names, chosen by **the declared parameter type**, not by the
function's purpose:

- **`self`** — the declared parameter type is the function's own class
  (the concrete derived type, or for non-vtable classes simply the class).
  Applies to: every helper (`static`/`static inline`); every local
  introduced by a downcast; every public function whose first parameter is
  declared as the concrete class.

- **`base`** — the declared parameter type is the abstract base struct
  (one that exposes vtable function-pointer members — `SolidSyslogBuffer`,
  `SolidSyslogStore`, `SolidSyslogFile`, etc.). Applies to: every vtable
  entry-point implementation; every concrete-class `_Destroy` whose
  declared first parameter is the abstract base; every base-class
  helper or free utility operating polymorphically on the base.

The rule is mechanical: if the declared type is the abstract base, the
name is `base`; otherwise it is `self`. The function's role does not
enter the decision.

#### The downcast: `<Class>_SelfFromBase`

When a vtable entry point or a concrete-class `_Destroy` receives a
`base` and needs to operate on its concrete type, the downcast is named
and centralised — one `static inline` helper per derived class:

```c
static inline struct SolidSyslogCircularBuffer*
CircularBuffer_SelfFromBase(struct SolidSyslogBuffer* base)
{
    return (struct SolidSyslogCircularBuffer*) base;
}
```

Every entry point then reads:

```c
static bool CircularBuffer_Read(struct SolidSyslogBuffer* base,
                                void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);
    ...
}
```

#### The storage cast: `<Class>_SelfFromStorage`

For classes still on the caller-supplied-storage pattern (the
Posix/Windows/FreeRTOS mutexes and streams, FatFs/TLS adapters, …),
`_Create` takes opaque storage and re-interprets it as the concrete
struct. The same convention applies — one named `static inline` helper
per class:

```c
static inline struct SolidSyslogPosixMutex*
PosixMutex_SelfFromStorage(SolidSyslogPosixMutexStorage* storage)
{
    return (struct SolidSyslogPosixMutex*) storage;
}
```

Classes migrated under E11 no longer use this cast — their instance
struct lives in a library-internal static pool, and `_Create` returns
a slot pointer without any storage cast.

Helpers are named per Tier 2 (`Class_Function`, `static inline`, no
`SolidSyslog` prefix). Placement follows the function-ordering rule:
forward-declared with the other helpers at the top of the file,
defined immediately beneath the first caller.

#### Headers

Function-pointer member parameter names inside the public
`SolidSyslog<X>Definition.h` structs follow the same `base` rule —
because the declared type at those member declarations is the abstract
base. C ignores function-pointer parameter names at struct-member
declarations (only the type matters for ABI and callers), so this is a
documentation choice: matching the implementations reduces the
cognitive load on a reader who flips between header and implementation.

#### Shadowing

`self` and `base` are reserved at Tier 3 for the this-pointer role.
Files must not use either name for any other parameter or block-scope
local (avoids MISRA 5.3 shadowing the moment a nested helper is added).
They are also reserved at Tier 2 — no file-scope static should be named
`self` or `base`.

---

## Tier 4 — Struct members

**Form:** PascalCase, no prefix, no class qualifier. No member-kind
exceptions — data members and function-pointer (vtable) members both use
the same shape. The boolean and no-Hungarian conventions from Tier 3
do **not** apply at this tier — PascalCase carries the visual signal that
"this is a named, persistent piece of state" without needing an `is`/`has`
prefix to convey "this is a boolean."

The domain-term exemption from Tier 3 applies equally at Tier 4 —
`struct SolidSyslogMessage`'s members `MessageId` (the full English
word) and `Msg` (RFC 5424's spec label for the body field) are an
example of how the two forms legitimately co-exist when one is an
RFC abbreviation and the other is not.

```c
struct SolidSyslogSecurityPolicy
{
    SolidSyslogIntegrityCheck IntegrityCheck;
    SolidSyslogEncryption     Encryption;
    uint32_t                  MaximumRecordLength;
    bool                      Enabled;
};

/* Vtable function-pointer members follow the same rule — and have done
   so already in practice. The Tier 4 PascalCase convention is the
   project-wide policy that consolidates them. Parameter naming for the
   function-pointer members follows the Tier 3 this-pointer rule: the
   declared type is the abstract base struct, so the parameter is `base`. */
struct SolidSyslogStore
{
    bool (*Write)(struct SolidSyslogStore* base, const void* data, size_t size);
    bool (*ReadNextUnsent)(struct SolidSyslogStore* base, ...);
    void (*MarkSent)(struct SolidSyslogStore* base);
    bool (*HasUnsent)(struct SolidSyslogStore* base);
};
```

### Why PascalCase, and why not member-kind-dependent

PascalCase at the member access site gives a strong visual signal that
something named-and-persistent is being touched, distinct from the
lowerCamelCase of parameters and locals. `buffer->Position += count` reads
clearly as "field write"; `position += count` is a transient local. The
case shape encodes lifetime, not the member's kind.

The previous scheme used lowerCamelCase for data members and tolerated
PascalCase only for vtable function-pointer members "to mirror the
function name." That is the kind of implicit semantic encoding Clean Code
argues against — case meaning shifted based on what *kind* of thing the
member held. Tier 4 now states a single rule.

### The `struct X X;` shape

Because struct tags are also PascalCase (Tier 1), this convention
produces legal declarations like:

```c
struct SolidSyslogBlockStore
{
    struct RecordStore   RecordStore;
    struct BlockSequence BlockSequence;
};
```

The struct tag and the member identifier live in separate C namespaces,
so this is unambiguous to the compiler and to MISRA. To a reader it parses
naturally after one or two exposures — the `struct ` keyword introduces
the type, the trailing identifier is the member.

### Uniqueness within a struct

Struct members live in their own name space, so MISRA does not require
uniqueness across structs. Access at the call site is already qualified
by the struct instance: `policy->IntegrityCheck` is self-documenting.

---

## No struct typedefs

SolidSyslog does **not** use `typedef struct` for its own struct types,
whether the type is opaque or value-typed. Code refers to struct types by
tag everywhere:

```c
/* Yes */
struct SolidSyslogBuffer*               buffer;
const struct SolidSyslogBuffer*         readOnlyBuffer;
struct SolidSyslogSecurityPolicy        policy;
const struct SolidSyslogSecurityPolicy* readOnlyPolicy;

/* No — do not introduce a typedef for any struct type */
typedef struct SolidSyslogBuffer         SolidSyslogBuffer;
typedef struct SolidSyslogBuffer*        SolidSyslogBufferHandle;
typedef const struct SolidSyslogBuffer*  SolidSyslogBufferConstHandle;
```

There are three reasons:

1. **Readability.** A typedef'd struct value type hides that it is a struct,
   and a typedef'd pointer-to-struct hides one level of indirection. Both
   make pointer and `const` placement ambiguous to the reader. The tag form
   keeps the type's nature visible at every declaration.

2. **MISRA Rule 5.6 plus header coupling.** A typedef is a *definition* and
   under Rule 5.6 must appear exactly once across the program — so it has to
   live in one canonical header. Any translation unit that wants to reference
   the typedef'd name must `#include` that header. By contrast, a tag
   forward declaration `struct SolidSyslogBuffer;` is not a definition and
   may be repeated wherever it is needed. Sticking to tags lets headers
   reference each other's types via local forward declarations and avoids a
   dense `#include` graph.

3. **The `const` trap.** `const SolidSyslogBufferHandle` does not mean
   "pointer to const buffer" — it means "const pointer to non-const buffer",
   because the `const` qualifies the typedef'd pointer rather than the
   pointee. The tag form `const struct SolidSyslogBuffer*` cannot be
   misread.

### Forward declarations are encouraged

In any header that needs to reference a struct only through a pointer, use
a local forward declaration rather than `#include`-ing the defining header.
This minimises header coupling and keeps compilation fast:

```c
/* SolidSyslogTransport.h — does not need to see Buffer's layout */
struct SolidSyslogBuffer;

int SolidSyslogTransport_Send(struct SolidSyslogTransport*    transport,
                              const struct SolidSyslogBuffer* source);
```

### Typedefs that are permitted

Typedefs are used **only** for:

- **Enum types** intended to be passed by value:
  `typedef enum SolidSyslogSeverity SolidSyslogSeverity;`
- **Function pointer types** in vtables:
  `typedef int (*SolidSyslogTransport_SendFn)(struct SolidSyslogTransport*, ...);`
- **Scalar aliases** where the underlying type is an implementation detail.
  Use sparingly.

---

## Macros

**Form:** `SOLIDSYSLOG_SCREAMING_SNAKE_CASE` for public macros,
`CLASS_SCREAMING_SNAKE_CASE` for file-scope macros.

```c
/* Public, in a header */
#define SOLIDSYSLOG_MAXIMUM_RECORD_LENGTH 2048U

/* File-scope, in Buffer.c */
#define BUFFER_RECORD_MAGIC 0xA53CU
```

Rule 5.4 requires macro uniqueness; rule 5.5 forbids reuse of a name as both
a macro and a non-macro identifier. The all-caps convention plus the prefix
handles both. (Note: macros use the joined `SOLIDSYSLOG_` form rather than
`SOLID_SYSLOG_` to match the existing codebase and avoid first-N-character
pressure from rule 5.4.)

**Exceptions:** CppUTest's `TEST`, `TEST_GROUP`, `CHECK_*`, `LONGS_EQUAL`,
etc. are used unmodified — see Tests below.

### Enum constants

All enum constants — tagged or anonymous, public or TU-local — are
`SCREAMING_SNAKE`. One rule, mechanically enforced by clang-tidy
(`EnumConstantCase: UPPER_CASE`, no exceptions).

```c
/* Tagged public enum — Tier 1 type with named members */
enum SolidSyslogSeverity
{
    SOLIDSYSLOG_SEVERITY_EMERGENCY = 0,
    SOLIDSYSLOG_SEVERITY_ALERT     = 1,
    /* ... */
};

/* Anonymous public enum — Tier 1 macro-equivalent */
enum
{
    SOLIDSYSLOG_CIRCULAR_BUFFER_OVERHEAD = 7,
    SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES = sizeof(uint16_t)
};

/* Anonymous TU-local enum — Tier 2 macro-equivalent */
enum
{
    HEADER_BYTES = SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES
};
```

**Word boundaries.** Snake-separate at every CamelCase boundary in the
source identifier (`DatagramSendResult` → `DATAGRAM_SEND_RESULT`,
`AuthPriv` → `AUTH_PRIV`). Trailing digits stay glued to the preceding
word (`Local0` → `LOCAL0`).

**Project prefix.** Public sites (anywhere visible outside a single
TU) carry `SOLIDSYSLOG_`. TU-local anonymous-enum constants
(`IPV4_HEADER_BYTES`, `UINT32_MAX_DECIMAL_DIGITS`) don't. clang-tidy
cannot distinguish public from TU-local enum constants syntactically;
the prefix rule for public sites is enforced by review and by
cppcheck-misra rule 5.4 distinctness.

The anonymous-enum named-constant idiom is itself unchanged — it's
still the project's type-safe alternative to `#define` for integer
constants, distinct in shape from tagged enums in purpose if not in
casing. MISRA rule 2.4 (unused tag declarations) cppcheck-fires on
anonymous enums; the project-wide deviation **D.009** covers all such
sites — see `docs/misra-deviations.md#d009`.

This single-rule convention replaces the S10.07/S10.12 split where
tagged-enum constants used `SolidSyslog<Class>_Constant` (PascalCase,
class-scoped) and anonymous-enum constants used SCREAMING_SNAKE. The
split was hard to enforce mechanically (only judgement separated the
two cases) and was drifting in S10.16; S10.22 collapsed it.

---

## Tests

Test code uses production conventions where natural, with these relaxations:

- **lowerCamelCase locals** preferred but not enforced; no rename sweep of
  existing test code.
- **PascalCase for static test helpers** when present
  (e.g. `SpyGetHost`, `GetDefaultPort`).
- **CppUTest macros** (`TEST`, `TEST_GROUP`, `TEST_GROUP_BASE`, `TEST_BASE`,
  `CHECK_*`, `LONGS_EQUAL`, etc.) are used as-is — the identifiers they
  expand to (e.g. `TEST_GroupName_TestName_TestShell`) are exempt from
  Tier 1 and routinely exceed any character limit.
- **Test-helper macros** in test translation units (`CALLED_FAKE`,
  `CALLED_DATAGRAM_SEND`, `CHECK_REPORTED_ERROR`, etc.) use whatever
  SCREAMING_SNAKE shape reads well; no project prefix required.
- **Test fakes and spies** drop the `SolidSyslog` prefix
  (e.g. `SocketFake_Reset`, `DatagramFake_SendCallCount`) so they read
  obviously as test infrastructure at the call site.
- **Test group names** carry a `Test` suffix and the `SolidSyslog` prefix
  because `TEST_GROUP(...)` macros expand to external-linkage identifiers
  (rule 5.8). Three forms are permitted:

  ```c
  /* Class-level group, when the tests cover the class as a whole */
  TEST_GROUP(SolidSyslogBufferTest) { /* ... */ };

  /* Function-level group, when a single function deserves its own group */
  TEST_GROUP(SolidSyslogBuffer_AppendRecordTest) { /* ... */ };

  /* Integration group, exercising more than one class */
  TEST_GROUP(SolidSyslogIntegrationTlsStoreAndForward) { /* ... */ };
  ```

- **Test case names** use UpperCamelCase describing the behaviour:

  ```c
  TEST(SolidSyslogBufferTest, AppendsRecordWhenSpaceAvailable)
  {
      /* ... */
  }
  ```

No MISRA rules apply to test code. Test code converges to these
relaxations organically as it is touched, not via a sweep.

---

## Worked example

A small slice showing every tier in one place, including the derived-class
vtable shape with `SelfFromBase` / `SelfFromStorage` helpers.

```c
/* Core/Interface/SolidSyslogBufferDefinition.h -------------------------- */

/* Tier 1 — abstract base struct with vtable function-pointer members.
   Function-pointer parameter names are `base` (Tier 3 this-pointer rule:
   declared type is the abstract base). */
struct SolidSyslogBuffer
{
    void (*Write)(struct SolidSyslogBuffer* base, const void* data, size_t size);
    bool (*Read)(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
};

/* Core/Interface/SolidSyslogCircularBuffer.h ---------------------------- */

#define SOLIDSYSLOG_CIRCULAR_BUFFER_STORAGE_SIZE_BYTES(bytes) /* ... */

typedef size_t SolidSyslogCircularBufferStorage;

/* Tier 1 — public Create returns the base-class view.
   _Destroy takes the base type (matches the abstract Buffer contract),
   so its parameter is `base`. */
struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    SolidSyslogCircularBufferStorage* storage, size_t storageBytes, struct SolidSyslogMutex* mutex
);
void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* base);

/* Core/Source/SolidSyslogCircularBuffer.c ------------------------------- */

#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogBufferDefinition.h"

/* Tier 2 — concrete struct definition (uses the public tag verbatim per
   the opaque-impl pattern). */
struct SolidSyslogCircularBuffer
{
    struct SolidSyslogBuffer Base;
    /* ... per-instance state ... */
};

/* Tier 2 — vtable entry points: declared type is the abstract base, so
   parameters are `base`. */
static bool CircularBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead);
static void CircularBuffer_Write(struct SolidSyslogBuffer* base, const void* data, size_t size);

/* Tier 2 — named downcast helpers, one per cast type. */
static inline struct SolidSyslogCircularBuffer*
CircularBuffer_SelfFromStorage(SolidSyslogCircularBufferStorage* storage);
static inline struct SolidSyslogCircularBuffer*
CircularBuffer_SelfFromBase(struct SolidSyslogBuffer* base);

/* Tier 2 — internal helpers: declared type is the concrete class, so
   parameters are `self`. */
static inline bool CircularBuffer_IsEmpty(const struct SolidSyslogCircularBuffer* self);

struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
    SolidSyslogCircularBufferStorage* storage, size_t storageBytes, struct SolidSyslogMutex* mutex
)
{
    /* Tier 3 — `self` is the concrete-class this-pointer obtained from the
       caller-supplied storage. */
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromStorage(storage);
    self->Base.Read  = CircularBuffer_Read;
    self->Base.Write = CircularBuffer_Write;
    /* ... */
    return &self->Base;
}

static inline struct SolidSyslogCircularBuffer*
CircularBuffer_SelfFromStorage(SolidSyslogCircularBufferStorage* storage)
{
    return (struct SolidSyslogCircularBuffer*) storage;
}

void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);
    self->Base.Read  = NULL;
    self->Base.Write = NULL;
    /* ... */
}

static inline struct SolidSyslogCircularBuffer*
CircularBuffer_SelfFromBase(struct SolidSyslogBuffer* base)
{
    return (struct SolidSyslogCircularBuffer*) base;
}

static bool CircularBuffer_Read(struct SolidSyslogBuffer* base, void* data, size_t maxSize, size_t* bytesRead)
{
    /* Tier 3 — `base` is the abstract-base this-pointer the vtable hands us;
       downcast names the concrete view as `self`. */
    struct SolidSyslogCircularBuffer* self = CircularBuffer_SelfFromBase(base);

    bool delivered = !CircularBuffer_IsEmpty(self);
    /* ... */
    return delivered;
}

static inline bool CircularBuffer_IsEmpty(const struct SolidSyslogCircularBuffer* self)
{
    /* ... */
}
```

---

## Quick reference

| Identifier kind                       | Form                                       | Example                                    |
|---------------------------------------|--------------------------------------------|--------------------------------------------|
| Public function                       | `SolidSyslogClass_Function`                | `SolidSyslogBuffer_Append`                 |
| Public struct tag                     | `SolidSyslogClass`                         | `struct SolidSyslogBuffer`                 |
| Public enum type                      | `SolidSyslogClass`                         | `enum SolidSyslogSeverity`                 |
| Public enum constant                  | `SOLIDSYSLOG_CLASS_CONSTANT`               | `SOLIDSYSLOG_SEVERITY_EMERGENCY`            |
| Public macro                          | `SOLIDSYSLOG_SCREAMING_SNAKE`              | `SOLIDSYSLOG_MAXIMUM_RECORD_LENGTH`        |
| Public typedef (enum/fn-pointer only) | `SolidSyslogClass` / `SolidSyslogClass_Fn` | `SolidSyslogTransport_SendFn`              |
| Static function                       | `Class_Function`                           | `Buffer_WriteMagic`                        |
| Static variable / constant            | `Class_Variable`                           | `Buffer_DefaultPolicy`                     |
| File-scope macro                      | `CLASS_SCREAMING_SNAKE`                    | `BUFFER_RECORD_MAGIC`                      |
| Function parameter / local            | `lowerCamelCase`                           | `recordLength`, `bytesAvailable`           |
| This-pointer parameter                | `self` (own type) / `base` (abstract base) | `* self` in helpers; `* base` in vtable impls |
| Downcast helper                       | `Class_SelfFromBase` / `Class_SelfFromStorage` | `CircularBuffer_SelfFromBase`         |
| Out-parameter                         | `outX` prefix                              | `outBuffer`                                |
| Boolean / predicate                   | `isX` / `hasX` / `canX`                    | `isValid`, `hasUnsent`                     |
| Loop variable                         | short domain word, lowerCamelCase          | `index`, `count`, `cursor`                 |
| Struct member                         | `PascalCase`                               | `WriteCursor`, `IntegrityCheck`, `Write` (function-pointer member) |
| Test group (class)                    | `SolidSyslogClassTest`                     | `SolidSyslogBufferTest`                    |
| Test group (function)                 | `SolidSyslogClass_FunctionTest`            | `SolidSyslogBuffer_AppendTest`             |
| Test group (integration)              | `SolidSyslogIntegrationDescription`        | `SolidSyslogIntegrationTlsStoreAndForward` |
| Test case                             | `UpperCamelCaseSentence`                   | `AppendsRecordWhenSpaceAvailable`          |
