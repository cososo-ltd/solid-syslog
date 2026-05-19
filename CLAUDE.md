# Claude Code Guidelines

## Read this with SKILL.md

Also read [`SKILL.md`](SKILL.md) at session start. It holds cross-context conventions
that the VSCode extension does not auto-load — notably the **DEVLOG cadence**
(append an entry after every meaningful session) and the **TDD pairing contract**.
This file (CLAUDE.md) covers implementation and workflow details; SKILL.md covers
the "why" and the habits that span Windows host / WSL / container sessions.

## Git Workflow

All changes to `main` must go via a pull request — direct pushes are blocked by branch protection.

**Branch naming:** `<type>/<short-description>` — e.g. `feat/clang-preset`, `ci/pin-action-shas`

**Merge strategy:** Squash merge only. This keeps a linear history on `main` and means the PR title
becomes the single commit message — so the PR title must follow Conventional Commits format (see below).

**Before raising a PR — run locally:**
- build-linux-gcc, build-linux-clang, sanitize-linux-gcc, coverage-linux-gcc, analyze-tidy, analyze-cppcheck, analyze-format
- Windows, BDD, and OpenSSL integration jobs are CI's responsibility — running them locally would slow development too much
- Commits on the branch can be informal (work-in-progress messages are fine)
- The PR title is what matters — it becomes the permanent commit message on `main`

