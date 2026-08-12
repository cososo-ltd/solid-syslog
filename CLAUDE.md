# Claude Code Guidelines

## Read this with SKILL.md

Also read [`SKILL.md`](SKILL.md) at session start. It holds cross-context conventions
that the VSCode extension does not auto-load — notably the **TDD pairing contract**.

**Division of labour.** SKILL.md holds how we work together: the pairing protocol, the
collaboration modes, and what this project is. CLAUDE.md holds how the repository works:
git and issue workflow, structure, style, and the design patterns. Neither file should
restate the other — where a rule belongs to one, the other links to it.

**Precedence, when two sources disagree.** The repository itself wins: `.clang-format`,
`.clang-tidy`, `.markdownlint-cli2.jsonc`, `CMakePresets.json` and
`.github/workflows/ci.yml` are executable and cannot drift from what actually happens.
Below that, CLAUDE.md and SKILL.md within their own halves as divided above. A briefing
in conversation outranks both — it is the most recent intent — but say so when it
contradicts a file, so the file gets fixed rather than quietly bypassed.

## Git Workflow

All changes to `main` must go via a pull request — direct pushes are blocked by branch protection.

**Branch naming:** `<type>/<short-description>` — e.g. `feat/clang-preset`, `ci/pin-action-shas`

**Merge strategy:** Squash merge only. This keeps a linear history on `main` and means the PR title
becomes the single commit message — so the PR title must follow Conventional Commits format (see below).

**Before raising a PR — see [docs/local-checks.md](docs/local-checks.md)** for the
full tiered pre-PR check budget. One-line summary:

- Per-commit: `debug` build + tests for the matching preset (~30–60 s)
- Pre-push (only when production source changed): `clang-format` reflow, then `scripts/misra_renumber.py` — in that order, because the reflow moves the lines the renumbering annotates (~2–3 min)
- Pre-push (only when Markdown changed): markdownlint over the changed `.md` files (~5 s)
- Everything else (`tidy`, `sanitize`, `coverage`, IWYU, Windows, BDD, integration, FreeRTOS host/cross) — CI's job; do not run locally. IWYU is advisory even in CI, and is not part of any pre-push budget; `docs/local-checks.md` keeps an optional command for the release-cleanup sweep
- If CI surfaces a finding you missed, fix in another commit on the same branch rather than re-running every lane locally

Commits on the branch can be informal (WIP messages are fine). The PR title is
what matters — it becomes the permanent commit message on `main` on squash merge.

**Branch protection rules (configured on GitHub):**

- Direct pushes to `main` are blocked
- PRs require all status checks to pass before merging: CodeQL, analyze-codeql, analyze-cppcheck, analyze-format, analyze-iwyu, analyze-iwyu-freertos-lwip, analyze-iwyu-freertos-plustcp, analyze-markdown, analyze-tidy, analyze-tidy-freertos-lwip, analyze-tidy-freertos-plustcp, bdd-freertos-qemu-lwip, bdd-freertos-qemu-plustcp, bdd-linux-syslog-ng, bdd-windows-otel, build-freertos-host-tdd-plustcp, build-freertos-target-lwip, build-freertos-target-plustcp, build-linux-c89-headers, build-linux-c99, build-linux-clang, build-linux-gcc, build-linux-tunable-override, build-windows-msvc, consumer-smoke-freertos-cross, consumer-smoke-linux, coverage-linux-gcc, docs-build, integration-linux-mbedtls, integration-linux-openssl, integration-windows-openssl, sanitize-linux-gcc, summary, verify-manifest
- The `analyze-iwyu*` lanes run `continue-on-error: true` — they are required contexts but advisory in substance, so they report success whatever IWYU finds
- Code scanning contributes two contexts and both are required. `analyze-codeql` is the Actions job in `codeql.yml`, and proves the analysis ran; `CodeQL` is the code-scanning results check, and is the one that fails when a PR introduces a new alert. Requiring only the job would let a PR add findings and still merge green
- Feeding the `summary` aggregator does **not** make a lane blocking. `summary` is declared `if: always()` and asserts nothing about `needs.*.result`, so a new lane gates merges only once its own context is added to the required list above
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
`Status` single-select field with options **Todo**, **In Progress**, **Done**.

