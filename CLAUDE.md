# Claude Code Guidelines

## Read this with SKILL.md

Also read [`SKILL.md`](SKILL.md) at session start. It holds cross-context conventions
that the VSCode extension does not auto-load — notably the **TDD pairing contract**.
This file (CLAUDE.md) covers implementation and workflow details; SKILL.md covers
the "why" and the habits that span Windows host / WSL / container sessions.

## Git Workflow

All changes to `main` must go via a pull request — direct pushes are blocked by branch protection.

**Branch naming:** `<type>/<short-description>` — e.g. `feat/clang-preset`, `ci/pin-action-shas`

**Merge strategy:** Squash merge only. This keeps a linear history on `main` and means the PR title
becomes the single commit message — so the PR title must follow Conventional Commits format (see below).

**Before raising a PR — see [docs/local-checks.md](docs/local-checks.md)** for the
full tiered pre-PR check budget. One-line summary:

- Per-commit: `debug` build + tests for the matching preset (~30–60 s)
- Pre-push (only when production source changed): IWYU + `clang-format` reflow + `scripts/misra_renumber.py` (~3–5 min)
- Everything else (`tidy`, `sanitize`, `coverage`, Windows, BDD, integration, FreeRTOS host/cross) — CI's job; do not run locally
- If CI surfaces a finding you missed, fix in another commit on the same branch rather than re-running every lane locally

Commits on the branch can be informal (WIP messages are fine). The PR title is
what matters — it becomes the permanent commit message on `main` on squash merge.

**Branch protection rules (configured on GitHub):**

- Direct pushes to `main` are blocked
- PRs require all status checks to pass before merging: build-linux-gcc, build-linux-clang, build-windows-msvc, sanitize-linux-gcc, coverage-linux-gcc, analyze-tidy, analyze-cppcheck, analyze-format, analyze-iwyu, integration-linux-openssl, integration-linux-mbedtls, integration-windows-openssl, bdd-linux-syslog-ng, bdd-windows-otel, bdd-freertos-qemu-plustcp, build-freertos-host-tdd-plustcp, build-freertos-target-plustcp, analyze-tidy-freertos-plustcp, analyze-iwyu-freertos-plustcp, bdd-freertos-qemu-lwip, build-freertos-target-lwip, analyze-tidy-freertos-lwip, analyze-iwyu-freertos-lwip, docs-build, summary
- Squash merge only — other merge strategies are disabled
- Branches are deleted automatically after merge

When cloning this template, reconfigure these branch protection rules on the new repository.

---

## Issue / Epic Linking

A line like `Parent epic: #5` in an issue body is for human readers only — it does **not**
create a GitHub sub-issue relationship. The epic's sub-issue summary, the roll-up percentage,
and the "child issues" column on the project board are all driven by GitHub's native sub-issue
feature, which is only reachable via the GraphQL API.

When creating a new story under an epic, link it immediately after `gh issue create`:

```bash
# 1. Get the node IDs (epic and story).
gh api graphql -f query='
query {
  repository(owner: "cososo-ltd", name: "solid-syslog") {
    epic:  issue(number: <EPIC_NUM>)  { id }
    story: issue(number: <STORY_NUM>) { id }
  }
}'

# 2. Link the story as a sub-issue of the epic.
gh api graphql -f query='
mutation {
  addSubIssue(input: {issueId: "<EPIC_NODE_ID>", subIssueId: "<STORY_NODE_ID>"}) {
    subIssue { number }
  }
}'

# 3. Verify.
gh api graphql -f query='
query {
  repository(owner: "cososo-ltd", name: "solid-syslog") {
    issue(number: <EPIC_NUM>) {
      subIssuesSummary { total completed }
      subIssues(first: 50) { nodes { number title state } }
    }
  }
}'
```

Always prefer this GraphQL wiring over `gh issue develop` or textual `Parent epic:` lines —
those leave the epic's sub-issue list empty and the project board incomplete. If an audit
turns up orphan stories (body references an epic but the epic's `subIssues` list doesn't
include them), run `addSubIssue` retroactively; it's idempotent-safe on closed issues.

---

## Project Board Membership

The `SolidSyslog` project board (`gh project list --owner DavidCozens` → project 1) has a
`Status` single-select field with options **Todo**, **In Progress**, **Done**. Adding an
issue to the repo does **not** add it to the board — that is a separate step and must be
done explicitly.

### Convention

- **Epics are never added to the project as items.** The board is grouped by the native
  *Parent issue* field — every story under the same epic forms a swimlane, and the
  swimlane header renders the epic's title automatically. Adding the epic itself as an
  item would make it appear a second time as an orphan row in the "no parent" lane, so
  don't do it.
- **Unstarted epic** → nothing on the board. No items, no swimlane. The swimlane only
  exists once at least one child story is on the board.
- **Started epic** → the swimlane appears the moment its first story is added to the
  board. There is no Status field for the epic itself (it isn't an item) — the epic's
  state is inferred visually from its stories' columns. `subIssuesSummary.percentCompleted`
  on the epic gives the same roll-up numerically.
- **All stories belonging to a started epic** → on the board as items, regardless of
  state. Closed → `Done`. Open and not yet begun → `Todo`. Working → `In Progress`.
- When the board fills up, Done stories are archived manually as needed — archival is a
  housekeeping step, not a status transition. Archived items stay on the project and still
  count in the epic's sub-issue roll-up.

### Add-to-board recipe

```bash
# Project and Status field IDs (stable for this repo):
#   projectId   = PVT_kwHOAPhEnM4BTETq
#   statusField = PVTSSF_lAHOAPhEnM4BTETqzhAat7w
#   options     = Todo:f75ad846  In Progress:47fc9ee4  Done:98236657

# 1. Get the issue's node ID.
gh api graphql -f query='
query {
  repository(owner: "cososo-ltd", name: "solid-syslog") {
    issue(number: <N>) { id }
  }
}'

# 2. Add to project (returns the new project item's id).
gh api graphql -f query='
mutation {
  addProjectV2ItemById(input: {
    projectId: "PVT_kwHOAPhEnM4BTETq",
    contentId: "<ISSUE_NODE_ID>"
  }) { item { id } }
}'

# 3. Set Status (use the option id matching Todo / In Progress / Done).
gh api graphql -f query='
mutation {
  updateProjectV2ItemFieldValue(input: {
    projectId: "PVT_kwHOAPhEnM4BTETq",
    itemId:    "<ITEM_ID>",
    fieldId:   "PVTSSF_lAHOAPhEnM4BTETqzhAat7w",
    value:     {singleSelectOptionId: "<OPTION_ID>"}
  }) { projectV2Item { id } }
}'
```

### Issue and story number format

Epic and story numbers in **issue titles and issue bodies** are zero-padded to two
digits so the board and issue lists sort in numeric order (GitHub sorts alphabetically
by title). Convention:

- Epics: `E00`, `E01`, …, `E20`, …, `E99`.
- Stories: `S<EE>.<NN>` — both parts always two digits. e.g. `S03.07`, `S13.09`.

This applies to:

- New issue titles (`gh issue create --title "S<EE>.<NN>: …"`).
- Cross-references inside issue bodies (epic story tables, "Parent epic", etc.).
- CLAUDE.md and memory files.

This does **not** apply to:

- Commit messages and branch names — historical commits use the unpadded form, and
  we don't rewrite history. Going forward both forms are acceptable, padded preferred
  for consistency with what the referenced issue will show. Readers treat `S3.7` and
  `S03.07` as equivalent; real linking is by issue number.
- Code, ADRs, or other repo docs — these reference story numbers only incidentally.

### New-story checklist

For every new story:

1. `gh issue create --title "S<EE>.<NN>: …" --label story` — creates the issue.
   Pad both the epic and story numbers to two digits.
2. `addSubIssue` it under the parent epic (see **Issue / Epic Linking** above). The
   Parent-issue link is what groups the story into the correct swimlane.
3. Add the story to the project board with `Status = Todo` using the recipe above. Do
   **not** add the parent epic itself — the swimlane appears automatically once a child
   story is on the board.

### When closing a story

When a PR merges and closes a story, flip its project Status to `Done`. Closing the
issue does not update the Status field automatically.

Epics don't have a Status field on the board (they aren't items). When every child
story is Done, the swimlane naturally becomes all-Done; the epic issue itself should
also be closed on GitHub. Check `subIssuesSummary.percentCompleted` on the epic if you
need a numeric roll-up.

---

## Commit Messages

