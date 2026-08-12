# SKILL.md — Claude Code Brief for SolidSyslog

## What this project is

SolidSyslog is a C syslog client library implementing RFC 5424/5426/6587/5425 for
embedded and industrial systems. It is developed under the "Crafted with AI" blog
series by Cozens Software Solutions Limited.

## How this file relates to CLAUDE.md

This file holds **how we work together**. [`CLAUDE.md`](CLAUDE.md) holds **how the
repository works** — git and issue workflow, project structure, naming, code style,
design patterns, and the board and milestone conventions. Neither restates the other,
precedence included: what wins when two sources disagree is stated once, in CLAUDE.md
under **Precedence, when two sources disagree**.

## Collaboration modes

Two modes are both normal, and the developer sets which is in play:

- **Briefed** — Claude.ai has done the planning, backlog decomposition or architecture
  work and hands over a briefing to execute. Follow it; raise conflicts with the repo
  rather than silently reconciling them.
- **Direct** — the work is planned in the session itself: refining the backlog, drafting
  epics, designing an API, auditing the docs. Expect to propose, discuss, and wait for
  agreement before acting.

If it is unclear which mode applies, ask. When in doubt about intent or architecture, ask
the developer rather than assuming — that holds in both modes.

## TDD pairing contract

- Discuss behaviour first — no code without agreement on what it should do
- Write one test at a time
- Confirm the failure reason before writing production code
- Write the minimal implementation to make it pass
- Commit on green with a behaviour-describing Conventional Commit message
- Refactor only under green

Test progression follows ZOMBIES order. The discipline behind this — the three laws, the
test-defaults pattern, the coverage bar, and when BDD forms acceptance — is in CLAUDE.md
under **TDD Discipline**.

## Architecture in one paragraph

OO-in-C: structs of function pointers, one vtable per role, with dependency injection
throughout and a Null object for every role. No dynamic allocation — every stateful
Created class lives in a library-internal static pool. No unions, no anonymous structs,
no `#ifdef` feature flags; optional features are composed at link time. C99 baseline.
Follows James Grenning's style (*TDD for Embedded C*) where consistent with clang-format.
The detail is in CLAUDE.md under **Project Structure**, **Design Patterns** and
**Pool Allocation (E11)**.

## Key references

- `docs/NAMING.md` — the per-tier naming scheme. Read it before adding any public identifier.
- `docs/misra-deviations.md` — where every MISRA deviation is justified. The cppcheck
  MISRA C:2012 addon runs in CI and is enforcing; `misra_suppressions.txt` carries the
  line-specific suppressions and no prose, because the rationale lives in the deviations doc.
- `docs/local-checks.md` — the tiered local check budget before raising a PR.
- Epics and stories are GitHub Issues; project board "SolidSyslog" (project #1).
- RFC 5424 — structured syslog message format
- RFC 5426 — syslog over UDP
- RFC 6587 — syslog over TCP (octet counting)
- RFC 5425 — syslog over TLS
