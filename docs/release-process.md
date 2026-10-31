# Release process

How a SolidSyslog release is cut, for the maintainer. Releases are source-only
(no binary artefacts) with a signed SBOM, a reproducible source-tree hash and a
signed offline copy of the documentation attached so integrators can verify
provenance and keep the documentation for the version they pinned (per
[`security/release-verification.md`](security/release-verification.md) and
[`security/sbom.md`](security/sbom.md)). *Security* releases run in lockstep with
[`security/triage-runbook.md`](security/triage-runbook.md).

## Versioning

- Semantic Versioning. Pre-1.0 (`0.x`), a breaking change bumps the minor, not
  the major (`bump-minor-pre-major`). The 0.x label describes the breadth of
  platform coverage and the absence of field integration so far, not the maturity
  of the code; `1.0.0` is the commitment that the public API is stable, after
  which a breaking change costs a major version.
- release-please derives the version from the Conventional Commit types merged
  since the last release.

## What the release notes contain

A release's notes open with a sentence or two on what the release is for, then
carry five written sections and the generated list, in this order. The written
sections are added by hand in the release pull request.

1. **What's in this release** - what changed that an integrator would act on, in
   prose. Then the platforms: name what was added, and give the full list.
   "Unchanged" is rarely true of the rest, since most releases touch them.
2. **Before you upgrade** - every change that stops code written for the previous
   release building, or changes what it does without a build error: a signature,
   a config field, a vtable slot an adapter fills, a default. Say for each what to
   change and, where there is one, how to keep the old behaviour. Take the list
   from `git diff v<previous>..HEAD -- Core/Interface Platform/*/Interface` and the
   merged stories, not from the generated list. When nothing breaks, say so in one
   line.

   It is written by hand because the generated list names each break by its
   commit subject and says nothing about what to change. A commit that breaks
   the API carries `!` or a `BREAKING CHANGE` trailer, so release-please lists it
   in a breaking-changes section; with `bump-minor-pre-major` a break before 1.0
   bumps the minor version, not the major. A break that went in unmarked appears
   in no generated section, so take the list from the interface diff rather than
   from the generated one.

   It also lists the documentation corrected in the release, where acting on the
   old text would have cost an integrator something, since `docs` commits do not
   appear in the generated list.
3. **RFC compliance at this release** - state in prose what moved - a clause
   newly met, a standard newly covered. Restate the summary table whenever a
   number changed; one line saying nothing changed when none did. Read the
   numbers off the matrix at the tag, not from memory.

   **This table and the platform list are restated rather than linked**, which is deliberate and
   is not the duplication the documentation rules forbid. Those rules guard
   against two *current-state* copies drifting apart. A release note is not a
   copy - it is a frozen record with a different lifetime. The
   [compliance matrix](rfc-compliance.md) and the
   [capability matrix](platforms/index.md) answer *where are we now*; the release
   note answers *where were we at this version*, which nothing else records. The
   site publishes from `main` only, so a link out of an old release resolves to
   today's state and silently loses what that release actually shipped.

   Where the full detail is wanted, link the **tag** rather than the branch -
   `blob/vX.Y.Z/docs/rfc-compliance.md` is frozen by git even though the site is
   not.

   Restating absolute numbers rather than only deltas is what keeps the series
   answerable: deltas compose badly, and one wrong delta propagates through every
   later release with nothing to correct it.

   **Carry the qualification with the numbers.** The matrix states that a status
   describes the library with a conforming platform supplying the roles it needs,
   that almost every requirement depends on which platform components are selected
   and how they are configured, and that this includes components the integrator
   writes, which the library cannot speak for. A table lifted out of that preamble
   claims more than the matrix does - and unlike the matrix, a release note is
   frozen and cannot be corrected later. So say in the release note that the
   figures are the maintainer's assessment of the library against the RFCs, that
   they depend on how the integrator configures it, and that they are neither a
   certification nor a conformance claim. One sentence is enough; omitting it is
   not.
4. **Known limitations** - the defects tracked at the release, each linking its
   tracking issue. State that they were found by audit and are disclosed on the
   pages that describe the affected platform: a bare list of open defects reads
   as unfinished work, and the same facts framed as deliberate disclosure read as
   rigour. A divergence a platform accepts by design is not a limitation of the
   release: it is stated on that platform's page, and the note points there in
   one sentence rather than listing it.
5. **Verifying this release** - the attached assets, how they are signed, and a
   link to the verification guide at the tag. The check that matters is that a
   bundle verifies, not that the assets are present.
6. **Full changelog** - release-please's generated list.

### Getting the written parts into both places

The `CHANGELOG.md` entry and the GitHub Release description carry the same parts,
and nothing copies one to the other.

1. Write the written sections and paste them above the generated list in
   `CHANGELOG.md`, in the release pull request, before merging it.
2. After the Release exists, build a file holding **every part** - the written
   sections and the generated list, which the merged `CHANGELOG.md` entry now
   contains - and set the description from it:

   ```bash
   gh release edit v<version> --notes-file <file>
   ```

`--notes-file` **replaces** the Release description rather than adding to it, so a
file carrying only the written sections would delete the generated changelog from the
Release. Take the generated section from the merged `CHANGELOG.md` entry, or from
the Release's own body with `gh release view --json body`, before editing.

Do not assume the Release description picks up a hand-edited `CHANGELOG.md`.
Setting it explicitly is correct whether it would or not, and editing a Release
fires `release: edited` rather than `release: published`, so neither `sbom.yml` nor
`docs-bundle.yml` re-runs, and the signed assets are undisturbed.

## What appears in the generated changelog

`feat` and `fix` only. `refactor`, `ci`, `chore` and `docs` are configured
`hidden` in `release-please-config.json`, because none of them changes what a
consumer of the library gets - refactoring is defined as preserving behaviour, and
the other three never reach the consumer at all.