All commit messages must follow [Conventional Commits](https://www.conventionalcommits.org/) format.
This drives automated changelog generation and release versioning via release-please.

```text
<type>[!]: <description>

[optional body]
```

| Type | Use for |
|---|---|
| `feat` | New functionality |
| `fix` | Bug fix |
| `ci` | CI/build/tooling changes |
| `refactor` | Code restructuring without behaviour change |
| `chore` | Maintenance (e.g. container image bump) |
| `docs` | Documentation only |

Append `!` for breaking changes: `feat!: rename Example target`.

PR titles must also follow this format — on squash merge the PR title becomes the commit message.

---

## TDD Discipline

Follow Uncle Bob's Three Rules of TDD strictly — **red/green/refactor in strict order**:

- **Red**: write the simplest failing test. Use inline literal values — do not introduce named constants
  or helpers at this step. Compilation failures count as failures.
- **Green**: write the minimum production code to pass the test. Hard-coded values are correct here.
- **Refactor**: while green, extract named constants, helpers, and DRY improvements. This is the right
  time to introduce `TEST_*` constants, field index constants, and test helpers.

### Test Defaults Pattern

For walking skeleton stories, use hard-coded "test default" values that are obviously fake
(e.g. `TestHost`, `42`, the RFC 5424 publication date `2009-03-23T00:00:00.000Z`). Name them `TEST_*`.
These are baked into production code initially; later stories drive real values in via config injection.

Named constants and test helpers emerge through the refactor step — never introduced upfront.

### Three Laws

1. You may not write production code unless it is to make a failing unit test pass.
2. You may not write more of a unit test than is sufficient to fail — compilation failures are failures.
3. You may not write more production code than is sufficient to pass the one failing unit test.

Refactoring must follow SOLID and DRY principles:

- **Single Responsibility** — one reason to change per module/class
- **Open/Closed** — open for extension, closed for modification
- **Liskov Substitution** — subtypes must be substitutable for their base types
- **Interface Segregation** — prefer narrow, focused interfaces
- **Dependency Inversion** — depend on abstractions, not concretions
- **DRY** — every piece of knowledge has a single, authoritative representation

The target is 100% line and branch coverage. The CI gate is 90% — if coverage drops below that, the build fails.
If 100% is proving difficult to achieve, the first response should be to reconsider the design, not lower the bar.
In practice, following TDD strictly means 100% is the natural outcome. Exceptions exist but are rare; if you find
yourself needing one, discuss the design first.

---

## CMake Presets

| Preset | Purpose |
|---|---|
| `debug` | Standard debug build — primary development preset |
| `clang-debug` | Clang build — portability check against GCC |
| `sanitize` | ASan + UBSan — run regularly during development |
| `coverage` | lcov/genhtml — 100% line and branch required |
| `tidy` | clang-tidy — all warnings treated as errors |
| `cppcheck` | cppcheck static analysis |
| `msvc-debug` | MSVC build — Windows portability check (requires vcpkg) |
| `release` | Release build — optimisations enabled, no instrumentation |

Build and test: `cmake --preset <name> && cmake --build --preset <name> --target junit`
Coverage report: `cmake --preset coverage && cmake --build --preset coverage --target coverage`

---

## Project Structure

```text
Core/Interface/     — Public headers of the core library. No implementation. This is the API boundary.
Core/Source/        — Core library implementation. Compiled into a static library.
Platform/           — Platform-specific code (Posix, Windows, OpenSsl) — each a subfolder with its own Interface/ and Source/.
Tests/              — CppUTest unit tests. Never link production code directly; always via the library.
Tests/Support/      — PosixFakes static lib (SocketFake, ClockFake) — shared across test executables.
Tests/Bdd/Targets/  — BDD target code unit tests (BddTargetTests executable).
Bdd/                — BDD test infrastructure: Gherkin features, step definitions, syslog-ng config.
Bdd/Targets/        — One BDD-driven binary per platform (Common, Linux, Windows, FreeRtos) — all named SolidSyslogBddTarget. Not pedagogical examples.
ci/                 — CI-specific files (e.g. docker-compose.bdd.yml).
docs/               — Project documentation.
```

### Support tiers

Not all directories carry the same review rigour or long-term support commitment.

| Tier | Scope | Directories |
|---|---|---|
| 1 | Full support, highest review bar, stable API | `Core/Interface/`, `Core/Source/` |
| 2 | Supported, API may evolve per target platform | `Platform/*/` |
| 3 | Best-effort BDD targets | `Bdd/Targets/` |
| — | Out of scope for support | `Tests/`, `Bdd/`, `docs/`, `ci/`, `.github/`, `.devcontainer/`, build tooling |

The separation between `Core/Interface/` and `Core/Source/` is deliberate — it enforces the dependency inversion
boundary that makes the code testable and portable to embedded targets.

### Public header audiences (Interface Segregation)

Public headers are split by audience — each user includes only what they need. Most headers
live under `Core/Interface/`; platform-specific helpers (the `SolidSyslogPosix*` and
`SolidSyslogWindows*` rows below) live under `Platform/Posix/Interface/` and
`Platform/Windows/Interface/` respectively.

| Header | Audience | Provides |
|---|---|---|
| `SolidSyslog.h` | Application code that logs events | `SolidSyslogMessage`, `SolidSyslog_Log(handle, message)`, `SolidSyslog_LogWithSd(handle, message, sd, sdCount)` (attach caller-built per-message SD-ELEMENTs after the per-instance SDs; `Log` is exactly this with `NULL, 0`), `SolidSyslog_Service(handle)`. `struct SolidSyslog` opaque. |
| `SolidSyslogConfig.h` | System setup code | `SolidSyslogConfig`, `SolidSyslog_Create(config) → handle`, `SolidSyslog_Destroy(handle)`. Instance struct lives in a library-internal static pool (E11; tunable `SOLIDSYSLOG_POOL_SIZE`, default `1U`). Pool-exhaustion fallback is a TU-private `NullInstance` wired to `SolidSyslogNull{Buffer,Sender,Store}_Get()` siblings — `_Log`/`_Service` against it dispatch through Null vtables and silently drop. |
| `SolidSyslogErrors.h` | Any code installing an error handler that wants to react to SolidSyslog-singleton events (pointer-identity match on `SolidSyslogErrorSource`, switch on `enum SolidSyslogErrors`) | `enum SolidSyslogErrors` (`SOLIDSYSLOG_ERROR_*` codes + `SOLIDSYSLOG_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource SolidSyslogErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogError.h` | Any code installing a handler to react to library-internal errors (NULL guards, send failures); also the library-internal call site for emitting them | `struct SolidSyslogErrorEvent` (`Severity`, `Source`, `uint16_t Category`, `int32_t Detail`), `SolidSyslogErrorHandler` typedef (`void(void* context, const struct SolidSyslogErrorEvent* event)`), `SolidSyslog_SetErrorHandler(handler, context)`, `SolidSyslog_Error(severity, source, category, detail)`. The handler reads three orthogonal axes off the event: `Source` (extensible identity, pointer-identity match), `Category` (portable reaction axis — see `SolidSyslogErrorCategory.h`), `Detail` (per-class `enum SolidSyslog<Class>Errors` value today, native `errno`/`X509_V_ERR_*` later). Default handler is a silent no-op; setting `handler = NULL` restores the default. Single global slot — intended for setup-time configuration, not synchronised with concurrent `Error` calls. |
| `SolidSyslogErrorCategory.h` | Any handler switching on the portable category axis; any emit site picking a category | Universal lifecycle category macros (`SOLIDSYSLOG_CAT_BAD_CONFIG` / `_BAD_ARGUMENT` / `_POOL_EXHAUSTED` / `_UNKNOWN_DESTROY`, all `uint16_t`) + per-role base ranges (`SOLIDSYSLOG_CAT_<ROLE>_BASE`, errno-domain style; a role occupies `[BASE, BASE + 0xFF]`, `0xC000+` reserved for integrator-defined roles). Category constants are `uint16_t` macros carrying their own cast so emit sites stay clean; the wire type is `uint16_t` (not an enum) so integrator classes can supply their own categories without being boxed into a library enum. Role-specific categories live in `SolidSyslog<Role>Categories.h` beside each `*Definition.h` (`SolidSyslogSenderCategories.h` → `_SENDER_DELIVERY_FAILED` / `_SENDER_DELIVERY_RESTORED`, shared by `StreamSender` + `UdpSender`; `SolidSyslogResolverCategories.h` → `_RESOLVER_RESOLVE_FAILED`; `SolidSyslogTlsStreamCategories.h` → `_TLSSTREAM_INIT_FAILED` / `_HANDSHAKE_FAILED`; `SolidSyslogSecurityPolicyCategories.h` → `_SECURITYPOLICY_KEY_UNAVAILABLE` / `_SEAL_FAILED` / `_OPEN_FAILED`; `SolidSyslogBufferCategories.h` → `_BUFFER_BACKEND_FAILED`). A category is defined only once a live emit site raises it. |
| `SolidSyslogConfigLock.h` | Setup code on systems where library-internal config-time critical sections (E11 pool `_Create`/`_Destroy` slot walks) may race across tasks or cores. Single-task setup needs nothing — defaults are no-ops. | `SolidSyslogConfigLockFunction` typedef (zero-arg `void(void)`), `SolidSyslog_SetConfigLock(lockFn, unlockFn)`, `SolidSyslog_LockConfig()`, `SolidSyslog_UnlockConfig()`. Pair API: both handlers installed together. `NULL` on either side restores that side's no-op default. Single global slot — intended for setup-time configuration, not synchronised with concurrent installs. Integrators wire `taskENTER_CRITICAL`/`taskEXIT_CRITICAL` (FreeRTOS), `pthread_mutex_lock`/`unlock` on a static `pthread_mutex_t` (POSIX), `EnterCriticalSection`/`LeaveCriticalSection` on a static `CRITICAL_SECTION` (Windows), or a spinlock pair. This is the only synchronisation primitive available to the Mutex and AtomicCounter pools for their own pool walks — chicken-and-egg eliminates their own injectables. |
| `SolidSyslogHeaderField.h` | Any code writing an RFC 5424 header field (HOSTNAME / APP-NAME / PROCID) into the field sink it is handed | Opaque `struct SolidSyslogHeaderField` value sink + `_PrintUsAscii`, `_Uint32` — appends PRINTUSASCII content (any other byte, space included, substituted) bounded to the field width. The library owns the charset; a callback cannot reach the raw formatter or break the header framing. Stack-transient, no pool (D.002). |
| `SolidSyslogHeaderFieldFunction.h` | Any code needing the header-field callback typedef | `SolidSyslogHeaderFieldFunction` (callback writes into a `SolidSyslogHeaderField*` + `void* context`) — used by `SolidSyslogConfig` for hostname/appName/processId. Replaces the retired `SolidSyslogStringFunction`. |
| `SolidSyslogPrival.h` | Any code that needs facility/severity enums | `SolidSyslogFacility`, `SolidSyslogSeverity` |
| `SolidSyslogTimestamp.h` | Any code that needs the timestamp struct | `SolidSyslogTimestamp`, `SolidSyslogClockFunction` |
| `SolidSyslogEndpoint.h` | System setup code that supplies destination host/port (and version on changes) | `SolidSyslogEndpoint` (its `Host` member is an opaque `SolidSyslogEndpointHost*` sink, not a formatter), `SolidSyslogEndpointFunction`, `SolidSyslogEndpointVersionFunction`, `SOLIDSYSLOG_MAX_HOST_SIZE` |
| `SolidSyslogEndpointHost.h` | Any code writing the destination host inside a `SolidSyslogEndpointFunction` | Opaque `struct SolidSyslogEndpointHost` value sink + `_String` — appends the host verbatim (no PRINTUSASCII substitution; a DNS name / IP literal must reach the resolver intact) bounded to the host-field width. The callback cannot reach the raw formatter. Stack-transient, no pool (D.002). |
| `SolidSyslogSender.h` | Any code holding a sender handle | `SolidSyslogSender_Send`, `SolidSyslogSender_Disconnect` |
| `SolidSyslogSenderDefinition.h` | Sender implementors (extension point) | `SolidSyslogSender` vtable struct (`Send`, `Disconnect`) |
| `SolidSyslogNullSender.h` | Any code installing a no-op sender slot (Send returns `true` to drop on the floor — keeps Store from filling with undeliverables; Disconnect is a no-op) | `SolidSyslogNullSender_Get` |
| `SolidSyslogTransport.h` | Any code selecting a transport or needing default port constants | `SolidSyslogTransport` enum (`UDP`, `TCP`), `SOLIDSYSLOG_UDP_DEFAULT_PORT`, `SOLIDSYSLOG_TCP_DEFAULT_PORT` |
| `SolidSyslogAddress.h` | Any code holding an address handle | Opaque `struct SolidSyslogAddress` forward declaration only — the destination slot a Resolver writes into and a Datagram / Stream reads from. Handles come from the per-platform `SolidSyslog{Posix,Winsock,PlusTcp}Address_Create()` factories below. |
| `SolidSyslogPosixAddress.h` | System setup code on POSIX targets wiring a UDP / TCP destination | `SolidSyslogPosixAddress_Create(void)`, `_Destroy(base)` (wraps `struct sockaddr_in`). Instance struct lives in a library-internal static pool sized by `SOLIDSYSLOG_ADDRESS_POOL_SIZE` (default 3 — covers the canonical multi-transport BDD wiring). Pool-exhaustion fallback is a TU-private singleton sized as a real `SolidSyslogPosixAddress`; multi-overflow integrators share that storage and race. |
| `SolidSyslogPosixAddressErrors.h` | Any code installing an error handler that wants to react to PosixAddress-specific events (pointer-identity match on `PosixAddressErrorSource`, switch on `enum SolidSyslogPosixAddressErrors`) | `enum SolidSyslogPosixAddressErrors` (`POSIXADDRESS_ERROR_*` codes + `POSIXADDRESS_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PosixAddressErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogWinsockAddress.h` | System setup code on Windows targets wiring a UDP / TCP destination | `SolidSyslogWinsockAddress_Create(void)`, `_Destroy(base)` (wraps Winsock `struct sockaddr_in`). Same pool semantics as `SolidSyslogPosixAddress`. |
| `SolidSyslogWinsockAddressErrors.h` | Any code installing an error handler that wants to react to WinsockAddress-specific events (pointer-identity match on `WinsockAddressErrorSource`, switch on `enum SolidSyslogWinsockAddressErrors`) | `enum SolidSyslogWinsockAddressErrors` (`WINSOCKADDRESS_ERROR_*` codes + `WINSOCKADDRESS_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource WinsockAddressErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogPlusTcpAddress.h` | System setup code on FreeRTOS targets wiring a UDP / TCP destination | `SolidSyslogPlusTcpAddress_Create(void)`, `_Destroy(base)` (wraps `struct freertos_sockaddr`). Same pool semantics as `SolidSyslogPosixAddress`. |
| `SolidSyslogPlusTcpAddressErrors.h` | Any code installing an error handler that wants to react to PlusTcpAddress-specific events (pointer-identity match on `PlusTcpAddressErrorSource`, switch on `enum SolidSyslogPlusTcpAddressErrors`) | `enum SolidSyslogPlusTcpAddressErrors` (`PLUSTCPADDRESS_ERROR_*` codes + `PLUSTCPADDRESS_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PlusTcpAddressErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogLwipRawAddress.h` | System setup code on **any target using lwIP Raw API** (bare metal, Zephyr, FreeRTOS, …) wiring a UDP / TCP destination | `SolidSyslogLwipRawAddress_Create(void)`, `_Destroy(base)` (wraps lwIP `ip_addr_t` + `u16_t` port as a POD; consumers reach `->Ip` / `->Port` via the `SolidSyslogLwipRawAddress_As` / `_AsConst` downcast helpers in `Private.h`). Same pool semantics as `SolidSyslogPosixAddress` — sized by `SOLIDSYSLOG_ADDRESS_POOL_SIZE`. |
| `SolidSyslogLwipRawAddressErrors.h` | Any code installing an error handler that wants to react to LwipRawAddress-specific events (pointer-identity match on `LwipRawAddressErrorSource`, switch on `enum SolidSyslogLwipRawAddressErrors`) | `enum SolidSyslogLwipRawAddressErrors` (`LWIPRAWADDRESS_ERROR_*` codes + `LWIPRAWADDRESS_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource LwipRawAddressErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogResolver.h` | Any code that needs to resolve a destination | `SolidSyslogResolver_Resolve(resolver, transport, host, port, *out)` |
| `SolidSyslogResolverDefinition.h` | Resolver implementors (extension point) | `SolidSyslogResolver` vtable struct (`Resolve`) |
| `SolidSyslogNullResolver.h` | Any code installing a no-op resolver slot (Resolve returns `false` so the caller's unresolved-host error path runs naturally) | `SolidSyslogNullResolver_Get` |
| `SolidSyslogPlusTcpResolver.h` | System setup code on FreeRTOS targets pinned to a hardcoded IPv4 destination (no DNS) | `SolidSyslogPlusTcpResolver_Create(ipv4Octets)`, `_Destroy(base)` (ignores host and transport at Resolve time; port is taken from each call). Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullResolver`. |
| `SolidSyslogPlusTcpResolverErrors.h` | Any code installing an error handler that wants to react to PlusTcpResolver-specific events (pointer-identity match on `PlusTcpResolverErrorSource`, switch on `enum SolidSyslogPlusTcpResolverErrors`) | `enum SolidSyslogPlusTcpResolverErrors` (`PLUSTCPRESOLVER_ERROR_*` codes + `PLUSTCPRESOLVER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PlusTcpResolverErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogLwipRawResolver.h` | System setup code on **any target using lwIP Raw API** that resolves numeric IPv4 hosts (no DNS) | `SolidSyslogLwipRawResolver_Create(void)`, `_Destroy(base)` — Resolve delegates to lwIP's `ipaddr_aton`: whatever the parser accepts, we accept; whatever it rejects (DNS names, alphabetic input, empty string), we reject. We do not enforce any specific shape on top of the parser. Transport is ignored. DNS-name resolution lands as the sibling `SolidSyslogLwipRawDnsResolver`. Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullResolver`. |
| `SolidSyslogLwipRawResolverErrors.h` | Any code installing an error handler that wants to react to LwipRawResolver-specific events (pointer-identity match on `LwipRawResolverErrorSource`, switch on `enum SolidSyslogLwipRawResolverErrors`) | `enum SolidSyslogLwipRawResolverErrors` (`LWIPRAWRESOLVER_ERROR_*` codes + `LWIPRAWRESOLVER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource LwipRawResolverErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogLwipRawDnsResolver.h` | System setup code on **any target using lwIP Raw API** that resolves hosts **by name** (DNS) — superset of the numeric resolver: literals, DNS-cache hits, and local-hostlist entries also resolve | `SolidSyslogLwipRawDnsResolverConfig` (required `Sleep`), `SolidSyslogLwipRawDnsResolver_Create(config)`, `_Destroy(base)` — wraps lwIP's async `dns_gethostbyname` under the `SolidSyslogLwipRaw_Marshal` hop (it touches lwIP core state, unlike the numeric resolver's `ipaddr_aton`), bridging it to the synchronous `Resolve()` via a bounded spin on the caller's thread (integrator-supplied `Sleep`; deadline `SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS`, poll `SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS`). `ERR_OK` synchronous hit → immediate; `ERR_INPROGRESS` → spin to the `dns_found_callback`; deadline → fail + error report. Transport ignored. Requires `LWIP_DNS=1`. Instance struct lives in a library-internal static pool (E11; role tunable `SOLIDSYSLOG_RESOLVER_POOL_SIZE`, default 1). Pool-exhaustion and bad-config (`NULL` config or `NULL` `Sleep`) fallback is the shared `SolidSyslogNullResolver`. See [`docs/integrating-lwip.md`](docs/integrating-lwip.md). |
| `SolidSyslogLwipRawDnsResolverErrors.h` | Any code installing an error handler that wants to react to LwipRawDnsResolver-specific events (pointer-identity match on `LwipRawDnsResolverErrorSource`, switch on `enum SolidSyslogLwipRawDnsResolverErrors`) | `enum SolidSyslogLwipRawDnsResolverErrors` (`LWIPRAWDNSRESOLVER_ERROR_*` codes incl. `_RESOLVE_TIMEOUT` + `LWIPRAWDNSRESOLVER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource LwipRawDnsResolverErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogWinsockResolverErrors.h` | Any code installing an error handler that wants to react to WinsockResolver-specific events (pointer-identity match on `WinsockResolverErrorSource`, switch on `enum SolidSyslogWinsockResolverErrors`) | `enum SolidSyslogWinsockResolverErrors` (`WINSOCKRESOLVER_ERROR_*` codes + `WINSOCKRESOLVER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource WinsockResolverErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogGetAddrInfoResolverErrors.h` | Any code installing an error handler that wants to react to GetAddrInfoResolver-specific events (pointer-identity match on `GetAddrInfoResolverErrorSource`, switch on `enum SolidSyslogGetAddrInfoResolverErrors`) | `enum SolidSyslogGetAddrInfoResolverErrors` (`GETADDRINFORESOLVER_ERROR_*` codes + `GETADDRINFORESOLVER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource GetAddrInfoResolverErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogDatagram.h` | Sender implementors using datagram transport | `SolidSyslogDatagram_Open`, `_SendTo`, `_Close` |
| `SolidSyslogDatagramDefinition.h` | Datagram implementors (extension point) | `SolidSyslogDatagram` vtable struct (`Open`, `SendTo`, `Close`) |
| `SolidSyslogNullDatagram.h` | Any code installing a no-op datagram slot (Open/Close are no-ops, SendTo returns `SENT` to drop on the floor so the Store does not fill with undeliverables, MaxPayload returns the IPv6-safe default) | `SolidSyslogNullDatagram_Get` |
| `SolidSyslogPlusTcpDatagram.h` | System setup code on FreeRTOS targets using FreeRTOS-Plus-TCP for UDP | `SolidSyslogPlusTcpDatagram_Create(void)`, `_Destroy(base)` (wraps `FreeRTOS_socket` / `FreeRTOS_sendto` / `FreeRTOS_closesocket`). Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullDatagram`. |
| `SolidSyslogPlusTcpDatagramErrors.h` | Any code installing an error handler that wants to react to PlusTcpDatagram-specific events (pointer-identity match on `PlusTcpDatagramErrorSource`, switch on `enum SolidSyslogPlusTcpDatagramErrors`) | `enum SolidSyslogPlusTcpDatagramErrors` (`PLUSTCPDATAGRAM_ERROR_*` codes + `PLUSTCPDATAGRAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PlusTcpDatagramErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogLwipRawDatagram.h` | System setup code on **any target using lwIP Raw API** wiring a UDP sender | `SolidSyslogLwipRawDatagram_Create(void)`, `_Destroy(base)` (wraps `udp_new` / `udp_sendto` / `udp_remove`; PBUF_REF zero-copy send; relies on lwIP `ARP_QUEUEING` for cache-miss recovery). Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullDatagram`. |
| `SolidSyslogLwipRawDatagramErrors.h` | Any code installing an error handler that wants to react to LwipRawDatagram-specific events (pointer-identity match on `LwipRawDatagramErrorSource`, switch on `enum SolidSyslogLwipRawDatagramErrors`) | `enum SolidSyslogLwipRawDatagramErrors` (`LWIPRAWDATAGRAM_ERROR_*` codes + `LWIPRAWDATAGRAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource LwipRawDatagramErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogWinsockDatagramErrors.h` | Any code installing an error handler that wants to react to WinsockDatagram-specific events (pointer-identity match on `WinsockDatagramErrorSource`, switch on `enum SolidSyslogWinsockDatagramErrors`) | `enum SolidSyslogWinsockDatagramErrors` (`WINSOCKDATAGRAM_ERROR_*` codes + `WINSOCKDATAGRAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource WinsockDatagramErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogPosixDatagramErrors.h` | Any code installing an error handler that wants to react to PosixDatagram-specific events (pointer-identity match on `PosixDatagramErrorSource`, switch on `enum SolidSyslogPosixDatagramErrors`) | `enum SolidSyslogPosixDatagramErrors` (`POSIXDATAGRAM_ERROR_*` codes + `POSIXDATAGRAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PosixDatagramErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogStream.h` | Sender implementors using stream transport | `SolidSyslogStream_Open`, `_Send`, `_Read`, `_Close`, `SolidSyslogSsize` |
| `SolidSyslogStreamDefinition.h` | Stream implementors (extension point) | `SolidSyslogStream` vtable struct (`Open`, `Send`, `Read`, `Close`) |
| `SolidSyslogNullStream.h` | Any code installing a no-op stream slot (Open/Close are no-ops, Send returns `true` to drop on the floor so the Store does not fill with undeliverables, Read returns `0` for would-block so the caller does not tear the connection down) | `SolidSyslogNullStream_Get` |
| `SolidSyslogPlusTcpTcpStream.h` | System setup code on FreeRTOS targets using FreeRTOS-Plus-TCP for TCP | `SolidSyslogPlusTcpTcpStream_Create(void)`, `_Destroy(base)` (wraps `FreeRTOS_socket` / `FreeRTOS_connect` / `FreeRTOS_send` / `FreeRTOS_recv` / `FreeRTOS_closesocket`; bounded blocking connect via `SO_SNDTIMEO=200ms`, cleared post-connect so Send/Read are non-blocking). Instance struct lives in a library-internal static pool (E11); default pool size 2 to support the future TLS-via-mbedTLS + plain-TCP pair (S08.07). Pool-exhaustion fallback is the shared `SolidSyslogNullStream`. |
| `SolidSyslogPlusTcpTcpStreamErrors.h` | Any code installing an error handler that wants to react to PlusTcpTcpStream-specific events (pointer-identity match on `PlusTcpTcpStreamErrorSource`, switch on `enum SolidSyslogPlusTcpTcpStreamErrors`) | `enum SolidSyslogPlusTcpTcpStreamErrors` (`PLUSTCPTCPSTREAM_ERROR_*` codes + `PLUSTCPTCPSTREAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PlusTcpTcpStreamErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogLwipRawTcpStream.h` | System setup code on **any target using lwIP Raw API** wiring a TCP sender (or a byte transport for `SolidSyslogTlsStream` / `SolidSyslogMbedTlsStream`) | `SolidSyslogLwipRawTcpStreamConfig` (`GetConnectTimeoutMs` + context + required `Sleep`), `SolidSyslogLwipRawTcpStream_Create(config)`, `_Destroy(base)` (wraps `tcp_new` / `tcp_connect` / `tcp_write` (`TCP_WRITE_FLAG_COPY`) / `tcp_output` / `tcp_recv` / `tcp_recved` / `tcp_close` / `tcp_abort`; integrator-supplied Sleep drives the bounded synchronous-Open spin loop; sets `SOF_KEEPALIVE` on every pcb; bounded RX pbuf queue via `SOLIDSYSLOG_LWIP_RAW_TCP_RX_QUEUE_SIZE`; encapsulates lwIP's `tcp_close`-after-`tcp_err` use-after-free rule). Instance struct lives in a library-internal static pool (E11); default pool size 2 to support the TLS-over-plain-TCP pair. Pool-exhaustion and bad-config (`NULL` config or `NULL` Sleep) fallback is the shared `SolidSyslogNullStream`. See [`docs/integrating-lwip.md`](docs/integrating-lwip.md) for the full integrator guide. |
| `SolidSyslogLwipRawTcpStreamErrors.h` | Any code installing an error handler that wants to react to LwipRawTcpStream-specific events (pointer-identity match on `LwipRawTcpStreamErrorSource`, switch on `enum SolidSyslogLwipRawTcpStreamErrors`) | `enum SolidSyslogLwipRawTcpStreamErrors` (`LWIPRAWTCPSTREAM_ERROR_*` codes + `LWIPRAWTCPSTREAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource LwipRawTcpStreamErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogLwipRawMarshal.h` | System setup code on **any target using lwIP Raw API** that needs to pin lwIP calls to the core-owning thread (`NO_SYS=0` integrators) | `SolidSyslogLwipRawCallback`, `SolidSyslogLwipRawMarshalFunction`, `SolidSyslogLwipRaw_SetMarshal(marshal)`. Routes the lwIP-touching Datagram/TcpStream adapter operations through one marshal hop per public operation (the Resolver's numeric `ipaddr_aton` parse touches no lwIP core state and is intentionally not marshalled). Default is a direct-call null marshal (correct for `NO_SYS=1`); `NO_SYS=0` integrators install a `tcpip_callback_with_block` shim or a `LOCK_TCPIP_CORE`/`UNLOCK_TCPIP_CORE` pair. The marshal MUST call its callback synchronously. Setting `marshal = NULL` restores the default. Single global slot — intended for setup-time configuration, not synchronised with concurrent installs. See [`docs/integrating-lwip.md`](docs/integrating-lwip.md). |
| `SolidSyslogWinsockTcpStreamErrors.h` | Any code installing an error handler that wants to react to WinsockTcpStream-specific events (pointer-identity match on `WinsockTcpStreamErrorSource`, switch on `enum SolidSyslogWinsockTcpStreamErrors`) | `enum SolidSyslogWinsockTcpStreamErrors` (`WINSOCKTCPSTREAM_ERROR_*` codes + `WINSOCKTCPSTREAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource WinsockTcpStreamErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogUdpSender.h` | System setup code using UDP transport | `SolidSyslogUdpSenderConfig` (resolver, datagram, address, endpoint, endpointVersion — the integrator supplies one platform Address handle per sender from `SolidSyslog{Posix,Winsock,PlusTcp}Address_Create`), `SolidSyslogUdpSender_Create(config)`, `_Destroy(sender)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create`. Pool-exhaustion and bad-config (including NULL Address) fallback is the shared `SolidSyslogNullSender`. |
| `SolidSyslogUdpSenderErrors.h` | Any code installing an error handler that wants to react to UdpSender-specific events (pointer-identity match on `UdpSenderErrorSource`, switch on `enum SolidSyslogUdpSenderErrors`) | `enum SolidSyslogUdpSenderErrors` (`UDPSENDER_ERROR_*` codes + `UDPSENDER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource UdpSenderErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogPosixTcpStream.h` | System setup code on POSIX targets using non-blocking TCP with bounded connect/keepalive | `SolidSyslogPosixTcpStream_Create(void)`, `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11); default pool size 2 to support the BDD-target plain-TCP + TLS-underlying-TCP pair. Pool-exhaustion fallback is the shared `SolidSyslogNullStream`. |
| `SolidSyslogPosixTcpStreamErrors.h` | Any code installing an error handler that wants to react to PosixTcpStream-specific events (pointer-identity match on `PosixTcpStreamErrorSource`, switch on `enum SolidSyslogPosixTcpStreamErrors`) | `enum SolidSyslogPosixTcpStreamErrors` (`POSIXTCPSTREAM_ERROR_*` codes + `POSIXTCPSTREAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PosixTcpStreamErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogStreamSender.h` | System setup code using octet-framed transport (RFC 6587) over any Stream — `SolidSyslogPosixTcpStream` (POSIX TCP), `SolidSyslogWinsockTcpStream` (Windows TCP), `SolidSyslogPlusTcpTcpStream` (FreeRTOS-Plus-TCP), `SolidSyslogTlsStream` (TLS; OpenSSL reference integration), `SolidSyslogMbedTlsStream` (TLS; Mbed TLS reference integration for embedded targets), or a caller-supplied Stream backend | `SolidSyslogStreamSenderConfig` (resolver, stream, address, endpoint, endpointVersion — the integrator supplies one platform Address handle per sender from `SolidSyslog{Posix,Winsock,PlusTcp}Address_Create`), `SolidSyslogStreamSender_Create(config)`, `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create`. Pool-exhaustion and bad-config fallback is the shared `SolidSyslogNullSender`. |
| `SolidSyslogStreamSenderErrors.h` | Any code installing an error handler that wants to react to StreamSender-specific events (pointer-identity match on `StreamSenderErrorSource`, switch on `enum SolidSyslogStreamSenderErrors`) | `enum SolidSyslogStreamSenderErrors` (`STREAMSENDER_ERROR_*` codes + `STREAMSENDER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource StreamSenderErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogTlsStream.h` | System setup code using TLS over an injected `SolidSyslogStream` transport (OpenSSL reference integration) | `SolidSyslogTlsStreamConfig` (transport, sleep, caBundlePath, serverName, cipherList, clientCertChainPath, clientKeyPath), `SolidSyslogTlsStream_Create(config)`, `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullStream`. |
| `SolidSyslogTlsStreamErrors.h` | Any code installing an error handler that wants to react to TlsStream-specific events (pointer-identity match on `TlsStreamErrorSource`, switch on `enum SolidSyslogTlsStreamErrors`) | `enum SolidSyslogTlsStreamErrors` (`TLSSTREAM_ERROR_*` codes + `TLSSTREAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource TlsStreamErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogMbedTlsStream.h` | System setup code using TLS over an injected `SolidSyslogStream` transport via Mbed TLS (embedded-target reference integration; pair with `SolidSyslogPlusTcpTcpStream`, `SolidSyslogPosixTcpStream`, `SolidSyslogWinsockTcpStream`, or a caller-supplied byte transport) | `SolidSyslogMbedTlsStreamConfig` (transport, sleep, rng, caChain, serverName, clientCertChain, clientKey — caller-built `mbedtls_ctr_drbg_context*` / `mbedtls_x509_crt*` / `mbedtls_pk_context*` handles, never file paths or PEM blobs), `SolidSyslogMbedTlsStream_Create(config)`, `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11); coexistence contract — `Platform/MbedTls/Source/` never calls process-global Mbed TLS APIs (`mbedtls_platform_setup` / `_teardown`, `psa_crypto_init`, threading-alt, debug hooks). Pool-exhaustion fallback is the shared `SolidSyslogNullStream`. See [`docs/integrating-mbedtls.md`](docs/integrating-mbedtls.md) for the full integrator guide. |
| `SolidSyslogMbedTlsStreamErrors.h` | Any code installing an error handler that wants to react to MbedTlsStream-specific events (pointer-identity match on `MbedTlsStreamErrorSource`, switch on `enum SolidSyslogMbedTlsStreamErrors`) | `enum SolidSyslogMbedTlsStreamErrors` (`MBEDTLSSTREAM_ERROR_*` codes + `MBEDTLSSTREAM_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource MbedTlsStreamErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogSwitchingSender.h` | System setup code composing multiple inner senders | `SolidSyslogSwitchingSenderConfig` (senders, senderCount, selector), `SolidSyslogSwitchingSenderSelector`, `SolidSyslogSwitchingSender_Create(config)`, `_Destroy(sender)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create`. Out-of-range selector and pool exhaustion both resolve to the shared `SolidSyslogNullSender`. |
| `SolidSyslogSwitchingSenderErrors.h` | Any code installing an error handler that wants to react to SwitchingSender-specific events (pointer-identity match on `SwitchingSenderErrorSource`, switch on `enum SolidSyslogSwitchingSenderErrors`) | `enum SolidSyslogSwitchingSenderErrors` (`SWITCHINGSENDER_ERROR_*` codes + `SWITCHINGSENDER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource SwitchingSenderErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogBuffer.h` | Library internals consuming a buffer (Service algorithm) | `SolidSyslogBuffer_Write`, `_Read` |
| `SolidSyslogNullBuffer.h` | Any code installing a no-op buffer slot (Read returns `false`/`bytesRead=0`; Write swallows the record) | `SolidSyslogNullBuffer_Get` |
| `SolidSyslogBufferDefinition.h` | Buffer implementors (extension point) | `SolidSyslogBuffer` vtable struct |
| `SolidSyslogPassthroughBuffer.h` | System setup code (single-task, no buffering) | `SolidSyslogPassthroughBuffer_Create(sender)`, `_Destroy(buffer)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create` to release the slot. Pool exhaustion resolves to the shared `SolidSyslogNullBuffer`. |
| `SolidSyslogPassthroughBufferErrors.h` | Any code installing an error handler that wants to react to PassthroughBuffer-specific events (pointer-identity match on `PassthroughBufferErrorSource`, switch on `enum SolidSyslogPassthroughBufferErrors`) | `enum SolidSyslogPassthroughBufferErrors` (`PASSTHROUGHBUFFER_ERROR_*` codes + `PASSTHROUGHBUFFER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PassthroughBufferErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogPosixMessageQueueBuffer.h` | System setup code using POSIX message queue buffer | `SolidSyslogPosixMessageQueueBuffer_Create(maxMessageSize, maxMessages)`, `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11); each slot's queue name is `/solidsyslog_<pid>_<slotIndex>` so multiple in-process slots remain isolated. Pool-exhaustion fallback is the shared `SolidSyslogNullBuffer`. |
| `SolidSyslogPosixMessageQueueBufferErrors.h` | Any code installing an error handler that wants to react to PosixMessageQueueBuffer-specific events (pointer-identity match on `PosixMessageQueueBufferErrorSource`, switch on `enum SolidSyslogPosixMessageQueueBufferErrors`) | `enum SolidSyslogPosixMessageQueueBufferErrors` (`POSIXMESSAGEQUEUEBUFFER_ERROR_*` codes + `POSIXMESSAGEQUEUEBUFFER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PosixMessageQueueBufferErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogCircularBuffer.h` | System setup code using an in-memory ring buffer (bare-metal / RTOS / Windows) | `SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES`, `SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(maxMessages)` (friendly: N max-sized messages → ring bytes), `SolidSyslogCircularBuffer_Create(mutex, ring, ringBytes)`, `_Destroy(buffer)` (uint16-length-prefixed records, drop-newest on overflow, no-split wrap, mutex-injected synchronisation). Instance struct lives in a library-internal static pool (E11); caller supplies the ring memory only. Pool exhaustion resolves to the shared `SolidSyslogNullBuffer`. |
| `SolidSyslogCircularBufferErrors.h` | Any code installing an error handler that wants to react to CircularBuffer-specific events (pointer-identity match on `CircularBufferErrorSource`, switch on `enum SolidSyslogCircularBufferErrors`) | `enum SolidSyslogCircularBufferErrors` (`CIRCULARBUFFER_ERROR_*` codes + `CIRCULARBUFFER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource CircularBufferErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogMutex.h` | Any code holding a mutex handle | `SolidSyslogMutex_Lock`, `_Unlock` |
| `SolidSyslogMutexDefinition.h` | Mutex implementors (extension point) | `SolidSyslogMutex` vtable struct (`Lock`, `Unlock`) |
| `SolidSyslogNullMutex.h` | System setup code (single-task, no synchronisation) | `SolidSyslogNullMutex_Get` |
| `SolidSyslogPosixMutex.h` | System setup code on POSIX targets needing thread-safe buffering | `SolidSyslogPosixMutex_Create(void)`, `_Destroy(base)` (wraps `pthread_mutex_t`). Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullMutex`. |
| `SolidSyslogPosixMutexErrors.h` | Any code installing an error handler that wants to react to PosixMutex-specific events (pointer-identity match on `PosixMutexErrorSource`, switch on `enum SolidSyslogPosixMutexErrors`) | `enum SolidSyslogPosixMutexErrors` (`POSIXMUTEX_ERROR_*` codes + `POSIXMUTEX_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PosixMutexErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogWindowsMutex.h` | System setup code on Windows targets needing thread-safe buffering | `SolidSyslogWindowsMutex_Create(void)`, `_Destroy(base)` (wraps `CRITICAL_SECTION`). Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullMutex`. |
| `SolidSyslogWindowsMutexErrors.h` | Any code installing an error handler that wants to react to WindowsMutex-specific events (pointer-identity match on `WindowsMutexErrorSource`, switch on `enum SolidSyslogWindowsMutexErrors`) | `enum SolidSyslogWindowsMutexErrors` (`WINDOWSMUTEX_ERROR_*` codes + `WINDOWSMUTEX_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource WindowsMutexErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogFreeRtosMutex.h` | System setup code on FreeRTOS targets needing thread-safe buffering | `SolidSyslogFreeRtosMutex_Create(void)`, `_Destroy(base)` (wraps `xSemaphoreCreateMutexStatic`; requires `configSUPPORT_STATIC_ALLOCATION=1`). Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullMutex`. |
| `SolidSyslogFreeRtosMutexErrors.h` | Any code installing an error handler that wants to react to FreeRtosMutex-specific events (pointer-identity match on `FreeRtosMutexErrorSource`, switch on `enum SolidSyslogFreeRtosMutexErrors`) | `enum SolidSyslogFreeRtosMutexErrors` (`FREERTOSMUTEX_ERROR_*` codes + `FREERTOSMUTEX_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource FreeRtosMutexErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogStore.h` | Library internals consuming a store (Service algorithm) and integrator code querying capacity | `SolidSyslogStore_Write`, `_ReadNextUnsent`, `_MarkSent`, `_HasUnsent`, `_IsHalted`, `_GetTotalBytes`, `_GetUsedBytes` |
| `SolidSyslogStoreDefinition.h` | Store implementors (extension point) | `SolidSyslogStore` vtable struct (`Write`, `ReadNextUnsent`, `MarkSent`, `HasUnsent`, `IsHalted`, `GetTotalBytes`, `GetUsedBytes`) |
| `SolidSyslogNullStore.h` | System setup code (no store-and-forward) | `SolidSyslogNullStore_Get` |
| `SolidSyslogFileDefinition.h` | File implementors (extension point) | `SolidSyslogFile` vtable struct |
| `SolidSyslogFile.h` | Any code using the file abstraction | `SolidSyslogFile_Open`, `_Close`, `_IsOpen`, `_Read`, `_Write`, `_SeekTo`, `_Size`, `_Truncate` |
| `SolidSyslogNullFile.h` | Any code installing a no-op file slot (Open/IsOpen/Read/Exists return `false`, Write/Delete return `true` so callers' success paths are not tripped, SeekTo/Truncate/Close are no-ops, Size returns `0`) | `SolidSyslogNullFile_Get` |
| `SolidSyslogBlockDevice.h` | Library internals consuming a block device (BlockSequence inside BlockStore) and integrator code addressing blocks directly | `SolidSyslogBlockDevice_Acquire`, `_Dispose`, `_Exists`, `_Read`, `_Append`, `_WriteAt`, `_Size`, `_GetBlockSize` (block-indexed I/O; Acquire makes a block ready for fresh writes, Dispose releases it; `_Size(blockIndex)` is a block's current occupancy, `_GetBlockSize()` is the device-wide per-block capacity — the device is the single source of truth a BlockStore reads at construction) |
| `SolidSyslogNullBlockDevice.h` | Any code installing a no-op block device slot (every method returns `false` / `0` — disk doesn't exist) | `SolidSyslogNullBlockDevice_Get` |
| `SolidSyslogBlockDeviceDefinition.h` | BlockDevice implementors (extension point) | `SolidSyslogBlockDevice` vtable struct (`Acquire`, `Dispose`, `Exists`, `Read`, `Append`, `WriteAt`, `Size`, `GetBlockSize`) |
| `SolidSyslogFileBlockDevice.h` | System setup code wiring a file-backed block device | `SolidSyslogFileBlockDevice_Create(file, pathPrefix, blockSize)` (sequence-numbered filenames `<prefix><NN>.log`, one cached file handle; `blockSize` is the per-block capacity reported via `GetBlockSize`, `0` selects the `SOLIDSYSLOG_FILE_DEFAULT_BLOCK_SIZE` tunable), `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create`. Pool exhaustion resolves to the shared `SolidSyslogNullBlockDevice`. |
| `SolidSyslogFileBlockDeviceErrors.h` | Any code installing an error handler that wants to react to FileBlockDevice-specific events (pointer-identity match on `FileBlockDeviceErrorSource`, switch on `enum SolidSyslogFileBlockDeviceErrors`) | `enum SolidSyslogFileBlockDeviceErrors` (`FILEBLOCKDEVICE_ERROR_*` codes + `FILEBLOCKDEVICE_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource FileBlockDeviceErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogBlockStore.h` | System setup code using a BlockDevice-backed store | `SolidSyslogBlockStoreConfig` (with `blockDevice`, `storeFullContext`, `getCapacityThreshold`, `onThresholdCrossed`, `thresholdContext`), `SolidSyslogBlockStore_Create(config)`, `_Destroy(store)`, `SolidSyslogDiscardPolicy` (`SolidSyslogDiscardPolicy_Oldest` / `_Newest` / `_Halt`), `SolidSyslogStoreFullCallback(void* context)`, `SolidSyslogStoreThresholdFunction(void* context)` (returns threshold in bytes; 0 disables; queried each Write), `SolidSyslogStoreThresholdCallback(void* context)` (edge-triggered; PassthroughBuffer recursion gotcha — see header). Instance struct lives in a library-internal static pool (E11); each slot composes one inner RecordStore + BlockSequence drawn 1:1 from sibling pools sized off the same `SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE` tunable. Pool exhaustion (or either inner Create failing) resolves to the shared `SolidSyslogNullStore`. |
| `SolidSyslogBlockStoreErrors.h` | Any code installing an error handler that wants to react to BlockStore-specific events (pointer-identity match on `BlockStoreErrorSource`, switch on `enum SolidSyslogBlockStoreErrors`) | `enum SolidSyslogBlockStoreErrors` (`BLOCKSTORE_ERROR_*` codes + `BLOCKSTORE_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource BlockStoreErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogSecurityPolicyDefinition.h` | SecurityPolicy implementors (extension point) | `SolidSyslogSecurityPolicy` vtable struct |
| `SolidSyslogNullSecurityPolicy.h` | System setup code (no integrity checking) | `SolidSyslogNullSecurityPolicy_Get` |
| `SolidSyslogCrc16Policy.h` | System setup code using CRC-16 integrity | `SolidSyslogCrc16Policy_Create`, `_Destroy` |
| `SolidSyslogCrc16.h` | Any code needing CRC-16 computation | `SolidSyslogCrc16_Compute` |
| `SolidSyslogPosixFile.h` | System setup code using POSIX file I/O | `SolidSyslogPosixFile_Create(void)`, `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullFile`. |
| `SolidSyslogPosixFileErrors.h` | Any code installing an error handler that wants to react to PosixFile-specific events (pointer-identity match on `PosixFileErrorSource`, switch on `enum SolidSyslogPosixFileErrors`) | `enum SolidSyslogPosixFileErrors` (`POSIXFILE_ERROR_*` codes + `POSIXFILE_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PosixFileErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogWindowsFile.h` | System setup code using Windows file I/O (MSVC `<io.h>`) | `SolidSyslogWindowsFile_Create(void)`, `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullFile`. |
| `SolidSyslogWindowsFileErrors.h` | Any code installing an error handler that wants to react to WindowsFile-specific events (pointer-identity match on `WindowsFileErrorSource`, switch on `enum SolidSyslogWindowsFileErrors`) | `enum SolidSyslogWindowsFileErrors` (`WINDOWSFILE_ERROR_*` codes + `WINDOWSFILE_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource WindowsFileErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogFatFsFile.h` | System setup code on targets using ChaN FatFs as the file layer (bare-metal flash / SD / eMMC, FreeRTOS, Zephyr, NuttX, …) | `SolidSyslogFatFsFile_Create(void)`, `_Destroy(file)` (wraps `f_open` / `f_close` / `f_read` / `f_write` with `f_sync` after every successful write so a power loss never loses a record the BlockStore claimed it stored; integrator supplies `diskio.c` and — if `FF_FS_REENTRANT=1` — `ffsystem.c`). Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullFile`. |
| `SolidSyslogFatFsFileErrors.h` | Any code installing an error handler that wants to react to FatFsFile-specific events (pointer-identity match on `FatFsFileErrorSource`, switch on `enum SolidSyslogFatFsFileErrors`) | `enum SolidSyslogFatFsFileErrors` (`FATFSFILE_ERROR_*` codes + `FATFSFILE_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource FatFsFileErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogPlusFatFile.h` | System setup code on FreeRTOS targets using FreeRTOS-Plus-FAT as the file layer | `SolidSyslogPlusFatFile_Create(void)`, `_Destroy(file)` (wraps `ff_fopen` (`"r+"` then `"w+"` fallback — open-or-create without truncation) / `ff_fclose` / `ff_fread` / `ff_fwrite` with `ff_fflush` after every complete write so a power loss never loses a record the BlockStore claimed it stored; `ff_fseek` / `ff_seteof` for truncate-to-empty; `ff_filelength`, `ff_stat`, `ff_remove`). The open-state is the `FF_FILE*` sentinel (no separate flag). Plus-FAT is FreeRTOS-coupled (`ff_headers.h` pulls `FreeRTOS.h` / `task.h` / `semphr.h`); integrator supplies the `FF_Disk_t` media driver and `FreeRTOSFATConfig.h`. Instance struct lives in a library-internal static pool (E11; role tunable `SOLIDSYSLOG_FILE_POOL_SIZE`). Pool-exhaustion fallback is the shared `SolidSyslogNullFile`. |
| `SolidSyslogPlusFatFileErrors.h` | Any code installing an error handler that wants to react to PlusFatFile-specific events (pointer-identity match on `PlusFatFileErrorSource`, switch on `enum SolidSyslogPlusFatFileErrors`) | `enum SolidSyslogPlusFatFileErrors` (`PLUSFATFILE_ERROR_*` codes + `PLUSFATFILE_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource PlusFatFileErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogPosixClock.h` | System setup code using POSIX clock | `SolidSyslogPosixClock_GetTimestamp` |
| `SolidSyslogPosixHostname.h` | String callback implementor using POSIX hostname | `SolidSyslogPosixHostname_Get` (writes into `SolidSyslogFormatter*`) |
| `SolidSyslogPosixProcessId.h` | String callback implementor using POSIX process ID | `SolidSyslogPosixProcessId_Get` (writes into `SolidSyslogFormatter*`) |
| `SolidSyslogPosixSysUpTime.h` | SysUpTime callback implementor using POSIX `CLOCK_BOOTTIME` | `SolidSyslogPosixSysUpTime_Get` (returns `uint32_t` hundredths since boot, wraps per RFC 3418 `TimeTicks`) |
| `SolidSyslogPosixSleep.h` | System setup code wiring a `SolidSyslogSleepFunction` on POSIX targets | `SolidSyslogPosixSleep` (wraps `nanosleep`) |
| `SolidSyslogWindowsClock.h` | System setup code using Windows clock | `SolidSyslogWindowsClock_GetTimestamp` |
| `SolidSyslogWindowsHostname.h` | String callback implementor using Windows hostname | `SolidSyslogWindowsHostname_Get` (writes into `SolidSyslogFormatter*`) |
| `SolidSyslogWindowsProcessId.h` | String callback implementor using Windows process ID | `SolidSyslogWindowsProcessId_Get` (writes into `SolidSyslogFormatter*`) |
| `SolidSyslogWindowsSysUpTime.h` | SysUpTime callback implementor using `GetTickCount64` | `SolidSyslogWindowsSysUpTime_Get` (returns `uint32_t` hundredths since boot), `WindowsSysUpTime_GetTickCount64` (function-pointer seam for unit tests) |
| `SolidSyslogFreeRtosSysUpTime.h` | SysUpTime callback implementor using `xTaskGetTickCount` on FreeRTOS targets | `SolidSyslogFreeRtosSysUpTime_Get` (returns `uint32_t` hundredths since boot, wraps per RFC 3418 `TimeTicks`; uint64 intermediate so the result is correct at any `configTICK_RATE_HZ`) |
| `SolidSyslogWindowsSleep.h` | System setup code wiring a `SolidSyslogSleepFunction` on Windows targets | `SolidSyslogWindowsSleep` (wraps `Sleep`) |
| `SolidSyslogSleep.h` | Any code passing or implementing a sleep callback | `SolidSyslogSleepFunction` typedef (used by `SolidSyslogTlsStreamConfig.sleep` for the bounded handshake retry) |
| `SolidSyslogStructuredData.h` | Library internals (SD dispatch) | `SolidSyslogStructuredData_Format` (writes into a `SolidSyslogSdElement*`) |
| `SolidSyslogStructuredDataDefinition.h` | SD implementors (extension point) | `SolidSyslogStructuredData` vtable struct (Format takes a `SolidSyslogSdElement*`, not a formatter) |
| `SolidSyslogSdElement.h` | SD implementors framing one `[SD-ID PARAM="value"…]` element | Opaque `struct SolidSyslogSdElement` + `_Begin(name, enterpriseNumber)`, `_Param(name) → SolidSyslogSdValue*`, `_End`. Owns the brackets, `@`-enterprise SD-ID suffix, and SD-NAME validation (NULL name → element/param skipped); a producer cannot break the framing. |
| `SolidSyslogSdValue.h` | SD implementors writing a PARAM value | Opaque `struct SolidSyslogSdValue` + `_String`, `_BoundedString`, `_Uint32` — appends a PARAM-VALUE with RFC 5424 `"`/`\`/`]` escaping applied by the library, streaming across calls. |
| `SolidSyslogSdValueFunction.h` | Any code needing the SD-value callback typedef | `SolidSyslogSdValueFunction` (callback writes into a `SolidSyslogSdValue*` + `void* context`) — used by `SolidSyslogMetaSdConfig` (language) and `SolidSyslogOriginSdConfig`. |
| `SolidSyslogNullSd.h` | Any code installing a no-op Structured Data slot in `SolidSyslogConfig.Sd[]` | `SolidSyslogNullSd_Get` |
| `SolidSyslogMetaSd.h` | System setup code using meta SD (sequenceId, sysUpTime, language) | `SolidSyslogMetaSdConfig` (counter, getSysUpTime, getLanguage — each independently optional via NULL; `getLanguage` is a `SolidSyslogSdValueFunction`), `SolidSyslogSysUpTimeFunction`, `SolidSyslogMetaSd_Create(config)`, `_Destroy(sd)`. Instance struct lives in a library-internal static pool (E11). Bad-config and pool exhaustion both resolve to the shared `SolidSyslogNullSd`. |
| `SolidSyslogMetaSdErrors.h` | Any code installing an error handler that wants to react to MetaSd-specific events (pointer-identity match on `MetaSdErrorSource`, switch on `enum SolidSyslogMetaSdErrors`) | `enum SolidSyslogMetaSdErrors` (`METASD_ERROR_*` codes + `METASD_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource MetaSdErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogAtomicCounter.h` | Any code holding a counter handle | `SolidSyslogAtomicCounter_Increment(base)` — public vtable-dispatched call. Wrap-aware in [1, 2³¹ - 1] per RFC 5424 §7.3.1, never returns 0. |
| `SolidSyslogAtomicCounterDefinition.h` | AtomicCounter implementors (extension point) | `SolidSyslogAtomicCounter` vtable struct (`Increment` function pointer) |
| `SolidSyslogNullAtomicCounter.h` | Any code installing a no-op counter slot (`Increment` returns `1U` unconditionally — RFC 5424 §7.3.1 forbids `0`, and `1` is indistinguishable from the post-power-on / post-wrap state) | `SolidSyslogNullAtomicCounter_Get` |
| `SolidSyslogStdAtomicCounter.h` | System setup code on platforms with C11 `<stdatomic.h>` | `SolidSyslogStdAtomicCounter_Create(void)`, `_Destroy(base)`. Uses `_Atomic uint32_t` + `atomic_compare_exchange_strong_explicit` CAS loop. Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullAtomicCounter`. |
| `SolidSyslogStdAtomicCounterErrors.h` | Any code installing an error handler that wants to react to StdAtomicCounter-specific events (pointer-identity match on `StdAtomicCounterErrorSource`, switch on `enum SolidSyslogStdAtomicCounterErrors`) | `enum SolidSyslogStdAtomicCounterErrors` (`STDATOMICCOUNTER_ERROR_*` codes + `STDATOMICCOUNTER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource StdAtomicCounterErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogWindowsAtomicCounter.h` | System setup code on Windows targets without `<stdatomic.h>` (legacy MSVC) | `SolidSyslogWindowsAtomicCounter_Create(void)`, `_Destroy(base)`. Uses `volatile LONG` + `InterlockedCompareExchange` CAS loop. Instance struct lives in a library-internal static pool (E11). Pool-exhaustion fallback is the shared `SolidSyslogNullAtomicCounter`. |
| `SolidSyslogWindowsAtomicCounterErrors.h` | Any code installing an error handler that wants to react to WindowsAtomicCounter-specific events (pointer-identity match on `WindowsAtomicCounterErrorSource`, switch on `enum SolidSyslogWindowsAtomicCounterErrors`) | `enum SolidSyslogWindowsAtomicCounterErrors` (`WINDOWSATOMICCOUNTER_ERROR_*` codes + `WINDOWSATOMICCOUNTER_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource WindowsAtomicCounterErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogTimeQuality.h` | Any code providing time quality data | `SolidSyslogTimeQuality`, `SolidSyslogTimeQualityFunction`, `SOLIDSYSLOG_SYNC_ACCURACY_OMIT` |
| `SolidSyslogTimeQualitySd.h` | System setup code using timeQuality SD | `SolidSyslogTimeQualitySd_Create(getTimeQuality)`, `_Destroy(sd)`. Instance struct lives in a library-internal static pool (E11). Pool exhaustion resolves to the shared `SolidSyslogNullSd`. |
| `SolidSyslogTimeQualitySdErrors.h` | Any code installing an error handler that wants to react to TimeQualitySd-specific events (pointer-identity match on `TimeQualitySdErrorSource`, switch on `enum SolidSyslogTimeQualitySdErrors`) | `enum SolidSyslogTimeQualitySdErrors` (`TIMEQUALITYSD_ERROR_*` codes + `TIMEQUALITYSD_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource TimeQualitySdErrorSource`. Integrators ignore if not handling errors per source. |
| `SolidSyslogOriginSd.h` | System setup code using origin SD (software, swVersion, enterpriseId, ip) | `SolidSyslogOriginSdConfig` (software, swVersion, enterpriseId, getIpCount, getIpAt — each independently optional via NULL), `SolidSyslogOriginIpCountFunction`, `SolidSyslogOriginIpAtFunction`, `SolidSyslogOriginSd_Create(config)`, `_Destroy(sd)`. `getIpAt` is a `SolidSyslogOriginIpAtFunction` writing each address into an `SolidSyslogSdValue*`. Instance struct lives in a library-internal static pool (E11); each slot borrows its config strings and emits the whole element at Format time (no pre-formatted scratch storage). Pool exhaustion resolves to the shared `SolidSyslogNullSd`. |
| `SolidSyslogOriginSdErrors.h` | Any code installing an error handler that wants to react to OriginSd-specific events (pointer-identity match on `OriginSdErrorSource`, switch on `enum SolidSyslogOriginSdErrors`) | `enum SolidSyslogOriginSdErrors` (`ORIGINSD_ERROR_*` codes + `ORIGINSD_ERROR_MAX` bookend), `extern const struct SolidSyslogErrorSource OriginSdErrorSource`. Integrators ignore if not handling errors per source. |

Most application code only needs `SolidSyslog.h` — it never sees allocators, senders, buffers, or config structs.

---

## Naming Conventions

`docs/NAMING.md` is the source of truth — a per-tier naming scheme
satisfying MISRA C:2012 rules 5.1–5.9 with clang-tidy enforcing shape
and cppcheck-misra enforcing uniqueness. Read it before adding any new
public identifier.

One-line summary: public C functions `SolidSyslogClass_Function`, public
types `SolidSyslogClass`, public macros `SOLIDSYSLOG_SCREAMING_SNAKE`,
file-scope statics `Class_Function` / `CLASS_SCREAMING_SNAKE`,
struct members `PascalCase` (data + vtable function-pointer alike),
locals/parameters `lowerCamelCase`, files `PascalCase.c`. No
Hungarian notation. No member-variable prefixes. No `typedef struct`
for project-owned struct types.

Deliberate deviations from the MISRA rule set are recorded in
`docs/misra-deviations.md`.

---

## Code Style

- Formatting is enforced by clang-format. Run format-on-save or `clang-format -i` before committing.
  CI will reject unformatted code.
- clang-tidy checks are configured in `.clang-tidy`. All warnings are errors.
- All compiler warnings are errors (`-Werror`). Do not suppress warnings without strong justification.
- cppcheck runs with `--error-exitcode=1`. Inline suppressions (`// cppcheck-suppress`) must include
  a comment explaining why.

### MISRA-load-bearing `.clang-format` settings

Two settings in `.clang-format` are not merely stylistic — they enforce MISRA C:2012 rules at
format-on-save:

- **`InsertBraces: true`** combined with `AllowShortIfStatementsOnASingleLine: Never`,
  `AllowShortLoopsOnASingleLine: false`, `AllowShortFunctionsOnASingleLine: None`, and
  `AllowShortBlocksOnASingleLine: Never` — formatter-side enforcement of **MISRA 15.6**
  (the body of an iteration- or selection-statement shall be a compound-statement). clang-format
  rewrites your code to add the braces if they are missing, and the `AllowShort*` settings
  stop them being collapsed back onto a single line.
- **`RemoveParentheses: Leave`** — keeps the project **MISRA 12.1 safe**. The advisory rule
  prefers explicit precedence parentheses; flipping this to `MultipleParentheses` would let
  clang-format strip them.

Do not change either group of settings without understanding the MISRA consequence.
See `docs/misra-deviations.md` for the project's stance on MISRA conformance.

---

## Design Patterns

These patterns are re-affirmed each time we do a code-hygiene pass. New
code should follow them; reviewers should call out drift.

### Production code (Tier 1, `Core/Source/`)

- **Single return per function.** MISRA-leaning. If the natural shape has an
  early return, restructure with a result local and an `if` wrapper. See
  `BlockSequence.c::ScanForExistingBlocks` for the pattern.
- **Intent-naming static-inline predicates.** When a composite condition is
  inlined into an `if` or a `return`, extract a `static inline bool IsXxx(...)`
  helper. The helper's *name* is the documentation. Examples:
  `BlockSequence_IsAboveThreshold`, `FileBlockDevice_IsHandleAlreadyOpenOnBlock`,
  `FileBlockDevice_IsValidBlockIndex`, `BlockSequence_BlockIsFull`,
  `BlockSequence_StoreIsFull`. The cost (one extra named function) is the
  benefit (the reader doesn't have to decode the boolean).
- **One thing at one level of abstraction.** Functions read top-down.
  `_Create` first, `_Destroy` second, public functions in API order, helpers
  defined immediately beneath their first caller. See **Function Ordering**
  below.
- **Bracket compound boolean conditions when mixing `||` with arithmetic /
  comparison operators.** Plain `&&` between bool-typed operands needs no extra
  parens — readability wins over MISRA pedantry there.
- **No null-pointer checks where the type's null object exists.** Use
  `SolidSyslogNullSecurityPolicy`, `SolidSyslogNullStore` rather than
  `if (policy != NULL) policy->Compute(...)`.

### Test code

- **TEST_BASE / TEST_GROUP_BASE for shared fixture.** When multiple TEST_GROUPs
  in one file declare the same storage / file / device variables and the same
  setup/teardown, lift the boilerplate into a `TEST_BASE` and derive each group
  via `TEST_GROUP_BASE`. Test bodies still reference fixture members by their
  bare names — they're inherited. See `BlockDeviceTestBase` in
  `SolidSyslogBlockStoreTest.cpp`.
- **`CHECK_*` macros for repeated assertion shapes.** When the same buf+memcmp
  or several-line assertion repeats across tests, wrap it in a macro that
  *names* the intent. The macro must be a macro (not a function) so test
  failures report the caller's `__FILE__`/`__LINE__`. A multi-statement body
  is a plain compound `{ ... }`; a single-statement body is just the bare
  expression. Do **not** use the `do { ... } while (0)` wrapper: its only job
  is to stop a multi-statement macro's tail escaping an unbraced `if`/`else`,
  and `.clang-format`'s `InsertBraces: true` (our MISRA 15.6 enforcement)
  already braces every conditional body before the macro expands, so the
  wrapper is dead weight — and `cppcoreguidelines-avoid-do-while` (kept on for
  the `Tests/` tier) now rejects it. No `NOLINT` is needed: `Tests/.clang-tidy`
  disables `cppcoreguidelines-macro-usage` tier-wide, so a `CHECK_*` macro
  needs no per-site suppression. Examples: `CHECK_PRIVAL` (single-statement,
  bare expression) in `SolidSyslogMessageFormatterTest.cpp`,
  `CHECK_BLOCK_CONTAINS` (declares a local, so keeps its `{ ... }` block) in
  `SolidSyslogFileBlockDeviceTest.cpp`.
- **DRY the setup, DRY the asserts, keep the test body small.** Each `TEST(...)`
  body should read as a sentence: arrange → act → assert. If three lines of
  setup repeat in five tests, the setup belongs in `setup()` or a TEST_GROUP
  helper; if the same assertion shape repeats, make a `CHECK_*` macro.

---

## Callback Conventions

The library is migrating callbacks toward a `void* context` parameter. The migration is
**opportunistic per-class** — not a sweep — so older context-less callbacks
(`SolidSyslogClockFunction`, `SolidSyslogStringFunction`, `SolidSyslogStoreFullCallback`, etc.)
keep their current shape until the class that owns them is next touched.

**For new callbacks:**

- The function pointer takes a `void* context` parameter.
- The config struct exposes a paired context field. When several callbacks form a logical
  feature, share one context field (e.g. `thresholdContext` shared between the threshold
  function and the threshold-crossed callback).
- Treat the context as opaque from the integrator's side — the library passes it through unchanged.

**For existing callbacks:**

- Migrate at the same time as a refactor or significant modification of the owning class. For
  example, `SolidSyslogStoreFullCallback` migrates inside the FileStore split (S18.01), not in a
  separate sweep PR.

## Pool Allocation (E11)

Every stateful Created class lives in a library-internal static pool of N slots, sized by a
`SOLIDSYSLOG_<CLASS>_POOL_SIZE` tunable in `Core/Interface/SolidSyslogTunablesDefaults.h`. The
public `_Create` accepts a config struct and returns an opaque handle (a pointer into the
pool); `_Destroy` takes the handle and releases the slot. Pool semantics:

Platform- and vendor-selected classes (TCP stream, datagram, resolver, mutex, file,
atomic counter, TLS stream, HMAC policy) share a **role-named** tunable rather than one
name per implementation — `SOLIDSYSLOG_TCP_STREAM_POOL_SIZE`, not a per-platform
`SOLIDSYSLOG_POSIX_TCP_STREAM_*` name. A build links one implementation per role, so
the integrator tunes the role. See `docs/NAMING.md`, *Pool-size tunables are named by
role, not platform*, for the rule and the two-implementations-in-one-build caveat.

- **No `malloc`.** Pools are file-scope `static` arrays. Integrators on bare-metal /
  FreeRTOS-static-allocation / DO-178C-style targets get the same code path as hosted targets.
- **Pool exhaustion** falls back to a shared null sibling — `SolidSyslogNullSender`,
  `SolidSyslogNullBuffer`, `SolidSyslogNullStore`, etc. — whose vtable methods are safe no-ops.
  Caller code keeps running; the integrator sees `SolidSyslog_Error(ERR, ...)` at the
  exhaustion site if a handler is installed.
- **Slot-walk synchronisation.** Every pool's Create / Destroy wraps its slot probe in the
  `SolidSyslog_LockConfig` / `_UnlockConfig` injection pair. Single-task targets get the
  no-op default; multi-task targets wire `taskENTER_CRITICAL` (FreeRTOS), `pthread_mutex_lock`
  (POSIX), `EnterCriticalSection` (Windows), etc.
- **Shared helper.** `Core/Source/SolidSyslogPoolAllocator.{h,c}` (TU-internal) owns the
  three-operation contract (`AcquireFirstFree`, `FreeIfInUse`, `IndexIsValid`) every
  pool class reuses. No class re-implements the slot walk.

**Caller-supplied storage** survives in two places only — both intentionally outside the
pool pattern. `SolidSyslogFormatter` is a transient stack-built builder whose payload size
is per-call (`SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(n)`); `SolidSyslogAddress` is a value type
(`SolidSyslogAddressStorage` + `SOLIDSYSLOG_ADDRESS_SIZE`) with no `_Create`/`_Destroy`
lifecycle. Both are documented under deviation D.002 in `docs/misra-deviations.md`.

**Internal sub-components** of pool-allocated classes (e.g. `RecordStore` and
`BlockSequence` inside `BlockStore`) live in sibling pools sized off the parent's
tunable. Their types stay in `Core/Source/` and never appear in public headers, so
integrator and example code physically cannot reach them.

---

## Function Ordering

Within a source file, functions are ordered top-down so the reader sees the lifecycle and public API
first, then drills into helpers as they appear:

1. `_Create` function first.
2. `_Destroy` function second.
3. Other public functions after, in whatever order reads naturally (often call order).
4. Helper functions are **forward-declared** at the top of the file (after constants/types, before
   the first definition), usually `static inline`, and **defined immediately beneath the function
   that first calls them**. If a second public function also calls that helper, the helper stays
   where it was — with its first caller.

This puts "what the file does" at the top, and every helper next to its nearest use. Forward
declarations are the price paid to keep that top-down reading order.

---

## Container Images

See [`docs/containers.md`](docs/containers.md) for the full image reference, Docker Compose setup,
and switching procedure.

When updating an image:

1. Build and push the new image in the container image repo
2. Update the SHA tag in all files that reference it (see [`docs/containers.md`](docs/containers.md) for the full list)
3. Rebuild the devcontainer and verify the new tooling works locally
4. Then commit — use `chore: bump container image to <sha>`

---

## Banned API Policy (Microsoft SDL)

Production code must never use Microsoft SDL banned functions (`strncpy`, `sprintf`, `sscanf`,
`strtok`, etc.). The library uses `SolidSyslogFormatter` for string building and `strtol` for
parsing — this is deliberate and must be maintained.

Test code uses the `SafeString` abstraction (`Tests/Support/SafeString.h`) instead of calling
`strncpy` directly. CMake selects the platform implementation at build time:

- **Windows** (`SafeStringWindows.c`): wraps `strncpy_s` with `_TRUNCATE`
- **Default** (`SafeStringStandard.c`): wraps `strncpy` + null-terminate

SafeString is compiled into test executables only — never linked into the production library.

`_CRT_SECURE_NO_WARNINGS` was removed from CMakeLists.txt and must not be re-added.
`memset`, `memcpy`, and `strcmp` are not SDL-banned and do not trigger MSVC C4996.

---

## Bash execution rules

- The Bash tool runs in a persistent session. The working directory is
  dynamic — it is NOT fixed at the launch directory and persists across calls.
- Prefer absolute paths in all file and shell operations. Do not rely on the
  current working directory being where you expect it.
- Do not use bare `cd` to move between dependent steps. If a sequence needs a
  specific directory, chain it in ONE command with `&&`
  (e.g. `cd /abs/path && cmd`) so the directory and the command succeed or
  fail together.
- Never issue multiple parallel Bash calls that depend on shared session state
  (working directory, environment variables, or each other's output). Run
  interdependent commands sequentially in a single Bash call.
- Only parallelise Bash calls that are genuinely independent and read-only
  (e.g. `git status`, `git diff`, `git log`).
- If a command fails, do not assume the session state (including cwd) is intact;
  re-establish it explicitly before continuing.
