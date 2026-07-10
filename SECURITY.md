# Security Policy

Cozens Software Solutions Limited (COSOSO) takes the security of SolidSyslog
seriously. This policy explains how to report a vulnerability and what to
expect in return.

## Reporting a vulnerability

**Please do not report security issues in public GitHub issues, pull
requests, or discussions.** Use one of the private channels below:

1. **GitHub private vulnerability reporting (preferred).** On this
   repository, go to the **Security** tab → **Report a vulnerability**.
   This opens a private advisory visible only to you and the maintainer.
2. **Web form.** If you cannot use GitHub, submit the form at
   **https://cososo.co.uk/security/report**. It routes to a private inbox.

We do not publish a security email address. Both channels above reach the
maintainer privately.

### What to include

The more of this you can provide, the faster we can triage:

- Affected component — **Core**, **Platform**, or **Bdd** (see *Scope* below)
- Affected version, tag, or commit SHA
- A description of the issue and its impact
- Reproduction steps or a proof of concept
- Your assessment of severity, and whether it is being actively exploited
- Whether you wish to be credited, and how

## Our commitments

For the free (noncommercial) tier:

- **Acknowledgement** within **72 hours** of receipt.
- **Initial triage** and a **CVSS v3.1** assessment within **7 days**.
- **Fixes** are best-effort and severity-driven. The free tier carries no
  fixed fix-time SLA. Extended support and guaranteed timelines are
  available under a commercial agreement.

These timelines are best-effort commitments from a solo maintainer. They may
be exceeded during periods of illness, bereavement, or other circumstances
beyond the maintainer's control. In such cases the report will be
acknowledged and acted on as soon as is practicable.

## Coordinated disclosure

We follow coordinated disclosure with a **90-day** window from the date of
report, plus a **14-day grace period** if a fix is imminent at day 90. We
will credit reporters who wish to be named — just tell us your preference in
your report.

Advisories are published as **GitHub Security Advisories**. For
vulnerabilities in **Core** (see *Scope*), a **CVE** is requested via
GitHub's CNA.

## Scope

SolidSyslog is a source-available component assembled into a product by the
integrator. Security treatment follows the repository's support tiers:

| Tier | Path | Treatment |
|---|---|---|
| 1 | `Core/` | Full treatment: CVE, advisory, fix, signed release, SBOM entry. |
| 2 | `Platform/` | Advisory treatment: public notice and an updated adapter. No CVE unless the root cause is in `Core/`. |
| 3 | `Bdd/Targets/` | Advisory treatment. Illustrative targets, not a supported product. |

Test code, build infrastructure, CI configuration, and documentation are not
in scope for this policy. Vulnerabilities in third-party libraries you choose
to link (OpenSSL, Mbed TLS, an RTOS TCP stack, …) are the responsibility of
those projects and your own supply-chain process; SolidSyslog bundles none of
them.

See the [threat model](docs/security/threat-model.md) for trust boundaries,
caller obligations, and what the library does and does not defend against.

## Supported versions

SolidSyslog is maintained as a **single mainline**. Security fixes land on the
**latest published release** only. There is no back-port commitment on the
free tier; back-ports are available under a commercial agreement.

## Continuity

If COSOSO ceases to maintain SolidSyslog without transferring it to a
successor maintainer, the then-current release will be relicensed under a
permissive open-source licence (for example Apache-2.0) so that existing
users are not stranded.

## Commercial support

For commercial licensing, guaranteed response times, or back-port support,
enquire via the form at
[cososo.co.uk](https://www.cososo.co.uk/?service=solidsyslog#contact).
