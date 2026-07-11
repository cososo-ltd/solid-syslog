# Release Verification Guide

You have a SolidSyslog release on disk and want to convince yourself, or an
auditor, that it was produced by the repo it claims to come from. This guide
walks through the verification a careful integrator would run, in the order
they'd naturally run it.

Prerequisites:

- [cosign](https://docs.sigstore.dev/system_config/installation/) v2 or
  later on your `$PATH`.
- [cyclonedx-cli](https://github.com/CycloneDX/cyclonedx-cli) v0.30.0 or
  later (optional — only needed to re-validate the SBOM).
- A `git` checkout of the repo at the release's tag (optional — only
  needed to reproduce the source hash yourself).

All four Release assets should be present:

```text
sbom.cdx.json
sbom.cdx.json.bundle
source-tree-sha256.txt
source-tree-sha256.txt.bundle
```

## 1. Verify the source is what we claim

`source-tree-sha256.txt` records the content-tree SHA-256 of the product at
the release commit. The product scope is `Core/` + `Platform/` plus the
root-level `CMakeLists.txt`, `CMakePresets.json`, and `LICENSE.md`. The hash
is deterministic across git versions, archive formats, locales, and tooling
— it depends only on the bytes of each tracked file and the sorted list of
paths.

### Reproduce with git (authoritative)

```shell
git clone --depth 1 --branch v<version> https://github.com/cososo-ltd/solid-syslog.git
cd solid-syslog
git ls-tree -r --name-only HEAD -- \
    Core/ Platform/ CMakeLists.txt CMakePresets.json LICENSE.md \
  | LC_ALL=C sort \
  | while IFS= read -r path; do
      printf "%s  %s\n" "$(git show "HEAD:$path" | sha256sum | cut -d' ' -f1)" "$path"
    done \
  | sha256sum | cut -d' ' -f1
```

Compare the output to the hash in `source-tree-sha256.txt`. The file has a
few `#`-prefixed header lines (scope / commit / algorithm / pointer) and one
blank line before the hash, so extract the value with:

```shell
grep -v '^#' source-tree-sha256.txt | grep -v '^$' | head -n 1
```

If that matches your computed hash, your source tree is byte-identical to
the tree the SBOM describes.

### Reproduce without git (fallback, working tree only)

If you don't have a git clone — e.g. you received a source archive instead —
but you do have an extracted working tree with the product files present:

```shell
find Core Platform CMakeLists.txt CMakePresets.json LICENSE.md -type f \
  | LC_ALL=C sort \
  | while IFS= read -r path; do
      printf "%s  %s\n" "$(sha256sum "$path" | cut -d' ' -f1)" "$path"
    done \
  | sha256sum | cut -d' ' -f1
```

Same hash expected, same comparison. Caveats:

- Assumes the working tree is clean — any untracked or locally-modified
  file under the in-scope paths will change the hash.
- Assumes line endings are LF on disk (git's `.gitattributes` enforces
  this at checkout; some extraction tools may convert on platforms that
  default to CRLF).

The git form is stricter because it always reads from the committed tree.

### If the hashes don't match

The SBOM's `metadata.properties[solidsyslog:commit-sha]` property names the
exact commit the hash was computed at. Verify you're on that commit:

```shell
git rev-parse HEAD
```

If the commit matches and the hash still doesn't, something on your side
has modified a file; `git status --short` and `git diff` will show what.

## 2. Verify the SBOM signature

The signature commits to the GitHub Actions workflow that produced it.
A valid signature proves three things:

1. This exact SBOM came from the SolidSyslog repo's `sbom.yml` workflow.
2. The workflow ran at the release tag you think it did (not on a random
   branch, not on a fork).
3. The signing event was logged to the Sigstore transparency log at the
   time claimed.

Verification:

```shell
cosign verify-blob \
  --bundle sbom.cdx.json.bundle \
  --certificate-identity "https://github.com/cososo-ltd/solid-syslog/.github/workflows/sbom.yml@refs/tags/v<version>" \
  --certificate-oidc-issuer "https://token.actions.githubusercontent.com" \
  sbom.cdx.json
```

`cosign` outputs `Verified OK` and exits 0 on success. Any of these
conditions fail the verification loudly:

- The SBOM has been modified after signing.
- The signature was produced by a different workflow file.
- The signature was produced on a different repo (e.g. a fork).
- The signature was produced against a different tag.
- The Sigstore transparency log entry is missing or doesn't match.

## 3. Verify the source-tree-hash signature

```shell
cosign verify-blob \
  --bundle source-tree-sha256.txt.bundle \
  --certificate-identity "https://github.com/cososo-ltd/solid-syslog/.github/workflows/sbom.yml@refs/tags/v<version>" \
  --certificate-oidc-issuer "https://token.actions.githubusercontent.com" \
  source-tree-sha256.txt
```

Same guarantees as step 2, but for the content-tree-hash file. Combined
with the hash match from step 1, you now know the source you have is the
source the SBOM describes, and the SBOM is the one the workflow produced.

## 4. (Optional) Re-validate the SBOM against CycloneDX

```shell
cyclonedx validate \
  --input-file sbom.cdx.json \
  --input-format json \
  --input-version v1_5 \
  --fail-on-errors
```

This isn't strictly a provenance check — the CI already ran it — but it
confirms the document on your disk is structurally and semantically a valid
CycloneDX 1.5 SBOM. Useful if you're plugging it into an SBOM-consuming tool
and want to rule out corruption-in-transit.

## What verification does *not* tell you

- It doesn't tell you whether the code behind the SBOM is bug-free, secure,
  or fit for purpose. The SBOM is provenance evidence, not a quality
  claim.
- It doesn't tell you anything about dependencies that SolidSyslog chose
  not to bundle. OpenSSL appears in the SBOM's `components[]` with
  `scope: optional`, but picking and verifying your OpenSSL is your job.
- It doesn't tell you whether the SolidSyslog licence is compatible with
  your intended use. That's a licence review, not a signature check.

See `docs/iec62443.md` for the security posture, and `docs/security/sbom.md`
for a walk-through of what the SBOM actually says.
