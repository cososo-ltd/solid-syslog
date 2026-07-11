# Software Bill of Materials (SBOM)

SolidSyslog publishes a [CycloneDX](https://cyclonedx.org/) 1.5 SBOM for the
shipped library. SBOMs come in three flavours that answer three different
questions — this document is only concerned with the first.

| Flavour | Question it answers | Status here |
|---|---|---|
| **Product SBOM** | "What am I linking against in my deployment?" | Covered by this workflow (see below). |
| **Build / dev-env SBOM** | "What tools, containers, and test harnesses were used to produce the release?" | Not yet — deferred to a separate story. Container image SHAs are tracked in `docs/containers.md` for now. |
| **Source SBOM** | "What third-party source code is embedded in the product?" | Empty — SolidSyslog vendors no third-party source. |

## Product SBOM scope

In scope:

- `Core/` — Tier 1 (full support, stable API).
- `Platform/` — Tier 2 (supported; API may evolve per target).
- Root `CMakeLists.txt` + `CMakePresets.json` — the build contract an
  integrator invokes directly. Tampering here affects the built library.
- Root `LICENSE.md` — the licence text we are legally bound by and that
  downstream integrators inherit. Tampering here is a compliance issue.

Out of scope:

- `Tests/`, `Bdd/` — test harnesses (`Bdd/Targets/` holds the BDD-driven binaries — test infrastructure, not product).
- `ci/`, `docs/`, `.devcontainer/`, `.github/`, `.vscode/` — dev/CI infrastructure.
- `sbom/` — the SBOM template itself (meta; including it would be self-referential).
- `scripts/` — utility scripts not consumed by the integrator.
- Other root-level meta files (`CLAUDE.md`, `SKILL.md`, `DEVLOG.md`,
  `README.md`, `CHANGELOG.md`, `.clang-format`, `.clang-tidy`,
  `.gitattributes`, `.gitignore`, `.release-please-manifest.json`).
  Informational / agent-facing / git configuration, not library source.

Runtime dependencies we declare but do not bundle:

- **OpenSSL** — optional, only when `SOLIDSYSLOG_OPENSSL=ON`. Listed as a
  CycloneDX component with `scope: optional`. No version pinned —
  integrators select their own OpenSSL and capture it in their own SBOM
  alongside the specific licence terms of the version they ship.

Runtime dependencies we document as environment (not components):

- **POSIX libc / Winsock / POSIX message queues** — host OS APIs, not
  shipped software. Recorded as `metadata.properties` rather than
  components.

## What the SBOM says

The SBOM is a single-component document. `Core/` is a pure-C library with no
runtime dependencies — so the subject (`metadata.component`) is SolidSyslog
itself, and the top-level `components` array is empty. Runtime facts that a
deployer must supply (a POSIX or Windows host, optionally a TLS library
implementing the Stream abstraction) are documented as `properties`, not as
components — they are *requirements on the deployment*, not *shipped software*.

Key fields worth reading:

| Field | Meaning |
|---|---|
| `metadata.component.name` | `SolidSyslog`. |
| `metadata.component.version` | The value from `.release-please-manifest.json` at the time of generation. Pre-release: `0.0.0`. |
| `metadata.component.purl` | Package URL keyed to the exact commit SHA — unambiguous pointer back to the source. |
| `metadata.component.supplier.name` | `COSOSO (Cozens Software Solutions Limited)`. |
| `metadata.component.licenses[0].license.id` | `PolyForm-Noncommercial-1.0.0` — SPDX identifier. |
| `metadata.properties[solidsyslog:source-tree-sha256]` | Content-tree hash: SHA-256 of a sorted list of `<content-sha256>  <path>` lines for every tracked file in `Core/` and `Platform/` at the commit. Reproducible byte-for-byte from any clone, with no dependency on `git archive` output format or git version. |

## How to generate one (rehearsal)

Each run produces a CycloneDX 1.5 JSON file, validated against the spec by
[`cyclonedx-cli`](https://github.com/CycloneDX/cyclonedx-cli), and uploaded as
a workflow artifact.

1. Open the **Actions** tab.
2. Select the **Generate SBOM** workflow.
3. Click **Run workflow**, pick the ref (usually `main` or a release tag),
   and **Run workflow**.
4. When the run completes, scroll to **Artifacts** at the bottom of the run
   page and download `sbom-cyclonedx-<version>`.
5. Unzip; the file inside is `sbom.cdx.json`.

The workflow uses only the default `GITHUB_TOKEN` — no repo secrets required.

## Sanity-check a generated SBOM

```shell
cyclonedx validate --input-file sbom.cdx.json --input-format json --input-version v1_5 --fail-on-errors
```

The CI workflow already runs this; the command is useful if you've fetched
the artifact locally and want to re-verify independently.

## Verifying a signed SBOM

Every GitHub Release created by Release Please gets four assets attached:

| Asset | Contents |
|---|---|
| `sbom.cdx.json` | The SBOM itself. |
| `sbom.cdx.json.bundle` | [sigstore/cosign](https://docs.sigstore.dev/) signature bundle — signature + ephemeral signing certificate + Rekor inclusion proof, in a single JSON blob. |
| `source-tree-sha256.txt` | The content-tree SHA-256 with a human-readable header. Reproducible from any clone at the SBOM's commit with `git ls-tree` + `git show` + `sha256sum` + `sort`. |
| `source-tree-sha256.txt.bundle` | cosign bundle for the above. |

Signing is **keyless** via GitHub OIDC — no private keys live in this repo.
The signature commits to the specific workflow run (`sbom.yml` in this repo
at the tagged commit) that produced the SBOM; a verifier checks the
certificate identity against an expected workflow identity to tell "this
SBOM" apart from any other CycloneDX document.

To verify a downloaded asset set:

```shell
cosign verify-blob \
  --bundle sbom.cdx.json.bundle \
  --certificate-identity "https://github.com/cososo-ltd/solid-syslog/.github/workflows/sbom.yml@refs/tags/v<version>" \
  --certificate-oidc-issuer "https://token.actions.githubusercontent.com" \
  sbom.cdx.json
```

The same pattern verifies `source-tree-sha256.txt.bundle` against `source-tree-sha256.txt`.

Every cosign signature is also logged to [Rekor](https://docs.sigstore.dev/logging/overview/),
Sigstore's public transparency log. Anyone can look up the signature entry
by its hash and confirm it was issued at the stated time — independent of
whether GitHub, Sigstore, or this project still exist at the time of audit.

For a step-by-step verification guide aimed at downstream integrators, see
[`release-verification.md`](./release-verification.md).

## Deferred

- **Signed SLSA provenance attestation.** `cosign attest` on top of
  `sign-blob` is a natural next step — it produces an attestation
  statement that says "this SBOM was produced by this workflow from
  these inputs" rather than just "this SBOM was signed by this
  workflow." Separate E19 follow-on.
- **Binary-artefact signing.** The project is source-only; nothing to
  sign beyond the SBOM and content-tree hash.
- **Flip the signing/attach steps off `continue-on-error: true`.** The
  initial rollout keeps those steps advisory so a signing infrastructure
  outage doesn't block a release. Tighten to hard-fail after the first
  real release has demonstrated the pipeline works.