A commit marked as breaking surfaces in a breaking-changes section whatever its
type, and in its type's own section even where that type is hidden, which is where
0.2.0's Refactoring section came from. Mark every break, whatever its type;
*Before you upgrade* then describes each one.

The Conventional Commit type is therefore a statement about consumer impact, not
only about which files were touched. A change that alters what an integrator
should do belongs under a type that appears.

## Cutting a release

`CHANGELOG.md` is the authoritative record of what changed between releases. Its
generated section is built by release-please from the Conventional Commit types
listed above; the written sections are added by hand in the release pull request.

1. Conventional Commits land on `main`. `feat` and `fix` map to a generated
   CHANGELOG section; the hidden types above produce no entry, and a commit marked
   as breaking is listed whatever its type.
2. release-please maintains a release PR that bumps the version and
   `CHANGELOG.md`. Write the written sections of the release notes into that PR
   before merging it, per *Getting the written parts into both places* above.
3. Run each target that a target checklist accepts against the release pull
   request's head before merging it: those whose `Bdd/Targets/<Target>/README.md`
   has a *Target checklist* section. CI cannot reach them, and
   [CLAUDE.md](../CLAUDE.md#a-target-ci-cannot-reach-is-accepted-by-a-checklist)
   states the rule. Record the run as a comment on the pull request - the commit,
   each step as pass, fail or not run, and the scenarios passed, failed and
   skipped - since release-please rewrites the pull request's body.
4. Merging that PR creates the tag and a **draft** GitHub Release
   (made with the release-please GitHub App's token, no personal GPG/SSH signing). The tag is created at
   once; a draft would otherwise get its tag only on publication, and
   release-please could not then find the previous release.
5. Publishing the draft is a separate step. Nothing the release attaches exists
   until then, so publish once everything the documentation links by name is in
   place - the example stacks' `release/<version>` branches, which pin the
   tag's SHA and so can only be cut once the tag exists. Publish with
   `gh release edit v<version> --draft=false --latest`, so the release is the
   one GitHub serves as latest; a fix to an older line takes `--latest=false`
   instead.
6. The `release: published` event triggers `sbom.yml`: it renders and validates
   the CycloneDX SBOM, writes the content-tree SHA-256 (scope: `Core/` +
   `Platform/` + `CMakeLists.txt`, `CMakePresets.json`, `LICENSE.md`,
   `LICENSES/`), cosign keyless-signs both (GitHub OIDC), and attaches each
   file with its signature to the Release.
7. The same event triggers `docs-bundle.yml`: it builds the documentation from
   the tag with `mkdocs-offline.yml`, checks that the result works with no
   server and no network, cosign-signs the zip and attaches it with its
   signature. The bundle is built and checked on every CI run as well, so only
   the signing and the attachment wait for a release.
8. Signing and attachment hard-fail. The Release already exists by the time the
   jobs run, so a failure cannot block it - it means the Release went out
   without provenance. A red run is the signal: fix the cause and re-run the
   job (see [release verification](security/release-verification.md)).

### Why the documentation ships as a file

A published copy of the site for every version would answer the same question,
and was the original plan. It was dropped: indexed version directories make
superseded API pages findable by search, the site grows with every release, and
published URLs cannot be withdrawn once linked. A fixed, hashed, signed file is
also closer to what an archival reader wants - it can be kept and verified later,
which a URL cannot.

The reader-facing consequence is that the bundle's footer names the release and
nothing else. The commit, the tag, the build date and the toolchain digest are in
`MANIFEST.txt` at the root of the bundle, where an auditor looks for them.

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
- [ ] Bring the documentation to the release before it is tagged, since the bundle
      is built from the tag: the example stacks' `release/<version>` links in
      `README.md`, `docs/README.md`, `docs/build-integration.md` and
      `docs/hardening-path.md`, the `GIT_TAG` in `docs/build-integration.md`, and
      the four figures under *What it costs* in `README.md`, taken from the
      re-measured hardening path.
- [ ] The release PR's notes carry the written sections, *Before you upgrade*
      included.
- [ ] Run each target that has a target checklist against the release PR's head,
      and record the run as a comment on the PR, per
      [Cutting a release](#cutting-a-release).
- [ ] Cut the release by merging the release PR.
- [ ] Confirm the tag and the draft GitHub Release, and read the notes in the
      draft before anything goes out.
- [ ] Set the draft's notes explicitly with `gh release edit`, per
      [Getting the written parts into both places](#getting-the-written-parts-into-both-places).
- [ ] Cut the example stacks' `release/<version>` branches pinned to the tag's
      SHA, then publish the draft.
- [ ] Confirm GitHub marks the new release as Latest:
      `gh api repos/cososo-ltd/solid-syslog/releases/latest --jq .tag_name`.
- [ ] Confirm the attachments, and verify all six assets - the SBOM, source
      hash and documentation bundle, and their three cosign signatures - per
      [`security/release-verification.md`](security/release-verification.md), not
      just that the assets are present - a bundle that is present is not yet a
      bundle that verifies. Install the tool versions that page states rather
      than using whatever is already on your `$PATH`: verifying with your own
      toolchain proves the signature good but hides any drift between the guide
      and what the workflow actually produces, which is the failure an
      integrator meets first.
- [ ] Open the documentation bundle before trusting it. Download
      `solid-syslog-docs-<version>.zip`, extract it and open `index.html` with
      the network off. The automated check proves every link resolves inside the
      bundle and that nothing is fetched from another host; it cannot tell you
      the pages render acceptably, and nothing else in the pipeline looks at
      them. Confirm the footer names the release, and that the notice explaining
      how to restore search appears.
- [ ] Security release: publish the coordinated GHSA.
