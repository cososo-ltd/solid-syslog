# Contributing to SolidSyslog

Thank you for your interest in SolidSyslog. This document explains what kinds
of contribution the project accepts today, and how to get involved.

## What's always welcome

- **Bug reports** — use the issue templates.
- **Feature requests** — use the issue templates.
- **Questions and discussion** — open an issue; see [SUPPORT.md](SUPPORT.md).
- **Security reports** — follow [SECURITY.md](SECURITY.md) (please do **not**
  file security issues publicly).

Well-described issues are genuinely valuable and are the primary way to help
the project.

## Code contributions

SolidSyslog is **source-available but closed to unsolicited code
contributions.** The library is dual-licensed — [PolyForm Noncommercial
1.0.0](LICENSE.md) for the free tier, plus a separate commercial licence — and
selling a commercial licence requires that every line can be relicensed. That
means the copyright must stay clean, so external patches cannot simply be
merged. Pull requests are by invitation only.

### Core (`Core/`) — closed

Core is the licensed, dual-licensed heart of the library. It is not open to
external code, and unsolicited pull requests against `Core/` will be
respectfully declined.

### Platform adapters (`Platform/`) — get in touch

Platform adapters — the glue to a particular RTOS, TCP stack, TLS library, or
filesystem — are exactly the kind of code the community is best placed to
help with, and they are **expected to be modifiable** by integrators.

We intend to open Platform-tier contributions in future under a permissive
inbound licence (so contributed adapters can ship in both the free and
commercial builds without a contributor licence agreement). That path is not
open yet.

**In the meantime, if you would like to contribute a Platform adapter, please
[get in touch via the contact form](https://www.cososo.co.uk/#contact).** We'd
rather have the conversation than turn you away — tell us the target you're
porting to and we'll work out how to take it.

## For invited contributors

If you have been invited to contribute, the development workflow, coding
standards, and pre-PR check budget are documented in the repository rather than
duplicated here:

- [`CLAUDE.md`](CLAUDE.md) — git workflow, Conventional Commits, squash-merge,
  TDD discipline, CMake presets, project structure and support tiers.
- [`docs/local-checks.md`](docs/local-checks.md) — the tiered pre-PR check
  budget (per-commit builds, pre-push analysis, what CI owns).
- [`docs/NAMING.md`](docs/NAMING.md) — the naming scheme and its MISRA posture.

## Code of conduct

Participation in this project is governed by our
[Code of Conduct](CODE_OF_CONDUCT.md).

## Commercial licensing and support

For commercial licensing, guaranteed response times, or paid support, enquire
via the form at
[cososo.co.uk](https://www.cososo.co.uk/?service=solidsyslog#contact).
