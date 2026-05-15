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
/* Class-scoped functions — operate on a specific module. */
void SolidSyslogBuffer_Read(struct SolidSyslogBuffer* buffer, ...);
int  SolidSyslogTransport_Send(struct SolidSyslogTransport* transport, ...);

/* Whole-library functions — operate on the library instance. The
   library is the class; there's nothing more specific to insert.
   Restricted to the small set of API entry points an integrator
   actually calls at the top level: Create / Destroy / Log / Service /
   SetErrorHandler / Error. */
struct SolidSyslog* SolidSyslog_Create(struct SolidSyslogConfig* config);
void                SolidSyslog_Destroy(struct SolidSyslog* solidSyslog);
void                SolidSyslog_Log(struct SolidSyslog* solidSyslog, ...);
void                SolidSyslog_Service(struct SolidSyslog* solidSyslog);
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

/* Public enum constants follow the same prefix rule. PascalCase
 * (rather than SCREAMING_SNAKE) is what disambiguates them from
 * macros (rule 5.5). */
enum SolidSyslogSeverity
{
    SolidSyslogSeverity_Emergency = 0,
    SolidSyslogSeverity_Alert     = 1,
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
static int  Buffer_AppendRecord(struct SolidSyslogBuffer* buffer, ...);
static void Buffer_ResetCursor(struct SolidSyslogBuffer* buffer);

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
int Buffer_AppendRecord(struct SolidSyslogBuffer* buffer,
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
  things and should be used unmodified (e.g. `mq`, `crc`, `tls`).
- **No pointer Hungarian.** Never prefix pointer variables with `p`/`P`
  or suffix with `Ptr`. Pointer-ness is visible from the declaration.
- **Booleans.** Predicates and boolean variables use `isX`, `hasX`,
  or `canX` shapes (`isValid`, `hasUnsent`, `canSend`).
- **Out-parameters.** Output parameters use the `outX` prefix
  (`SolidSyslogBuffer_Initialise(..., struct SolidSyslogBuffer** outBuffer)`).

---

## Tier 4 — Struct members

**Form:** PascalCase, no prefix, no class qualifier. No member-kind
exceptions — data members and function-pointer (vtable) members both use
the same shape. The boolean and no-Hungarian conventions from Tier 3
do **not** apply at this tier — PascalCase carries the visual signal that
"this is a named, persistent piece of state" without needing an `is`/`has`
prefix to convey "this is a boolean."

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
   project-wide policy that consolidates them. */
struct SolidSyslogStore
{
    bool (*Write)(struct SolidSyslogStore* self, const void* data, size_t size);
    bool (*ReadNextUnsent)(struct SolidSyslogStore* self, ...);
    void (*MarkSent)(struct SolidSyslogStore* self);
    bool (*HasUnsent)(struct SolidSyslogStore* self);
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

A small slice showing every tier in one place:

```c
/* Core/Interface/SolidSyslogBuffer.h ------------------------------------ */

#define SOLIDSYSLOG_BUFFER_MINIMUM_CAPACITY 64U

/* Opaque forward declaration — the layout lives in SolidSyslogBuffer.c */
struct SolidSyslogBuffer;

/* Forward declaration of a value-typed companion struct.
   SolidSyslogTransport.h would only need this much to take a pointer,
   even though the layout is defined elsewhere. */
struct SolidSyslogSecurityPolicy;

/* Caller supplies storage for the buffer object itself */
size_t SolidSyslogBuffer_RequiredStorageBytes(void);

int SolidSyslogBuffer_Initialise(void*                                    storage,
                                 size_t                                   storageLength,
                                 const struct SolidSyslogSecurityPolicy*  policy,
                                 struct SolidSyslogBuffer**               outBuffer);

int  SolidSyslogBuffer_Append(struct SolidSyslogBuffer* buffer,
                              const uint8_t*            record,
                              size_t                    recordLength);

bool SolidSyslogBuffer_IsEmpty(const struct SolidSyslogBuffer* buffer);

/* Core/Source/SolidSyslogBuffer.c --------------------------------------- */

#include "SolidSyslogBuffer.h"

#define BUFFER_RECORD_MAGIC 0xA53CU

struct SolidSyslogBuffer
{
    uint8_t* Storage;
    size_t   Capacity;
    size_t   WriteCursor;
    size_t   ReadCursor;
    const struct SolidSyslogSecurityPolicy* Policy;
};

static int Buffer_WriteMagic(struct SolidSyslogBuffer* buffer);
static int Buffer_WriteRecord(struct SolidSyslogBuffer* buffer,
                              const uint8_t*            record,
                              size_t                    recordLength);

static const uint16_t Buffer_RecordOverhead = 6U; /* magic + length */

int SolidSyslogBuffer_Append(struct SolidSyslogBuffer* buffer,
                             const uint8_t*            record,
                             size_t                    recordLength)
{
    size_t bytesRequired  = recordLength + Buffer_RecordOverhead;
    size_t bytesAvailable = buffer->Capacity - buffer->WriteCursor;
    int    result;

    if (bytesRequired > bytesAvailable)
    {
        result = SolidSyslogResult_BufferFull;
    }
    else
    {
        int status = Buffer_WriteMagic(buffer);
        if (status != SolidSyslogResult_Ok)
        {
            result = status;
        }
        else
        {
            result = Buffer_WriteRecord(buffer, record, recordLength);
        }
    }
    return result;
}

/* Tests/SolidSyslogBufferTest.cpp --------------------------------------- */

TEST_GROUP(SolidSyslogBufferTest)
{
    struct SolidSyslogBuffer* buffer;
    uint8_t                   storage[256];

    void setup() override
    {
        struct SolidSyslogSecurityPolicy policy = { /* ... */ };
        (void) SolidSyslogBuffer_Initialise(storage, sizeof storage,
                                            &policy, &buffer);
    }

    void teardown() override
    {
        /* ... */
    }
};

TEST(SolidSyslogBufferTest, AppendsRecordWhenSpaceAvailable)
{
    /* ... */
}

TEST(SolidSyslogBufferTest, RejectsRecordWhenFull)
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
| Public enum constant                  | `SolidSyslogClass_Constant`                | `SolidSyslogSeverity_Emergency`            |
| Public macro                          | `SOLIDSYSLOG_SCREAMING_SNAKE`              | `SOLIDSYSLOG_MAXIMUM_RECORD_LENGTH`        |
| Public typedef (enum/fn-pointer only) | `SolidSyslogClass` / `SolidSyslogClass_Fn` | `SolidSyslogTransport_SendFn`              |
| Static function                       | `Class_Function`                           | `Buffer_WriteMagic`                        |
| Static variable / constant            | `Class_Variable`                           | `Buffer_DefaultPolicy`                     |
| File-scope macro                      | `CLASS_SCREAMING_SNAKE`                    | `BUFFER_RECORD_MAGIC`                      |
| Function parameter / local            | `lowerCamelCase`                           | `recordLength`, `bytesAvailable`           |
| Out-parameter                         | `outX` prefix                              | `outBuffer`                                |
| Boolean / predicate                   | `isX` / `hasX` / `canX`                    | `isValid`, `hasUnsent`                     |
| Loop variable                         | short domain word, lowerCamelCase          | `index`, `count`, `cursor`                 |
| Struct member                         | `PascalCase`                               | `WriteCursor`, `IntegrityCheck`, `Write` (function-pointer member) |
| Test group (class)                    | `SolidSyslogClassTest`                     | `SolidSyslogBufferTest`                    |
| Test group (function)                 | `SolidSyslogClass_FunctionTest`            | `SolidSyslogBuffer_AppendTest`             |
| Test group (integration)              | `SolidSyslogIntegrationDescription`        | `SolidSyslogIntegrationTlsStoreAndForward` |
| Test case                             | `UpperCamelCaseSentence`                   | `AppendsRecordWhenSpaceAvailable`          |