Project workflows keep membership and status; there is no manual step. Linking a story
under its epic with `addSubIssue` puts it on the board at `Todo`, opening a pull request
that links the issue moves it to `In Progress`, and closing it sets `Done`. The workflows
add nothing that has no parent, so a chore or docs issue raised without an epic stays off
the board; a few early items predate them.

Confirm any of that by reading the board rather than the workflow list — an automation
being enabled says nothing about which field it writes, and the API exposes each
workflow's name and enabled flag but not its action. Pass
`archivedStates: [ARCHIVED, NOT_ARCHIVED]` when you read: `projectV2.items` omits
archived items by default, so a board read without it is a partial one.

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

### Repairing board state by hand

Nothing routine needs this — the workflows above place items and set status. It is here
for the case where one has not fired, or a status is wrong and needs correcting.

```bash
# Project and Status field IDs (stable for this repo):
#   projectId   = PVT_kwHOAPhEnM4BTETq
#   statusField = PVTSSF_lAHOAPhEnM4BTETqzhAat7w
#   options     = Todo:f75ad846  In Progress:47fc9ee4  Done:98236657

# Read the board. `issue.projectItems` returns 0 here -- the project is user-owned and
# the repository org-owned -- so enumerate the project's items and paginate past 100.
# archivedStates is required: without it the archived items are silently missing.
gh api graphql --paginate -f query='
query($endCursor: String) {
  user(login: "DavidCozens") {
    projectV2(number: 1) {
      items(first: 100, after: $endCursor, archivedStates: [ARCHIVED, NOT_ARCHIVED]) {
        pageInfo { hasNextPage endCursor }
        nodes {
          id
          content { ... on Issue { number state } }
          fieldValueByName(name: "Status") {
            ... on ProjectV2ItemFieldSingleSelectValue { name }
          }
        }
      }
    }
  }
}'

# Correct a status, using the item id the query above returns.
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

### Not every PR needs an issue

Chores and documentation can go straight to a branch and PR with no issue behind them.
That is the normal way to do small, self-evident work — the edits to this file are an
example — and the PR body is the record.

Stories are unchanged: work that needs acceptance criteria gets a story, and a story sits
under an epic. Raise an issue only when the work needs to be remembered rather than done
now — it is deferred, it is committed to a release, or it needs agreement before it
starts. A chore or docs issue raised that way needs no epic.

### New-story checklist

For every new story:

1. `gh issue create --title "S<EE>.<NN>: …" --label story` — creates the issue.
   Pad both the epic and story numbers to two digits.
2. `addSubIssue` it under the parent epic (see **Issue / Epic Linking** above). The
   Parent-issue link is what groups the story into the correct swimlane.

There is no third step. Step 2 puts the story on the board at `Todo`, and the swimlane
appears with it. Do **not** add the parent epic — it is not an item.

### Work-in-progress limit

`In Progress` holds at most two items — typically one functional story and one BDD
story. A linked pull request is what moves a story there, so this is in practice a limit
on open PRs: **check the count before opening one that would make it three, and say so.**
The limit is only worth having if someone is watching it, and the board does not enforce
it.

### When closing a story

A merged PR that closes the story sets `Done` for you.

Epics don't have a Status field on the board (they aren't items). When every child
story is Done, the swimlane naturally becomes all-Done; the epic issue itself should
also be closed on GitHub. Check `subIssuesSummary.percentCompleted` on the epic if you
need a numeric roll-up.

---

## Milestones

Milestones carry the release axis. Epics answer *what kind of work is this*, the
board answers *what is being worked on now*, and the milestone answers *which
release does this ship in*.

### Which issues get one

**Assign the milestone to the finest-grained committed unit that exists.**

- An epic with no stories yet carries the milestone itself. Stories are only
  written when work is prioritised, so for a committed but unelaborated epic the
  epic is the only thing that can represent the commitment.
- Once the epic is elaborated, its stories take the milestone and it comes off the
  epic, so nothing is counted twice.

This mirrors the board, where the swimlane appears when the first story does.

An issue with no milestone is uncommitted. `no:milestone` is the backlog view;
there is deliberately no `Future` or `Backlog` milestone, because that would be a
second way to say the same thing. Epics raised to gauge interest — the platform
epics E34–E37 — stay unmilestoned permanently, since "no commitment" is what makes
their 👍 vote signal meaningful.

### Closed work is backfilled

The progress bar is read as *how far through the release are we*, not *how much
raw work is left*, so completed work is assigned to the release it shipped in.
265 completed stories and chores were backfilled into `0.1.0` on 2026-08-04. Do
not strip them out.

Excluded from backfill:

- Issues closed as `NOT_PLANNED` — they were not delivered.
- Epics whose stories already carry the milestone — that would double count.

`CHANGELOG.md`, generated by release-please from Conventional Commits, remains the
authoritative record of what shipped. The milestone is a progress view, not a
competing record.

### Due dates

Set a due date only where an external commitment exists, and set it to the
commitment rather than the internal estimate. `0.1.0` is due 2026-08-31 and
`1.0.0` 2026-12-31 because both dates are stated publicly on the website;
shipping earlier is the intent. Nothing goes red for a date we set for ourselves.

### What each release milestone means

`0.1.0` is the first public release: production quality and thoroughly tested,
on a limited set of platforms. Beta describes the breadth of platform coverage
and the absence of field integration so far, not the maturity of the code. The
public API is believed stable, and the reservation to change it is exactly that —
a reservation, exercised only if integration feedback shows a problem. Do not
write it up as though instability were expected.

`1.0.0` is the commitment that the public API is stable; after it, a breaking
change costs a major version. It is not a feature bucket: platform support and
feedback-driven work belong to the 0.x releases in between, each milestoned as it
is planned. `1.0.0` admits only what must be settled before the commitment
itself — API review, and any breaking change beta feedback shows is needed.

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

### BDD is acceptance, not a trailing story

Where an epic adds a feature or a platform, its BDD scenarios are how that epic is
accepted — they are not a separate story bolted on at the end. Plan the scenario with the
work, and expect it to pass before the work reaches `main`. Unit tests prove the code does
what we said; the BDD scenario proves the library still does what a user needs.

Documentation, website and release-process work has no BDD dimension — CI green is the
bar there. That is the current phase, so the rule above is dormant rather than repealed;
it applies again the moment a feature or platform epic starts.

Merging is the maintainer's decision, not a state a check can declare.

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
Tests/Support/      — Shared test-support library: fakes for the platform and vendor seams (socket, clock, mq, config lock, error handler, OpenSSL, Mbed TLS, Winsock, and the FreeRtosFakes / LwipFakes / FatFsFakes / PlusFatFakes subtrees), plus SafeString and the syslog field parser. Linked by test executables only, never by the production library.
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

Public headers are split by audience — each user includes only what they need. Code that
only logs events never sees allocators, senders, buffers, or config structs; the setup
code that wires an instance does, and includes only the headers for the pieces it wires.
Core headers live under `Core/Interface/`; each platform pack puts its own under
`Platform/<Pack>/Interface/`.

**Each header's `@file` brief is the authoritative description of what it provides and
why.** Read the header rather than a copy of it. For the wider map:

- `docs/roles/index.md` — the roles, each with its vtable contract and the backends
  that realise it.
- `docs/platforms/<slug>/index.md` — what each platform supplies.
- `docs/api-reference/` plus the generated Doxygen indexes (`docs/api/files.md`,
  `annotated.md`, `functions.md`, `macros.md`) — every header and symbol.

What application and setup code actually includes:

| Header | Audience |
|---|---|
| `SolidSyslog.h` | Application code that logs events — `SolidSyslog_Log`, `SolidSyslog_LogWithSd`, `SolidSyslog_Service`. Most application code needs nothing else |
| `SolidSyslogConfig.h` | System setup — `SolidSyslogConfig`, `SolidSyslog_Create`, `SolidSyslog_Destroy` |
| `SolidSyslogPrival.h` | Facility and severity enums |
| `SolidSyslogTimestamp.h` | Timestamp struct and the clock callback |
| `SolidSyslogEndpoint.h`, `SolidSyslogEndpointHost.h` | Setup code supplying the destination host and port |
| `SolidSyslogTransport.h` | Transport selection and the default port constants |
| `SolidSyslogError.h`, `SolidSyslogErrorCategory.h` | Installing an error handler; the portable category axis |
| `SolidSyslogConfigLock.h` | Setup on targets where pool slot walks can race across tasks or cores |
| `SolidSyslogSdElement.h`, `SolidSyslogSdValue.h` | Authors of custom structured data |
| `SolidSyslogHeaderField.h`, `SolidSyslogHeaderFieldFunction.h` | Callbacks writing HOSTNAME / APP-NAME / PROCID |

Everything else follows one of four naming patterns, so it can be found rather than listed:

| Pattern | Holds |
|---|---|
| `SolidSyslog<Role>.h` | The role's public call surface, e.g. `SolidSyslogSender_Send` |
| `SolidSyslog<Role>Definition.h` | The vtable an implementor fills — the extension point |
| `SolidSyslogNull<Role>.h` | The role's Null object, returned on bad config and on pool exhaustion |
| `SolidSyslog<Class>Errors.h` | `enum SolidSyslog<Class>Errors` plus `extern const struct SolidSyslogErrorSource <Class>ErrorSource`, for handlers matching on source identity |

Two invariants hold across every role and are worth knowing without looking anything up:
every role has a Null fallback, so an unfilled slot degrades safely rather than dangling at
link time; and every stateful Created class lives in a static pool whose exhaustion resolves
to that Null sibling — see **Pool Allocation (E11)** below.

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

### Characters in source

Write what a UK keyboard types. In `.c`, `.h` and `.cpp` — comments included — that
means `-` and not an em or en dash, `...` and not an ellipsis, `->` and not an arrow,
and the ASCII spelling of a symbol: `<=`, `+/-`, `x`, `||`, "sum of". A continuation
ellipsis takes no leading comma: `first, second...`, not `first, second, ...`.

Three exceptions stand, each authorised rather than assumed:

- `µ` in a unit, and `§` in an RFC citation. Both read better as themselves and
  neither is ambiguous.
- A character that *is* the subject of its comment. The UTF-8 tests name U+00E9, the
  euro sign and an emoji to explain the byte sequences they encode; replacing them
  would delete the point.
- String literals were left alone in the sweep that established this. Seven carry an
  em dash and are runtime or test output, where the character is data rather than
  typography.

**Anything else non-ASCII: ask.** Do not quietly pick a typographic character, and do
not quietly rewrite a sentence to avoid one — either way the decision goes unrecorded.

This is a rule about source. Documentation under `docs/` keeps typographic characters,
on the reasoning that prose of that length is written with an authoring tool; `README.md`
is hyphenated by hand and is the deliberate exception.

### MISRA-load-bearing `.clang-format` settings

Two settings in `.clang-format` are not merely stylistic — they enforce MISRA C:2012 rules at
format-on-save:

- **`InsertBraces: true`** combined with `AllowShortIfStatementsOnASingleLine: Never`,
  `AllowShortLoopsOnASingleLine: false`, `AllowShortFunctionsOnASingleLine: None`, and
  `AllowShortBlocksOnASingleLine: Never` — formatter-side enforcement of **MISRA 15.6**,
  which is why every `if`, `else`, `for` and `while` body in this project is braced.
  clang-format rewrites your code to add the braces if they are missing, and the
  `AllowShort*` settings stop them being collapsed back onto a single line.
- **`RemoveParentheses: Leave`** — keeps the project **MISRA 12.1 safe**. The advisory rule
  prefers explicit precedence parentheses; flipping this to `MultipleParentheses` would let
  clang-format strip them.

Do not change either group of settings without understanding the MISRA consequence.
See `docs/misra-deviations.md` for the project's stance on MISRA conformance.

---

## Documentation

Three rules, and they matter more than anything about wording. A wrong claim
costs one edit to fix; a wrong claim that has been copied costs an audit of
every page to find, and the copies rot silently because nothing checks them.

### Verify before asserting

Every statement about what the code does is read out of the code, not inferred
from a name, a neighbouring document, or something written earlier in the same
session. This is absolute for **failure modes**: before writing that something
fails, degrades, is silent, or is not reported, open the function and confirm
it. Claims that an error is *not* reported are the ones most often wrong, and
the most damaging, because they push an integrator into defending against a
problem that does not exist.

A document that is already in the repository is not evidence. It may be the
thing that is wrong.

### One claim, one place

Every fact has exactly one home. Everywhere else links to it, or omits it.

| The fact | Its home |
|---|---|
| What a config field or parameter means, including its edge values | the doc comment on that field |
| What a role's contract requires | that role's `SolidSyslog<Role>Definition.h` |
| What a platform ships, needs, guarantees, and leaves to the integrator | that platform's page under `docs/platforms/<slug>/` |
| How to wire a platform, and what will catch you out | that platform's `setup.md` |
| What Core does | the Core headers; `docs/core/index.md` curates and links them |
| How to get it building | `docs/build-integration.md` |

Writing the same sentence on a second page is the signal that it belongs on
neither — find its home, put it there once, and link. Do not restate a fact to
make a page self-contained: self-contained pages are how a set of documents
drifts out of agreement with itself.

This applies with particular force to the compliance guides, which attract
detail they should not hold. `docs/iec62443.md` is a quick reference that
reassures a developer or a security officer that the library can meet their
needs. It is **not** an audit artefact, and it is not a place to gather role
behaviour, platform behaviour, or catalogues of failure modes.

### A platform page never describes another platform

Naming a second platform to contrast behaviour, or to say where a capability
comes from, couples the two: the eleventh platform then has to be added to ten
pages. State this platform's own behaviour completely, and point at the
capability matrix in `docs/platforms/index.md` for who fills what.

### Link the record, not the source

A documentation page does not send the reader into the source tree. Where a
symbol has a generated API page, link that; otherwise name the file in code font
and leave it there. The page's job is to say what the library does, not to show
where it is implemented.

Issues and pull requests are the opposite case, and are linked. They are the
tracking record, and a reader who has just been told that a platform diverges
from a contract wants to see whether that is still true.

The repository-root documents are outside this rule rather than an exception to
it. `README.md`, `SECURITY.md`, `SUPPORT.md` and `LICENSE.md` are read on GitHub
as well as published into the site by `hooks/root_pages.py`, so their links stay
repo-relative; `hooks/source_links.py` rewrites whatever escapes `docs/` to a
canonical URL at build time.

---

## Design Patterns

These patterns are re-affirmed each time we do a code-hygiene pass. New
code should follow them; reviewers should call out drift.

### Production code (Tier 1, `Core/Source/`)

- **Single return per function.** MISRA-leaning. If the natural shape has an
  early return, restructure with a result local and an `if` wrapper. See
  `SolidSyslogBlockSequence.c::ScanForExistingBlocks` for the pattern.
- **Intent-naming static-inline predicates.** When a composite condition is
  inlined into an `if` or a `return`, extract a `static inline bool IsXxx(...)`
  helper. The helper's *name* is the documentation. Examples:
  `BlockSequence_IsAboveThreshold`, `FileBlockDevice_IsHandleAlreadyOpenOnBlock`,
  `FileBlockDevice_IsValidBlockIndex`, `BlockSequence_BlockIsFull`,
  `BlockSequence_StoreIsFull`. The cost (one extra named function) is the
  benefit (the reader doesn't have to decode the boolean).
- **One thing at one level of abstraction.** Functions read top-down.
  `<Class>_Create` first, `<Class>_Destroy` second, public functions in API order, helpers
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
public `<Class>_Create` takes whatever that class needs — a config struct, a short argument list,
or `void` — and returns an opaque handle (a pointer into the pool); `<Class>_Destroy` takes the
handle and releases the slot. Pool semantics:

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
  `SolidSyslog_LockConfig` / `SolidSyslog_UnlockConfig` injection pair. Single-task targets get the
  no-op default. Cleanup runs inside the lock, so a multi-task target must install one that
  tolerates blocking and is not a SolidSyslog Mutex; `SolidSyslogConfigLock.h` states the contract.
- **Shared helper.** `Core/Source/SolidSyslogPoolAllocator.{h,c}` (TU-internal) owns the
  three-operation contract (`AcquireFirstFree`, `FreeIfInUse`, `IndexIsValid`) every
  pool class reuses. No class re-implements the slot walk.

**Caller-supplied storage** never holds an instance — every stateful class is a pool slot.
What a caller can still supply is payload memory. Publicly that is one case:
`SolidSyslogCircularBuffer_Create` takes the backing ring
(`SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(maxMessages)`) plus a mutex, both of which must
outlive the buffer, while the buffer's own instance stays in the pool. Library-internally
`SolidSyslogFormatter` is a transient stack-built builder whose payload size is per-call
(`SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(n)`); it lives in `Core/Source/`, so integrators never
see it. The Formatter shape is documented under deviation D.002 in `docs/misra-deviations.md`.

**Internal sub-components** of pool-allocated classes (e.g. `RecordStore` and
`BlockSequence` inside `BlockStore`) live in sibling pools sized off the parent's
tunable. Their types stay in `Core/Source/` and never appear in public headers, so
integrator and example code physically cannot reach them.

---

## Function Ordering

Within a source file, functions are ordered top-down so the reader sees the lifecycle and public API
first, then drills into helpers as they appear:

1. `<Class>_Create` function first.
2. `<Class>_Destroy` function second.
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

Images are referenced by digest (`@sha256:…`) in `.github/workflows/ci.yml`,
`.devcontainer/docker-compose.yml` and `ci/docker-compose.bdd.yml` alike. A
`sha-<short>` tag names the source commit that built the image; it is not a
content digest and the registry may repoint it, which is why the digest is what
resolves. The tag is kept alongside so the reference stays readable: on a
`container:` or Compose `image:` key as a trailing comment, and on a `docker run`
invocation in the comment above the step, because a shell line continuation must
end in `\` and cannot carry one.

Pin whatever `docker buildx imagetools inspect` reports — the index digest for a
multi-architecture image, the manifest digest for a single-platform one. Never a
per-platform digest dug out of `--raw`, which would nail a multi-architecture
image to one architecture.

When updating an image:

1. Build and push the new image in the container image repo
2. Resolve the new tag to its digest — `docker buildx imagetools inspect <repo>:<tag> --format '{{.Manifest.Digest}}'`
3. Update the digest and the accompanying tag comment in all files that reference it (see [`docs/containers.md`](docs/containers.md) for the full list)
4. Rebuild the devcontainer and verify the new tooling works locally
5. Then commit — use `chore: bump container image to <sha>`

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
