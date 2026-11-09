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

### What is a breaking change

A breaking change is a change to a public header - one under `Core/Interface/`
or `Platform/*/Interface/` - or to the documented behaviour behind one, that
stops code written against the previous release building or changes what it
does. Only such a change carries `!` or a `BREAKING CHANGE` trailer, and only
such a change costs a major version after `1.0.0`. The platform packs' headers
count: an integrator cannot tell a Core header from a pack header, so both are
held to the same bar. Removing a pack is a breaking change, since its headers go.

Raising the floor of something the library builds against - a platform pack's
upstream RTOS, network stack or crypto library, the CMake version, the C
standard - is **not** a breaking change, and is not marked as one, because no
SolidSyslog header changed and code written against the previous release builds
unchanged once the integrator meets the new floor. The exception is a floor that
forces a change to a vendor type a public header passes through: the header
changed, so it is a break. A new major version of an upstream is not a floor
move: it is platform work, decided in an epic of its own, and may land as a
second pack rather than a raised floor. [`SUPPORT.md`](../SUPPORT.md) states
how the floors move and what a major version means.

A raised floor is always listed under *Before you upgrade*, which is wider than
the version bump: it holds everything an integrator must act on, whether or not
it changed the version. The record of a floor is the platform's own page under
`docs/platforms/<slug>/`, which states the minimum and the version the pack is
tested against; the release process reads the floors from the diff of those
pages rather than from a commit marker.

## What the release notes contain

A release's notes open with one paragraph naming what the release is for: each
theme in a sentence, linking the page that specifies it, and any platform added.
Then four written sections and the generated list, in this order. The written
sections are added by hand in the release pull request. There is no feature
summary beyond the opening paragraph: the generated list is the inventory, and a
section restating it in other words was dropped at 0.4.0 as repetition.

