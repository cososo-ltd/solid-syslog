# SKILL.md — Claude Code Brief for SolidSyslog

## What this project is

SolidSyslog is a C syslog client library implementing RFC 5424/5426/6587/5425 for
embedded and industrial systems. It is developed under the "Crafted with AI" blog
series by Cozens Software Solutions Limited.

## Collaboration architecture

- **Claude.ai** — planning, backlog decomposition, blog drafting, architecture decisions.
  Produces briefings for Claude Code to execute.
- **Claude Code** — implementation, commits, GitHub operations, DEVLOG.md maintenance.
  Receives briefings from the developer and executes them.

When in doubt about intent or architecture, ask the developer rather than assuming.

## DEVLOG.md convention

Append an entry to DEVLOG.md after every meaningful session. Format:
```
## YYYY-MM-DD — <short session title>

### Decisions
- <decision and rationale>

### Deferred
- <item deferred and why>

### Open questions
- <question> — <context>
```

Never rewrite history. Always append. Commit DEVLOG.md changes with:
`docs: update DEVLOG for <session topic>`

## TDD pairing contract

- Discuss behaviour first — no code without agreement on what it should do
- Write one test at a time
- Confirm the failure reason before writing production code
- Write the minimal implementation to make it pass
- Commit on green with a behaviour-describing Conventional Commit message
- Refactor only under green

Test progression follows ZOMBIES order.

## Branch and PR rules

- Feature branches: `<type>/<short-description>` (e.g. `feat/prival-encoding`, `ci/pin-action-shas`)
- PRs to main when a BDD scenario passes (or for infrastructure work, when CI is green)
- Squash merge only — PR title becomes the commit message
- PR title must follow Conventional Commits

## Code style

- Formatting is enforced by clang-format — see `.clang-format`. This is the authoritative
  style rule; it overrides any conflicting guidance in this file or from Claude.ai briefings.
- Public C functions: `PascalCase_PascalCase` (e.g. `SolidSyslog_Create`)
- Variables/parameters: `camelCase`
- Types and files: `PascalCase`
- Follows James Grenning's style (*TDD for Embedded C*) where consistent with clang-format
- No dynamic memory allocation required — allocator is caller-injected. No unions, no anonymous structs, no `#ifdef` feature flags
- C99 baseline

## Architecture principles

- OO-in-C: structs with function pointers (vtable pattern)
- Dependency injection for transport, buffering, clock, hostname, allocator
- Buffer abstraction decouples formatting from sending: `SolidSyslog_Log` writes to buffer,
  `SolidSyslog_Service` reads from buffer and sends. Implementations: NullBuffer (direct send,
  single-task), CircularBuffer (portable ring with caller-allocated storage and an injected
  `SolidSyslogMutex` — Posix/Windows/Null mutex shipped, integrators can plug their own RTOS
  primitive), PosixMessageQueueBuffer (POSIX message queue, used by the Linux Threaded example)
- Null object pattern throughout (NullBuffer is the buffer null object)
- All fields use uniform field object pattern with format function pointer
- Optional features composed at link time — no conditional compilation
- C11 static assertions via compatibility shim

## Static analysis

- cppcheck runs in CI — MISRA C:2012 addon is a future addition
- Suppressions in `misra_suppressions.txt` — each entry must have documented rationale
- Library is MISRA-informed, not claiming certified compliance

## GitHub Project board conventions

- Only the active epic and its stories live on the board — future epics stay as issues until decomposition begins
- WIP limit on In Progress is 2 (one functional story, one BDD story)
- When an epic is complete: archive its stories, leave the epic in Done
- When a new epic is pulled onto the board: archive any completed epics first

## Key references

- Epics tracked as GitHub Issues, Project board "SolidSyslog" (project #1); active and future epics are labelled `epic` and numbered beyond the original #2–#12 range (e.g. E12 #31, E14 #64, E17 #105, E18 #112)
- RFC 5424 — structured syslog message format
- RFC 5426 — syslog over UDP
- RFC 6587 — syslog over TCP (octet counting)
- RFC 5425 — syslog over TLS