**Branch protection rules (configured on GitHub):**
- Direct pushes to `main` are blocked
- PRs require all status checks to pass before merging: build-linux-gcc, build-linux-clang, build-windows-msvc, sanitize-linux-gcc, coverage-linux-gcc, analyze-tidy, analyze-cppcheck, analyze-format, analyze-iwyu, integration-linux-openssl, integration-windows-openssl, bdd-linux-syslog-ng, bdd-windows-otel, bdd-freertos-qemu, build-freertos-host-tdd, build-freertos-target, summary
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
  repository(owner: "DavidCozens", name: "solid-syslog") {
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
  repository(owner: "DavidCozens", name: "solid-syslog") {
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
  repository(owner: "DavidCozens", name: "solid-syslog") {
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

```
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

```
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
| `SolidSyslog.h` | Application code that logs events | `SolidSyslogMessage`, `SolidSyslog_Log`, `SolidSyslog_Service` |
| `SolidSyslogConfig.h` | System setup code | `SolidSyslogConfig`, `SolidSyslog_Create`, `SolidSyslog_Destroy` |
| `SolidSyslogError.h` | Any code installing a handler to react to library-internal errors (NULL guards, send failures); also the library-internal call site for emitting them | `SolidSyslogErrorHandler` typedef, `SolidSyslog_SetErrorHandler(handler, context)`, `SolidSyslog_Error(severity, message)`. Default handler is a silent no-op; setting `handler = NULL` restores the default. Single global slot — intended for setup-time configuration, not synchronised with concurrent `Error` calls. |
| `SolidSyslogConfigLock.h` | Setup code on systems where library-internal config-time critical sections (E11 pool `_Create`/`_Destroy` slot walks) may race across tasks or cores. Single-task setup needs nothing — defaults are no-ops. | `SolidSyslogConfigLockFunction` typedef (zero-arg `void(void)`), `SolidSyslog_SetConfigLock(lockFn, unlockFn)`, `SolidSyslog_LockConfig()`, `SolidSyslog_UnlockConfig()`. Pair API: both handlers installed together. `NULL` on either side restores that side's no-op default. Single global slot — intended for setup-time configuration, not synchronised with concurrent installs. Integrators wire `taskENTER_CRITICAL`/`taskEXIT_CRITICAL` (FreeRTOS), `pthread_mutex_lock`/`unlock` on a static `pthread_mutex_t` (POSIX), `EnterCriticalSection`/`LeaveCriticalSection` on a static `CRITICAL_SECTION` (Windows), or a spinlock pair. This is the only synchronisation primitive available to the Mutex and AtomicCounter pools for their own pool walks — chicken-and-egg eliminates their own injectables. |
| `SolidSyslogStringFunction.h` | Any code needing the string-callback typedef | `SolidSyslogStringFunction` (callback writes into a `SolidSyslogFormatter*`) — used by `SolidSyslogConfig` (hostname/appName/processId), `SolidSyslogMetaSdConfig` (language), `SolidSyslogOriginSdConfig` |
| `SolidSyslogFormatter.h` | Any code that formats into a bounded buffer | `SolidSyslogFormatter`, `SolidSyslogFormatterStorage`, `SOLIDSYSLOG_FORMATTER_STORAGE_SIZE`, `_Create`, `_FromStorage`, `_AsciiCharacter`, `_Bom`, `_BoundedString`, `_EscapedString`, `_PrintUsAsciiString`, `_Uint32`, `_TwoDigit`, `_FourDigit`, `_SixDigit`, `_AsFormattedBuffer`, `_Length` |
| `SolidSyslogPrival.h` | Any code that needs facility/severity enums | `SolidSyslogFacility`, `SolidSyslogSeverity` |
| `SolidSyslogTimestamp.h` | Any code that needs the timestamp struct | `SolidSyslogTimestamp`, `SolidSyslogClockFunction` |
| `SolidSyslogEndpoint.h` | System setup code that supplies destination host/port (and version on changes) | `SolidSyslogEndpoint`, `SolidSyslogEndpointFunction`, `SolidSyslogEndpointVersionFunction`, `SOLIDSYSLOG_MAX_HOST_SIZE` |
| `SolidSyslogSender.h` | Any code holding a sender handle | `SolidSyslogSender_Send`, `SolidSyslogSender_Disconnect` |
| `SolidSyslogSenderDefinition.h` | Sender implementors (extension point) | `SolidSyslogSender` vtable struct (`Send`, `Disconnect`) |
| `SolidSyslogNullSender.h` | Any code installing a no-op sender slot (Send returns `true` to drop on the floor — keeps Store from filling with undeliverables; Disconnect is a no-op) | `SolidSyslogNullSender_Get` |
| `SolidSyslogTransport.h` | Any code selecting a transport or needing default port constants | `SolidSyslogTransport` enum (`UDP`, `TCP`), `SOLIDSYSLOG_UDP_DEFAULT_PORT`, `SOLIDSYSLOG_TCP_DEFAULT_PORT` |
| `SolidSyslogResolver.h` | Any code that needs to resolve a destination | `SolidSyslogResolver_Resolve(resolver, transport, host, port, *out)` |
| `SolidSyslogResolverDefinition.h` | Resolver implementors (extension point) | `SolidSyslogResolver` vtable struct (`Resolve`) |
| `SolidSyslogFreeRtosStaticResolver.h` | System setup code on FreeRTOS targets pinned to a hardcoded IPv4 destination (no DNS) | `SolidSyslogFreeRtosStaticResolverStorage`, `SOLIDSYSLOG_FREERTOSSTATICRESOLVER_SIZE`, `SolidSyslogFreeRtosStaticResolver_Create(storage, ipv4Octets)`, `_Destroy(resolver)` (ignores host and transport at Resolve time; port is taken from each call) |
| `SolidSyslogDatagram.h` | Sender implementors using datagram transport | `SolidSyslogDatagram_Open`, `_SendTo`, `_Close` |
| `SolidSyslogDatagramDefinition.h` | Datagram implementors (extension point) | `SolidSyslogDatagram` vtable struct (`Open`, `SendTo`, `Close`) |
| `SolidSyslogFreeRtosDatagram.h` | System setup code on FreeRTOS targets using FreeRTOS-Plus-TCP for UDP | `SolidSyslogFreeRtosDatagramStorage`, `SOLIDSYSLOG_FREERTOSDATAGRAM_SIZE`, `SolidSyslogFreeRtosDatagram_Create(storage)`, `_Destroy(datagram)` (wraps `FreeRTOS_socket` / `FreeRTOS_sendto` / `FreeRTOS_closesocket`) |
| `SolidSyslogStream.h` | Sender implementors using stream transport | `SolidSyslogStream_Open`, `_Send`, `_Read`, `_Close`, `SolidSyslogSsize` |
| `SolidSyslogStreamDefinition.h` | Stream implementors (extension point) | `SolidSyslogStream` vtable struct (`Open`, `Send`, `Read`, `Close`) |
| `SolidSyslogFreeRtosTcpStream.h` | System setup code on FreeRTOS targets using FreeRTOS-Plus-TCP for TCP | `SolidSyslogFreeRtosTcpStreamStorage`, `SOLIDSYSLOG_FREERTOSTCPSTREAM_SIZE`, `SolidSyslogFreeRtosTcpStream_Create(storage)`, `_Destroy(stream)` (wraps `FreeRTOS_socket` / `FreeRTOS_connect` / `FreeRTOS_send` / `FreeRTOS_recv` / `FreeRTOS_closesocket`; bounded blocking connect via `SO_SNDTIMEO=200ms`, cleared post-connect so Send/Read are non-blocking) |
| `SolidSyslogUdpSender.h` | System setup code using UDP transport | `SolidSyslogUdpSenderConfig` (resolver, datagram, endpoint, endpointVersion), `SolidSyslogUdpSender_Create(config)`, `_Destroy(sender)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create`. Pool-exhaustion and bad-config fallback is the shared `SolidSyslogNullSender`. |
| `SolidSyslogStreamSender.h` | System setup code using octet-framed transport (RFC 6587) over any Stream — `SolidSyslogPosixTcpStream` (POSIX TCP), `SolidSyslogWinsockTcpStream` (Windows TCP), `SolidSyslogFreeRtosTcpStream` (FreeRTOS-Plus-TCP), `SolidSyslogTlsStream` (TLS; OpenSSL reference integration), or a caller-supplied Stream backend | `SolidSyslogStreamSenderConfig` (resolver, stream, endpoint, endpointVersion), `SolidSyslogStreamSender_Create(config)`, `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create`. Pool exhaustion resolves to the shared `SolidSyslogNullSender`. |
| `SolidSyslogSwitchingSender.h` | System setup code composing multiple inner senders | `SolidSyslogSwitchingSenderConfig` (senders, senderCount, selector), `SolidSyslogSwitchingSenderSelector`, `SolidSyslogSwitchingSender_Create(config)`, `_Destroy(sender)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create`. Out-of-range selector and pool exhaustion both resolve to the shared `SolidSyslogNullSender`. |
| `SolidSyslogBuffer.h` | Library internals consuming a buffer (Service algorithm) | `SolidSyslogBuffer_Write`, `_Read` |
| `SolidSyslogNullBuffer.h` | Any code installing a no-op buffer slot (Read returns `false`/`bytesRead=0`; Write swallows the record) | `SolidSyslogNullBuffer_Get` |
| `SolidSyslogBufferDefinition.h` | Buffer implementors (extension point) | `SolidSyslogBuffer` vtable struct |
| `SolidSyslogPassthroughBuffer.h` | System setup code (single-task, no buffering) | `SolidSyslogPassthroughBuffer_Create(sender)`, `_Destroy(buffer)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create` to release the slot. Pool exhaustion resolves to the shared `SolidSyslogNullBuffer`. |
| `SolidSyslogPosixMessageQueueBuffer.h` | System setup code using POSIX message queue buffer | `SolidSyslogPosixMessageQueueBuffer_Create`, `_Destroy` |
| `SolidSyslogCircularBuffer.h` | System setup code using an in-memory ring buffer (bare-metal / RTOS / Windows) | `SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES`, `SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(maxMessages)` (friendly: N max-sized messages → ring bytes), `SolidSyslogCircularBuffer_Create(mutex, ring, ringBytes)`, `_Destroy(buffer)` (uint16-length-prefixed records, drop-newest on overflow, no-split wrap, mutex-injected synchronisation). Instance struct lives in a library-internal static pool (E11); caller supplies the ring memory only. Pool exhaustion resolves to the shared `SolidSyslogNullBuffer`. |
| `SolidSyslogMutex.h` | Any code holding a mutex handle | `SolidSyslogMutex_Lock`, `_Unlock` |
| `SolidSyslogMutexDefinition.h` | Mutex implementors (extension point) | `SolidSyslogMutex` vtable struct (`Lock`, `Unlock`) |
| `SolidSyslogNullMutex.h` | System setup code (single-task, no synchronisation) | `SolidSyslogNullMutex_Create`, `_Destroy` |
| `SolidSyslogPosixMutex.h` | System setup code on POSIX targets needing thread-safe buffering | `SolidSyslogPosixMutexStorage`, `SOLIDSYSLOG_POSIXMUTEX_SIZE`, `SolidSyslogPosixMutex_Create(storage)`, `_Destroy(mutex)` (wraps `pthread_mutex_t`) |
| `SolidSyslogWindowsMutex.h` | System setup code on Windows targets needing thread-safe buffering | `SolidSyslogWindowsMutexStorage`, `SOLIDSYSLOG_WINDOWSMUTEX_SIZE`, `SolidSyslogWindowsMutex_Create(storage)`, `_Destroy(mutex)` (wraps `CRITICAL_SECTION`) |
| `SolidSyslogFreeRtosMutex.h` | System setup code on FreeRTOS targets needing thread-safe buffering | `SolidSyslogFreeRtosMutexStorage`, `SOLIDSYSLOG_FREERTOSMUTEX_SIZE`, `SolidSyslogFreeRtosMutex_Create(storage)`, `_Destroy(mutex)` (wraps `xSemaphoreCreateMutexStatic` — caller's `StaticSemaphore_t` lives inside the injected storage; requires `configSUPPORT_STATIC_ALLOCATION=1`) |
| `SolidSyslogStore.h` | Library internals consuming a store (Service algorithm) and integrator code querying capacity | `SolidSyslogStore_Write`, `_ReadNextUnsent`, `_MarkSent`, `_HasUnsent`, `_IsHalted`, `_GetTotalBytes`, `_GetUsedBytes` |
| `SolidSyslogStoreDefinition.h` | Store implementors (extension point) | `SolidSyslogStore` vtable struct (`Write`, `ReadNextUnsent`, `MarkSent`, `HasUnsent`, `IsHalted`, `GetTotalBytes`, `GetUsedBytes`) |
| `SolidSyslogNullStore.h` | System setup code (no store-and-forward) | `SolidSyslogNullStore_Get` |
| `SolidSyslogFileDefinition.h` | File implementors (extension point) | `SolidSyslogFile` vtable struct |
| `SolidSyslogFile.h` | Any code using the file abstraction | `SolidSyslogFile_Open`, `_Close`, `_IsOpen`, `_Read`, `_Write`, `_SeekTo`, `_Size`, `_Truncate` |
| `SolidSyslogBlockDevice.h` | Library internals consuming a block device (BlockSequence inside BlockStore) and integrator code addressing blocks directly | `SolidSyslogBlockDevice_Acquire`, `_Dispose`, `_Exists`, `_Read`, `_Append`, `_WriteAt`, `_Size` (block-indexed I/O; Acquire makes a block ready for fresh writes, Dispose releases it) |
| `SolidSyslogNullBlockDevice.h` | Any code installing a no-op block device slot (every method returns `false` / `0` — disk doesn't exist) | `SolidSyslogNullBlockDevice_Get` |
| `SolidSyslogBlockDeviceDefinition.h` | BlockDevice implementors (extension point) | `SolidSyslogBlockDevice` vtable struct (`Acquire`, `Dispose`, `Exists`, `Read`, `Append`, `WriteAt`, `Size`) |
| `SolidSyslogFileBlockDevice.h` | System setup code wiring a file-backed block device | `SolidSyslogFileBlockDevice_Create(file, pathPrefix)` (sequence-numbered filenames `<prefix><NN>.log`, one cached file handle), `_Destroy(base)`. Instance struct lives in a library-internal static pool (E11); `_Destroy` takes the handle returned by `_Create`. Pool exhaustion resolves to the shared `SolidSyslogNullBlockDevice`. |
| `SolidSyslogBlockStore.h` | System setup code using a BlockDevice-backed store | `SolidSyslogBlockStoreConfig` (with `blockDevice`, `storeFullContext`, `getCapacityThreshold`, `onThresholdCrossed`, `thresholdContext`), `SolidSyslogBlockStore_Create(config)`, `_Destroy(store)`, `SolidSyslogDiscardPolicy` (`SolidSyslogDiscardPolicy_Oldest` / `_Newest` / `_Halt`), `SolidSyslogStoreFullCallback(void* context)`, `SolidSyslogStoreThresholdFunction(void* context)` (returns threshold in bytes; 0 disables; queried each Write), `SolidSyslogStoreThresholdCallback(void* context)` (edge-triggered; PassthroughBuffer recursion gotcha — see header). Instance struct lives in a library-internal static pool (E11); each slot composes one inner RecordStore + BlockSequence drawn 1:1 from sibling pools sized off the same `SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE` tunable. Pool exhaustion (or either inner Create failing) resolves to the shared `SolidSyslogNullStore`. |
| `SolidSyslogSecurityPolicyDefinition.h` | SecurityPolicy implementors (extension point) | `SolidSyslogSecurityPolicy` vtable struct, `SOLIDSYSLOG_MAX_INTEGRITY_SIZE` |
| `SolidSyslogNullSecurityPolicy.h` | System setup code (no integrity checking) | `SolidSyslogNullSecurityPolicy_Get` |
| `SolidSyslogCrc16Policy.h` | System setup code using CRC-16 integrity | `SolidSyslogCrc16Policy_Create`, `_Destroy` |
| `SolidSyslogCrc16.h` | Any code needing CRC-16 computation | `SolidSyslogCrc16_Compute` |
| `SolidSyslogPosixFile.h` | System setup code using POSIX file I/O | `SolidSyslogPosixFile_Create`, `_Destroy` |
| `SolidSyslogWindowsFile.h` | System setup code using Windows file I/O (MSVC `<io.h>`) | `SolidSyslogWindowsFile_Create`, `_Destroy` |
| `SolidSyslogFatFsFile.h` | System setup code on targets using ChaN FatFs as the file layer (bare-metal flash / SD / eMMC, FreeRTOS, Zephyr, NuttX, …) | `SolidSyslogFatFsFileStorage`, `SOLIDSYSLOG_FATFS_FILE_SIZE`, `SolidSyslogFatFsFile_Create(storage)`, `_Destroy(file)` (wraps `f_open` / `f_close` / `f_read` / `f_write` with `f_sync` after every successful write so a power loss never loses a record the BlockStore claimed it stored; integrator supplies `diskio.c` and — if `FF_FS_REENTRANT=1` — `ffsystem.c`) |
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
| `SolidSyslogStructuredData.h` | Library internals (SD dispatch) | `SolidSyslogStructuredData_Format` (writes into `SolidSyslogFormatter*`) |
| `SolidSyslogStructuredDataDefinition.h` | SD implementors (extension point) | `SolidSyslogStructuredData` vtable struct (Format takes `SolidSyslogFormatter*`) |
| `SolidSyslogNullSd.h` | Any code installing a no-op Structured Data slot in `SolidSyslogConfig.Sd[]` | `SolidSyslogNullSd_Get` |
| `SolidSyslogMetaSd.h` | System setup code using meta SD (sequenceId, sysUpTime, language) | `SolidSyslogMetaSdConfig` (counter, getSysUpTime, getLanguage — each independently optional via NULL), `SolidSyslogSysUpTimeFunction`, `SolidSyslogMetaSd_Create(config)`, `_Destroy(sd)`. Instance struct lives in a library-internal static pool (E11). Bad-config and pool exhaustion both resolve to the shared `SolidSyslogNullSd`. |
| `SolidSyslogAtomicCounter.h` | Any code holding a counter handle | `SolidSyslogAtomicCounter_Increment(base)` — public vtable-dispatched call. Wrap-aware in [1, 2³¹ - 1] per RFC 5424 §7.3.1, never returns 0. |
| `SolidSyslogAtomicCounterDefinition.h` | AtomicCounter implementors (extension point) | `SolidSyslogAtomicCounter` vtable struct (`Increment` function pointer) |
| `SolidSyslogStdAtomicCounter.h` | System setup code on platforms with C11 `<stdatomic.h>` | `SolidSyslogStdAtomicCounterStorage`, `SOLIDSYSLOG_STDATOMICCOUNTER_SIZE`, `SolidSyslogStdAtomicCounter_Create(storage)`, `_Destroy(base)`. Uses `_Atomic uint32_t` + `atomic_compare_exchange_strong_explicit` CAS loop. |
| `SolidSyslogWindowsAtomicCounter.h` | System setup code on Windows targets without `<stdatomic.h>` (legacy MSVC) | `SolidSyslogWindowsAtomicCounterStorage`, `SOLIDSYSLOG_WINDOWSATOMICCOUNTER_SIZE`, `SolidSyslogWindowsAtomicCounter_Create(storage)`, `_Destroy(base)`. Uses `volatile LONG` + `InterlockedCompareExchange` CAS loop. |
| `SolidSyslogTimeQuality.h` | Any code providing time quality data | `SolidSyslogTimeQuality`, `SolidSyslogTimeQualityFunction`, `SOLIDSYSLOG_SYNC_ACCURACY_OMIT` |
| `SolidSyslogTimeQualitySd.h` | System setup code using timeQuality SD | `SolidSyslogTimeQualitySd_Create(getTimeQuality)`, `_Destroy(sd)`. Instance struct lives in a library-internal static pool (E11). Pool exhaustion resolves to the shared `SolidSyslogNullSd`. |
| `SolidSyslogOriginSd.h` | System setup code using origin SD (software, swVersion, enterpriseId, ip) | `SolidSyslogOriginSdConfig` (software, swVersion, enterpriseId, getIpCount, getIpAt — each independently optional via NULL), `SolidSyslogOriginIpCountFunction`, `SolidSyslogOriginIpAtFunction`, `SolidSyslogOriginSd_Create(config)`, `_Destroy(sd)`. Instance struct lives in a library-internal static pool (E11); each slot carries the pre-formatted static-prefix Formatter storage. Pool exhaustion resolves to the shared `SolidSyslogNullSd`. |

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
  failures report the caller's `__FILE__`/`__LINE__`. Wrap with
  `// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)`
  and use `do { ... } while (0)` for safe single-statement use. Examples:
  `CHECK_PRIVAL` in `SolidSyslogTest.cpp`, `CHECK_BLOCK_CONTAINS` in
  `SolidSyslogFileBlockDeviceTest.cpp`.
- **DRY the setup, DRY the asserts, keep the test body small.** Each `TEST(...)`
  body should read as a sentence: arrange → act → assert. If three lines of
  setup repeat in five tests, the setup belongs in `setup()` or a TEST_GROUP
  helper; if the same assertion shape repeats, make a `CHECK_*` macro.

---

## Callback Conventions

The library is migrating away from singleton instances toward integrator-injected storage and
context-pointer callbacks. The migration is **opportunistic per-class** — not a sweep — so older
context-less callbacks (`SolidSyslogClockFunction`, `SolidSyslogStringFunction`,
`SolidSyslogStoreFullCallback`, etc.) keep their current shape until the class that owns them is
next touched.

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

**Storage injection:**

- Mirrors the `SolidSyslogFormatterStorage` pattern. The public header exposes an opaque storage
  type and a `SOLIDSYSLOG_<TYPE>_STORAGE_SIZE` macro; `_Create` takes a pointer to caller-allocated
  storage of that size. No `malloc` anywhere in the library.
- Internal sub-components (e.g. RecordStore and BlockSequence inside BlockStore) nest inside the
  parent's storage blob. Their types stay in `Core/Source/` and never appear in public headers,
  so integrator and example code physically cannot reach them.

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
