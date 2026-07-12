# Vulnerability Triage Runbook

This is the maintainer's operational counterpart to [`SECURITY.md`](../../SECURITY.md):
how a vulnerability report is handled end-to-end, from receipt to retrospective.
`SECURITY.md` states the public promises (72-hour acknowledgement, 90+14
disclosure); this runbook is how they are met.

The single tracking record for any vulnerability is its draft GitHub Security
Advisory (GHSA). No separate issue, spreadsheet, or external tracker: the draft
GHSA holds the timeline, severity, affected/fixed versions, and reporter details
from receipt through publication.

Reports arrive via the channels in `SECURITY.md`: GitHub private vulnerability
reporting (which opens a draft GHSA directly) or the `cososo.co.uk/security/report`
web form.

## Stage 1 — Receipt (hours 0–72)

- [ ] Log the report by opening (or confirming) a draft GHSA on the repo.
- [ ] If it came via the web form, an auto-acknowledgement has already gone out;
      send a human acknowledgement within 72 hours regardless.
- [ ] Capture the reporter's contact and their credit preference (name/handle,
      or anonymous) in the advisory.
- [ ] Do not confirm or deny severity yet; that's triage.

## Stage 2 — Triage (days 0–7)

- [ ] Scope the issue against the support tiers (see `SECURITY.md`):
  - **Tier 1 `Core/`**: full treatment (CVE, advisory, fix, signed release).
  - **Tier 2 `Platform/`** / **Tier 3 `Bdd/Targets/`**: advisory only, no CVE
    unless the root cause reaches into `Core/`.
  - **Out of repo** (integrator code, a linked TLS/crypto library, the OS):
    redirect; see *Non-standard reports* below.
  - **Not a supported product** (test code, build/CI infrastructure,
    documentation): out of scope; no CVE or release treatment, matching
    `SECURITY.md`.
- [ ] Reproduce where feasible; record the reproduction in the advisory.
- [ ] Only once the issue is confirmed in-scope and reproduced, assign a
      CVSS v3.1 vector and score and derive the qualitative band; this
      avoids scoring reports that turn out out-of-scope, upstream, or intended.
- [ ] For a confirmed `Core/` vulnerability, request a CVE via GitHub's
      CNA from the advisory.
- [ ] Decide the fix workflow by severity:
  - **High / Critical** → private: develop the fix in the GHSA's private fork,
    publish the advisory coordinated with the release.
  - **Low / Medium** → open: fix in a normal PR; the advisory publishes when the
    release ships.
- [ ] Update the reporter with the triage outcome and expected next step.

## Stage 3 — Fix development

- [ ] Regression test first: a failing test that captures the vulnerability,
      per the project's TDD discipline. This becomes the permanent guard.
- [ ] Implement the minimal fix to pass it.
- [ ] Use a Conventional Commit (`fix:` …) so release-please picks it up and
      the CHANGELOG entry is generated.
- [ ] High/Critical: keep this on the private fork until release is coordinated.

## Stage 4 — Release coordination

See [`release-process.md`](../release-process.md) for the mechanics; the
security-specific steps are:

- [ ] Merge the fix to `main` (from the private fork for High/Critical).
- [ ] Merge release-please's release PR to cut the tagged release.
- [ ] Verify the release carries all four provenance assets: the SBOM, the
      source-tree hash, and their two signatures. `sbom.yml` (triggered by the
      `release.published` event) attempts to attach them, but attachment is
      advisory (`continue-on-error`), so confirm all four are present before
      relying on provenance.
- [ ] Record and verify the affected and fixed version ranges in the advisory;
      never publish without a safe version for users to move to.
- [ ] Publish the GHSA coordinated with the release going live.
- [ ] Edit the release notes to reference the GHSA / CVE.

## Stage 5 — Post-release

- [ ] Notify the reporter that the fix has shipped.
- [ ] Credit the reporter per the consent captured at intake.
- [ ] Confirm the advisory is published and mark the workflow complete. (A
      published advisory stays in the repo; "close" applies only to draft or
      not-a-vulnerability reports.)

## Stage 6 — Retrospective

A brief post-mortem, captured as a comment on the advisory:

- [ ] How did the bug get in? Did existing tests miss it, and why?
- [ ] Are there similar-class issues elsewhere in the code to sweep proactively?
- [ ] Any process gap this response exposed?

## Reporter communication

Update the reporter at each stage transition: receipt, triage outcome, fix
in progress, release shipped. Silence is the most common complaint in coordinated
disclosure; a short "still on it" beats nothing.

## Reporter data

Reporter contact details are used solely to coordinate the disclosure and are
handled under the [COSOSO privacy policy](https://www.cososo.co.uk/privacy-policy/).
They are visible only to the maintainer via the advisory, are never published
in advisory text or credits without the reporter's explicit consent (captured at
intake), and are not retained beyond what the coordination requires.

## Non-standard reports

- **Out of scope / not a vulnerability / intended behaviour**: explain the
  reasoning, point to the relevant docs (e.g. the threat model's *caller
  obligations*), and thank the reporter. Close the advisory as not-applicable.
- **Root cause in an upstream dependency** (a linked TLS/crypto library):
  redirect the reporter to that project's disclosure process; SolidSyslog bundles
  none of them. Track only if `Core/` needs a compensating change.
- **Self-reported** (you find it yourself): same flow, no external reporter; skip
  the acknowledgement/credit steps.

## Maintainer unavailability

The public timelines are best-effort (see `SECURITY.md`'s force-majeure clause).
When unavailable:

- **Short-term** (days): covered by the stated SLAs plus the force-majeure clause.
- **Medium-term** (illness, travel): post a brief holding note (repo README or
  `cososo.co.uk`) and tell any active reporters that timelines are paused under
  the force-majeure clause. As a solo maintainer there is no standing cover; if a
  co-maintainer exists, hand the active advisories and repository access to them.
- **Long-term** (unable to continue): triggers the continuity commitment: the
  current release is relicensed under a permissive OSS licence so users aren't
  stranded (see `SECURITY.md`).

## Evidence retention

GitHub is the primary record of the diligence trail: published GHSAs, release
assets, SBOMs, signatures, and commit history. GitHub retention is not an
immutable guarantee, though: advisories can be deleted via GitHub Support,
release assets removed with write access, and history rewritten. To rely on it
as the CRA evidence store: protect the release tags and `main`, restrict who can
delete releases and advisories, and take a periodic immutable export (or
off-platform archive) of the advisories, SBOMs, and signatures.
