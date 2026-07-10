# Release Process

How a SolidSyslog release is cut, for the maintainer. Releases are source-only —
no binary artefacts — with a signed SBOM and a reproducible source-tree hash
attached so integrators can verify provenance (per
[`security/release-verification.md`](security/release-verification.md) and
[`security/sbom.md`](security/sbom.md)). *Security* releases run in lockstep with
[`security/triage-runbook.md`](security/triage-runbook.md).

## Versioning

- **Semantic Versioning.** Pre-1.0 (`0.x`), breaking changes bump the **minor**,
  not the major (`bump-minor-pre-major`) — no premature 1.0 signal while the API
  is still settling.
- release-please derives the version from the Conventional Commit types merged
  since the last release.

## Cutting a release

1. Conventional Commits land on `main`, each mapping to a CHANGELOG section.
2. release-please maintains a **release PR** that bumps the version and
   `CHANGELOG.md`.
3. Merging that PR creates the **tag** and **GitHub Release** (release-please's
   bot — no personal GPG/SSH signing).
4. The `release: published` event triggers `sbom.yml`: it renders and validates
   the CycloneDX SBOM, writes the content-tree SHA-256 (scope: `Core/` +
   `Platform/` + `CMakeLists.txt`, `CMakePresets.json`, `LICENSE.md`), **cosign
   keyless-signs** both (GitHub OIDC), and attaches all four assets to the
   Release.
5. Signing is advisory (`continue-on-error`) on initial rollout, so it can't
   block a release.

> **Status:** release-please is parked (manual `workflow_dispatch` only) until the
> 0.1.0 baseline; trigger a release from the Actions tab meanwhile. See
> `release-please.yml` for the reason and the unpark steps.

## Security releases

Coordinated with the disclosure — see the runbook's *Release coordination* stage:

- **High / Critical** — fix on the GHSA private fork; publish the advisory
  coordinated with the release.
- **Low / Medium** — fix in the open; the advisory publishes when the release
  ships.
- Record affected and fixed version ranges in the advisory **before** publishing.

## Checklist

- [ ] `main` is green.
- [ ] Cut the release: merge the release PR, or trigger release-please from the
      Actions tab while parked.
- [ ] Confirm the tag + GitHub Release and the four attached SBOM/hash assets.
- [ ] Security release: publish the coordinated GHSA.