1. **Before you upgrade** - every change that stops code written for the previous
   release building, or changes what it does without a build error: a signature,
   a config field, a vtable slot an adapter fills, a default. Say for each what to
   change and, where there is one, how to keep the old behaviour. Take the list
   from `git diff v<previous>..HEAD -- Core/Interface 'Platform/*/Interface/**'`,
   quoted so that a removed pack's headers are in the diff, and the merged
   stories, not from the generated list. Then take the raised floors from
   `git diff v<previous>..HEAD -- docs/platforms/*/index.md CMakeLists.txt docs/builds.md`,
   which covers the packs' upstream floors, the CMake floor and the C standard,
   since a raised floor is not a breaking change and appears in neither the
   interface diff nor the generated list (see
   [What is a breaking change](#what-is-a-breaking-change)).
   When nothing breaks, say so in one line. The section holds only what an
   integrator acts on; a fix or a feature the generated list already names is not
   repeated here.

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
2. **RFC compliance at this release** - state in prose what moved - a clause
   newly met, a standard newly covered. Restate the summary table whenever a
   number changed; one line saying nothing changed when none did. Read the
   numbers off the matrix at the tag, not from memory.

   **This table is restated rather than linked**, which is deliberate and
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
3. **Known limitations** - the defects tracked at the release, listed here, each
   linking its tracking issue. State that they were found by audit and that the
   page of each affected platform discloses them too: a bare list of open
   defects reads as unfinished work, and the same facts framed as deliberate
   disclosure read as rigour. A divergence a platform accepts by design is not a
   limitation of the release: it is stated on that platform's page alone. When
   none is tracked, one sentence says so.
4. **Verifying this release** - the attached assets, how they are signed, and a
   link to the verification guide at the tag. The check that matters is that a
   bundle verifies, not that the assets are present.
5. **Full changelog** - release-please's generated list.

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

A merged pull request's message can be replaced after the fact. A block in its
body of the form

```text
BEGIN_COMMIT_OVERRIDE
feat: what the consumer gained (#<number>)
END_COMMIT_OVERRIDE
```

is what release-please reads in place of the squash message, on its next run,
for any pull request merged since the last release. Use it to correct a type,
and to give an epic's stories one entry that says what the consumer gained,
retyping the other stories as `chore` so they drop out of the list. The
commits on `main` are untouched; only the changelog reads differently.

## Cutting a release

`CHANGELOG.md` is the authoritative record of what changed between releases. Its
generated section is built by release-please from the Conventional Commit types
listed above; the written sections are added by hand in the release pull request.

1. Conventional Commits land on `main`. `feat` and `fix` map to a generated
   CHANGELOG section; the hidden types above produce no entry, and a commit marked
   as breaking is listed whatever its type.
2. release-please maintains a release PR that bumps the version and
   `CHANGELOG.md`, and rewrites that branch on every push to `main`, discarding
   anything written there. Write the written sections of the release notes into
   that PR, per *Getting the written parts into both places* above, only after
   the last merge before the release has landed and release-please has run on
   it; then merge the release PR before anything else lands.
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
   tag's SHA and so can only be cut once the tag exists. The branch is the
   first of four parts of a stack's release, and the other three are listed
   under *Releasing the example stacks* below. Publish with
   `gh release edit v<version> --draft=false --latest`, so the release is the
   one GitHub serves as latest; a fix to an older line takes `--latest=false`
   instead.
6. The `release: published` event triggers `sbom.yml`: it renders and validates
   the CycloneDX SBOM, writes the content-tree SHA-256 (scope: `Core/` +
   `Platform/` + `CMakeLists.txt`, `CMakePresets.json`, `LICENSE.md`,
   `LICENSES/`), cosign keyless-signs both (GitHub OIDC), and attaches each
   file with its signature to the Release.
7. The same event triggers `docs-bundle.yml`: it measures coverage from the
   tag, with the gate CI applies, fetches the JUnit results of the CI run that
   tested the tagged commit on `main`, builds the documentation from the tag
   with `mkdocs-offline.yml` and the coverage and test reports under its
   Testing pages, checks that the result works with no server and no network,
   cosign-signs the zip and attaches it with its signature. The bundle is built
   and checked on every CI run as well, so only the signing and the attachment
   wait for a release. A coverage gate that fails here, or a CI run whose
   results have expired - they are kept a month - withholds the bundle: a
   release does not ship with its evidence missing, so fix the cause and
   re-run.
8. Signing and attachment hard-fail. The Release already exists by the time the
   jobs run, so a failure cannot block it - it means the Release went out
   without provenance. A red run is the signal: fix the cause and re-run the
   job (see [release verification](security/release-verification.md)).
9. Close the release's milestone when the checklist below is complete, and
   create the next one if it does not exist. The milestone is a progress view
   of a release that is being worked on; an open milestone with nothing left in
   it reads as a release still in flight. [CLAUDE.md](../CLAUDE.md#milestones)
   states what goes in the next one, and when it carries a due date.

### Bringing the documentation to the release

The documentation is brought to a release on one branch, from one issue, and that
branch merges after the last functional change and before the release pull request.
It carries everything the checklist below names: the example stacks' `release/<version>`
links, the `GIT_TAG`, the cost figures in `README.md`, and `docs/hardening-path.md`
re-taken stage by stage. The figures come from the example stacks' `wip/<version>`
branches, replayed onto the current `main` and swept; the evidence commit on each
release branch states why the figures hold at the tag. From the moment the branch merges
the site links release branches that do not yet exist, so the release PR, the tag, the
release branches and the publication follow in one sitting.

### Releasing the example stacks

Each example stack - `solid-syslog-example` and `solid-syslog-example-make` -
gets a release of its own for every library release. The library's
documentation links the branch by name, so the branch exists before the
library release is published; the rest follows it. A stack's release is
finished when all four parts exist, and the stack's `main` README names them.

1. The branch `release/<version>`: the Baseline with `solid-syslog.pin` set to
   the tag's SHA and the pin line of its commit message reading
   `v<version>, <short SHA>.`, and the stage commits rebased onto it.
2. An `evidence:` commit at the branch tip holding the Sweep run's output under
   `evidence/`. Its message names the run and the commit the sweep measured,
   and, where that commit is not the tag, why the figures hold at the tag.
3. A tag `v<version>` at the branch tip, carrying a GitHub Release marked
   latest, with the sweep output attached as `sweep-<version>.zip` and a
   description naming the branch, the library tag and SHA it pins, and the
   Sweep run. One command creates them together: with an asset named, gh
   creates the Release as a draft, uploads the asset, then publishes, so a
   failed upload leaves a draft rather than a published Release without its
   file. Confirm with `gh release view v<version>` that the asset is listed:

   ```bash
   gh release create v<version> --repo cososo-ltd/<stack> --target <tip> \
     --title <version> --latest --notes-file <file> sweep-<version>.zip
   ```

4. A row for the branch in the *Releases* table of the stack's `main` README,
   committed to `main`.

Steps 1 and 2 happen before the library release is published, steps 3 and 4
after it; none of them is done by a workflow.

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
      included, written after release-please's run on the last merge before the
      release.
- [ ] Each pack's page states the latest patch or minor release of its upstream
      within the major it supports. The version each page names is the one CI
      built against - `docs-build` holds the pages to the record every lane
      uploads, so a green `main` settles that - and the test report's
      [Versions](testing/tests.md) table shows what that was. Compare it with
      the upstream's latest release. A pack left behind is either bumped before
      the release or listed under *Known limitations* with the reason, per
      [`SUPPORT.md`](../SUPPORT.md).
- [ ] Run each target that has a target checklist against the release PR's head,
      and record the run as a comment on the PR, per
      [Cutting a release](#cutting-a-release).
- [ ] Cut the release by merging the release PR.
- [ ] Confirm the tag and the draft GitHub Release, and read the notes in the
      draft before anything goes out.
- [ ] Set the draft's notes explicitly with `gh release edit`, per
      [Getting the written parts into both places](#getting-the-written-parts-into-both-places).
- [ ] Cut the example stacks' `release/<version>` branches pinned to the tag's
      SHA, each with its evidence commit, then publish the draft.
- [ ] Confirm GitHub marks the new release as Latest:
      `gh api repos/cososo-ltd/solid-syslog/releases/latest --jq .tag_name`.
- [ ] Finish each example stack's release: the `v<version>` tag and GitHub
      Release with `sweep-<version>.zip` attached, and the row on its `main`
      README, per [Releasing the example stacks](#releasing-the-example-stacks).
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
- [ ] Close the release's milestone, and create the next one if it does not
      exist: `gh api -X PATCH repos/cososo-ltd/solid-syslog/milestones/<number>
      -f state=closed`, then `gh api -X POST repos/cososo-ltd/solid-syslog/milestones
      -f title=<next-version>` if `gh api repos/cososo-ltd/solid-syslog/milestones
      --jq '.[].title'` does not list `<next-version>`.
- [ ] Security release: publish the coordinated GHSA.
