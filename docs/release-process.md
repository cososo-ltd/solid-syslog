# Release Process

How a SolidSyslog release is cut, for the maintainer. Releases are source-only
(no binary artefacts) with a signed SBOM and a reproducible source-tree hash
attached so integrators can verify provenance (per
[`security/release-verification.md`](security/release-verification.md) and
[`security/sbom.md`](security/sbom.md)). *Security* releases run in lockstep with
[`security/triage-runbook.md`](security/triage-runbook.md).

## Versioning

- Semantic Versioning. Pre-1.0 (`0.x`), breaking changes bump the minor,
  not the major (`bump-minor-pre-major`), no premature 1.0 signal while the API
  is still settling.
- release-please derives the version from the Conventional Commit types merged
  since the last release.

## What the release notes contain

Every release carries three parts, in this order. The first two are written; the
third is generated.

1. **What's in this release** — prose, plus a compliance and platform snapshot.
   What the release is for, and what changed that an integrator would act on.

   This is also where a documentation change that corrected a claim an integrator
   could have acted on gets surfaced, since `docs` commits do not appear in the
   generated list.

   **The snapshot is restated here rather than linked**, which is deliberate and
   is not the duplication the documentation rules forbid. Those rules guard
   against two *current-state* copies drifting apart. A release note is not a
   copy — it is a frozen record with a different lifetime. The
   [compliance matrix](rfc-compliance.md) and the
   [platform matrix](platforms/index.md) answer *where are we now*; the release
   note answers *where were we at this version*, which nothing else records. The
   site publishes from `main` only, so a link out of an old release resolves to
   today's state and silently loses what that release actually shipped.

   Where the full detail is wanted, link the **tag** rather than the branch —
   `blob/vX.Y.Z/docs/rfc-compliance.md` is frozen by git even though the site is
   not.

   What to include:

   - **RFC compliance.** State in prose what moved — a clause newly met, a
     standard newly covered. Restate the summary table whenever a number changed;
     one line saying nothing changed when none did. Read the numbers off the
     matrix at the tag, not from memory.
   - **Platforms.** The full list at the first release; afterwards name what was
     added and say the rest are unchanged.

   Restating absolute numbers rather than only deltas is what keeps the series
   answerable: deltas compose badly, and one wrong delta propagates through every
   later release with nothing to correct it.
2. **Known limitations** — the defects and divergences shipping with the release,
   each linking its tracking issue. State that they were found by audit and are
   disclosed on the pages that describe the affected platform: a bare list of open
   defects reads as unfinished work, and the same facts framed as deliberate
   disclosure read as rigour.
3. **Full changelog** — release-please's generated list.

The GitHub Release description and the `CHANGELOG.md` entry carry the same text.
Write it once to a file and use it for both.

## What appears in the generated changelog

`feat` and `fix` only. `refactor`, `ci`, `chore` and `docs` are configured
`hidden` in `release-please-config.json`, because none of them changes what a
consumer of the library gets — refactoring is defined as preserving behaviour, and
the other three never reach the consumer at all.

Breaking changes surface in their own section regardless of the type that carried
them.

The Conventional Commit type is therefore a statement about consumer impact, not
only about which files were touched. A change that alters what an integrator
should do belongs under a type that appears.

## Cutting a release

1. Conventional Commits land on `main`, each mapping to a CHANGELOG section.
2. release-please maintains a release PR that bumps the version and
   `CHANGELOG.md`. Write the first two parts of the release notes into that PR
   before merging it.
3. Merging that PR creates the tag and GitHub Release (release-please's
   bot, no personal GPG/SSH signing).
4. The `release: published` event triggers `sbom.yml`: it renders and validates
   the CycloneDX SBOM, writes the content-tree SHA-256 (scope: `Core/` +
   `Platform/` + `CMakeLists.txt`, `CMakePresets.json`, `LICENSE.md`,
   `LICENSES/`), cosign keyless-signs both (GitHub OIDC), and attaches the
   four assets to the Release.
5. Signing and attachment hard-fail. The Release already exists by the time the
   job runs, so a failure cannot block it — it means the Release went out
   without provenance. A red run is the signal: fix the cause and re-run the
   job (see [release verification](security/release-verification.md)).

> Status: release-please is parked (manual `workflow_dispatch` only) until the
> 0.1.0 baseline; trigger a release from the Actions tab meanwhile. See
> `release-please.yml` for the reason and the unpark steps.

## Security releases

Coordinated with the disclosure; see the runbook's *Release coordination* stage:

- High / Critical: develop the fix on the advisory's temporary private fork;
  the draft GHSA stays the single tracking record, published coordinated with the
  release.
- Low / Medium: fix in the open; the advisory publishes when the release
  ships.
- Record affected and fixed version ranges in the advisory before publishing.

## Checklist

- [ ] `main` is green.
- [ ] Cut the release by merging the release PR.
- [ ] Confirm the tag + GitHub Release, and verify the attached SBOM, source
      hash, and both cosign signatures per
      [`security/release-verification.md`](security/release-verification.md), not
      just that the assets are present — a bundle that is present is not yet a
      bundle that verifies.
- [ ] Security release: publish the coordinated GHSA.
