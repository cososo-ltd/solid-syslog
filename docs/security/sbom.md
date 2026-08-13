# Software Bill of Materials (SBOM)

SolidSyslog publishes a [CycloneDX](https://cyclonedx.org/) 1.5 SBOM for the
shipped library. SBOMs come in three flavours that answer three different
questions; this document is only concerned with the first.

| Flavour | Question it answers | Status here |
|---|---|---|
| **Product SBOM** | "What am I linking against in my deployment?" | Covered by this workflow (see below). |
| **Build / dev-env SBOM** | "What tools, containers, and test harnesses were used to produce the release?" | Not yet — deferred to a separate story. Meanwhile every container image is pinned by digest at the point it is used, and [Container images](../containers.md) names them. |
| **Source SBOM** | "What third-party source code is embedded in the product?" | Empty — SolidSyslog vendors no third-party source. |

## Product SBOM scope

In scope:

- `Core/`: Tier 1 (full support, stable API).
- `Platform/`: Tier 2 (supported; API may evolve per target).
- Root `CMakeLists.txt` + `CMakePresets.json`: the build contract an
  integrator invokes directly. Tampering here affects the built library.
- Root `LICENSE.md` and `LICENSES/`: the licence terms we are legally bound by
  and that downstream integrators inherit (see
  [`LICENSE.md`](../../LICENSE.md)). Tampering here is a compliance issue.

Out of scope:

- `Tests/`, `Bdd/`: test harnesses (`Bdd/Targets/` holds the BDD-driven binaries, test infrastructure, not product).
- `ci/`, `docs/`, `.devcontainer/`, `.github/`, `.vscode/`: dev/CI infrastructure.
- `sbom/`: the SBOM template itself (meta; including it would be self-referential).
- `scripts/`: utility scripts not consumed by the integrator.
- Other root-level meta files (`CLAUDE.md`, `SKILL.md`,
  `README.md`, `CHANGELOG.md`, `.clang-format`, `.clang-tidy`,
  `.gitattributes`, `.gitignore`, `.release-please-manifest.json`).
  Informational / agent-facing / git configuration, not library source.

## What the SBOM says

The SBOM is a single-component document. The subject (`metadata.component`) is
SolidSyslog, the `components` array is empty, and the dependency graph records
that it depends on nothing.

That is the whole point of it. SolidSyslog vendors no third-party source, and
Core reaches no further than a C99 compiler and four standard headers —
`<stddef.h>`, `<stdbool.h>`, `<stdint.h>` and `<string.h>`. It calls `memcpy`
and `strlen`, and makes no operating-system calls at all: no OS, no network
stack, no filesystem, no heap. The `c99` lane proves it on every pull request
by building Core alone, and the FreeRTOS cross lanes build it with no host
platform present.

### Why the platform backends are not components

Reference adapters ship for POSIX, Windows, FreeRTOS, lwIP, FreeRTOS-Plus-TCP,
ChaN FatFs, FreeRTOS-Plus-FAT, OpenSSL and Mbed TLS, and an integrator may
supply their own instead. None of them is a component of SolidSyslog. Each is
software the integrator chooses, versions, links and licenses, and the choices
are mutually exclusive per role — no build links both OpenSSL and Mbed TLS, or
both lwIP and FreeRTOS-Plus-TCP.

Listing them would therefore describe a product that does not exist, and would
do real harm: a scanner reading an OpenSSL dependency against a FreeRTOS build
that links Mbed TLS reports vulnerabilities in software that is not there. We
could pin no version for any of them either, so each entry would look like
coverage while carrying nothing a scanner can use.

They are recorded instead as `metadata.component.properties`
(`solidsyslog:runtime-environment` and `solidsyslog:platform-backends`),
alongside an `externalReferences` link to [Adding it to your
build](../build-integration.md) for the capability matrix. Whichever packs you
select are your dependencies and belong in your product SBOM — which is the
document that can state them correctly, because it knows which build you
shipped.

Key fields worth reading:

| Field | Meaning |
|---|---|
| `metadata.tools.components[0]` | The workflow that rendered this document, versioned by its own commit SHA and linked via a `build-system` reference. Distinct from `metadata.component.purl`, which pins the source being described: one says what produced the SBOM, the other what it describes. |
| `metadata.component.name` | `SolidSyslog`. |
| `metadata.component.version` | The value from `.release-please-manifest.json` at the time of generation. Pre-release: `0.0.0`. |
| `metadata.component.purl` | Package URL keyed to the exact commit SHA — unambiguous pointer back to the source. |
| `metadata.component.supplier.name` | `Cozens Software Solutions Limited (COSOSO)`. |
| `metadata.component.externalReferences[type=license]` | One per PolyForm term, so a scanner resolves each to its canonical text instead of leaving it unknown. The commercial term has no licence document to point at and carries an `other` reference to the enquiry route instead. See [reading the licence expression](#reading-the-licence-expression). |
| `metadata.component.licenses[0].expression` | `PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial` — an SPDX expression, because the library is offered under three alternative licences and the recipient chooses. Only the Noncommercial identifier is on the SPDX License List; the other two are `LicenseRef-`. |
| `metadata.properties[solidsyslog:source-tree-sha256]` | Content-tree hash: SHA-256 of a sorted list of `<content-sha256>  <path>` lines for every tracked file in `Core/` + `Platform/` plus the root-level `CMakeLists.txt`, `CMakePresets.json`, `LICENSE.md`, and `LICENSES/`, at the commit. Reproducible byte-for-byte from any clone, with no dependency on `git archive` output format or git version. |

## Reading the licence expression

Written for a compliance reviewer holding SolidSyslog as an item in a review
queue. If you are choosing a licence rather than reviewing one,
[`LICENSE.md`](../../LICENSE.md) is the document you want instead.

`metadata.component.licenses[0].expression` reads:

```text
PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
```

**`OR` is a choice, not an accumulation.** SPDX defines the operator as
alternatives: a recipient relies on one term and complies with that one. The
component is not encumbered by all three at once, and nothing obliges a
recipient to satisfy the noncommercial term if they hold a different one.

This expression is the whole licence question for the component. SolidSyslog
vendors no third-party source, so the `components` array is empty and there is
no transitive licence graph underneath it — see
[why the platform backends are not components](#why-the-platform-backends-are-not-components).

### Why two of the three are `LicenseRef-`

Only `PolyForm-Noncommercial-1.0.0` appears on the
[SPDX License List](https://spdx.org/licenses/). The other two do not, for
different reasons:

| Term | Why it is a `LicenseRef-` |
|---|---|
| `LicenseRef-PolyForm-Internal-Use-1.0.0` | A published, unmodified PolyForm licence that SPDX has not listed. The verbatim text ships in the repository and is linked from [`LICENSE.md`](../../LICENSE.md). |
| `LicenseRef-COSOSO-Commercial` | A negotiated agreement between COSOSO and the licensee. It has no single public text, so no published identifier could describe it. |

If your scanner reports these identifiers as unknown or custom, that result is
expected. A `LicenseRef-` is how SPDX names a licence its list does not cover,
so there is no template in any scanner's corpus for it to match — and for the
PolyForm term the verbatim text ships in the repository, for a reviewer who
needs to read it.

An unknown identifier is not, by itself, evidence of a licence defect. Neither
is it a resolution: the item stays open until you have confirmed which of the
three terms your organisation relies on, and hold the evidence for it.

So that the terms resolve to documents rather than to nothing, the SBOM carries
an `externalReferences` entry of type `license` for each PolyForm licence,
pointing at its canonical URL. An SPDX expression has nowhere to put a URL,
which is why they are attached to the component instead.

`LicenseRef-COSOSO-Commercial` has no such entry. There is no public document to
point at, and type `license` means the URL of a licence file — labelling an
enquiry form as one would resolve the identifier to something that is not a
licence. The enquiry route is carried as an `other` reference instead, with a
comment saying what it is.

### Which term applies

| If your organisation is | The term you rely on |
|---|---|
| Using SolidSyslog in the internal business operations of you and your company — which covers evaluation, porting, integration and testing, and internal deployment, commercial organisations included | `LicenseRef-PolyForm-Internal-Use-1.0.0` |
| Distributing it for a noncommercial purpose | `PolyForm-Noncommercial-1.0.0` |
| Supplying, selling or otherwise making available a commercial product, device, firmware or service containing it | `LicenseRef-COSOSO-Commercial` |

If you arrived here from a source file rather than from the SBOM, the same
answer applies. Every file under `Core/` and `Platform/` opens with a
`SPDX-License-Identifier` carrying this same three-term expression, for the case
where a file is copied into another build and leaves `LICENSE.md` behind. It is
the same disjunction and it resolves the same way — establish which term your
organisation relies on, and record that one.

This table says only which term to read. The conditions each one attaches are
in [`LICENSE.md`](../../LICENSE.md), which is the authoritative statement.

### What to record in your own SBOM

The expression above is SolidSyslog's **declared** licence — what COSOSO offers
to any recipient. It is not a statement about your organisation's position.

Once you have taken one of the three, record **that single term** as the
concluded licence for this component in your own product SBOM. Carrying the
disjunction forward re-raises the same review item on every rebuild, and
misstates your position to anyone reading your SBOM downstream.

`PolyForm-Noncommercial-1.0.0` is on the SPDX list, so an expression carries
everything a tool needs to identify it:

```json
"licenses": [
  { "expression": "PolyForm-Noncommercial-1.0.0" }
]
```

The other two are `LicenseRef-` terms, and an expression has nowhere to put a
name or a URL for them — which is the whole reason they resolve as unknown. Use
CycloneDX's named-licence form instead. PolyForm Internal Use is a published
document, so its canonical URL is enough:

```json
"licenses": [
  {
    "license": {
      "name": "PolyForm Internal Use License 1.0.0",
      "url": "https://polyformproject.org/licenses/internal-use/1.0.0"
    }
  }
]
```

`LicenseRef-COSOSO-Commercial` needs more again. It names a class of negotiated
agreement rather than your particular contract, so on its own it tells a
downstream reader of your SBOM nothing about what was granted. CycloneDX has
fields for exactly this — use the named-licence form:

```json
"licenses": [
  {
    "license": {
      "name": "COSOSO Commercial Licence",
      "url": "https://www.cososo.co.uk/#contact",
      "licensing": {
        "licensor": {
          "organization": { "name": "Cozens Software Solutions Limited" }
        },
        "licensee": {
          "organization": { "name": "<your organisation>" }
        },
        "purchaseOrder": "<your agreement or PO reference>",
        "licenseTypes": ["oem"],
        "expiration": "<RFC 3339 timestamp, if your agreement has a term>"
      }
    }
  }
]
```

`licenseTypes` takes values from CycloneDX's own enumeration — `oem`,
`appliance`, `perpetual`, `subscription` and others — so pick whichever
describes your agreement. A `licenses` array is *either* a list of named
licences *or* exactly one expression; the two forms cannot be mixed.

**Do not put the agreement itself in `license.text`.** A negotiated commercial
agreement is confidential between the parties, and an SBOM is a document you
distribute. The reference is what belongs here; the terms are not.

The same question applies to `purchaseOrder` and `licensee`. They are exactly
right in an internal compliance record, but if you pass this SBOM on to your
own customers, your commercial arrangements travel with it. Decide which of
these fields belong in the copy you distribute and which stay in the copy you
keep — `name` and `url` alone are enough to resolve the identifier.

Whichever term applies, keep the evidence for it — the agreement reference for
a commercial licence, or a record of the permitted purpose relied on for a
PolyForm one — in your compliance record alongside the SBOM entry. That is what
lets the next reviewer resolve this without repeating your work.

### A policy rule you can adopt

Stated in prose rather than a vendor syntax, because the encoding differs
across scanning platforms:

> SolidSyslog is a disjunctively multi-licensed component. Resolve it to the
> single term this organisation holds, record that as the concluded licence
> together with the evidence supporting it, and close the item on that basis.
> Do not assess the component against the noncommercial term unless that is the
> term being relied on.

If your review turns up a question this page does not answer, ask before
escalating it internally: <https://www.cososo.co.uk/#contact>.

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

The workflow uses only the default `GITHUB_TOKEN`: no repo secrets required.

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
| `sbom.cdx.json.sigstore` | [sigstore/cosign](https://docs.sigstore.dev/) signature bundle — signature + ephemeral signing certificate + Rekor inclusion proof, in a single JSON blob. |
| `source-tree-sha256.txt` | The content-tree SHA-256 with a human-readable header. Reproducible from any clone at the SBOM's commit with `git ls-tree` + `git show` + `sha256sum` + `sort`. |
| `source-tree-sha256.txt.sigstore` | cosign bundle for the above. |

Signing is keyless via GitHub OIDC: no private keys live in this repo.
The signature commits to the specific workflow run (`sbom.yml` in this repo
at the tagged commit) that produced the SBOM; a verifier checks the
certificate identity against an expected workflow identity to tell "this
SBOM" apart from any other CycloneDX document.

To verify a downloaded asset set:

```shell
cosign verify-blob \
  --bundle sbom.cdx.json.sigstore \
  --certificate-identity "https://github.com/cososo-ltd/solid-syslog/.github/workflows/sbom.yml@refs/tags/v<version>" \
  --certificate-oidc-issuer "https://token.actions.githubusercontent.com" \
  sbom.cdx.json
```

The same pattern verifies `source-tree-sha256.txt.sigstore` against `source-tree-sha256.txt`.

Every cosign signature is also logged to [Rekor](https://docs.sigstore.dev/logging/overview/),
Sigstore's public transparency log. Anyone can look up the signature entry
by its hash and confirm it was issued at the stated time, independent of
whether GitHub, Sigstore, or this project still exist at the time of audit.

For a step-by-step verification guide aimed at downstream integrators, see
[`release-verification.md`](./release-verification.md).

## Deferred

- Signed SLSA provenance attestation. `cosign attest` on top of
  `sign-blob` is a natural next step: it produces an attestation
  statement that says "this SBOM was produced by this workflow from
  these inputs" rather than just "this SBOM was signed by this
  workflow."
- Binary-artefact signing. The project is source-only; nothing to
  sign beyond the SBOM and content-tree hash.
