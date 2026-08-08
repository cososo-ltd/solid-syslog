"""MkDocs build hook: give every hand-written page its own meta description.

Without this, every page emits ``site_description`` — one string describing the
product, used as the search-result snippet for the CRA guide, the porting
contract and the release process alike.

The descriptions live here rather than in per-page front matter because the
same Markdown is read on GitHub, where YAML front matter renders as a table
above the content.

Keeping them out of the pages costs proximity, so the map is checked against
the documentation tree on every build: a page with no description, or a
description whose key names no page, aborts the build. Generated ``api/``
pages are exempt — their briefs come from the header doc comments.

The check is against every hand-written page, not only the ones in the nav.
Each platform's ``setup.md`` is deliberately off-nav — it is reached from the
platform's own page, because the two have different audiences — and a page a
search engine can reach still needs its own snippet.
"""

import os

from mkdocs.exceptions import PluginError

# mkdoxy generates these from Core/Interface and Platform/*/Interface headers.
GENERATED_PREFIX = "api/"

# src_uri -> the page's meta description. Around 150 characters: long enough to
# say what the page answers, short enough to survive a search-result snippet.
DESCRIPTIONS = {
    "README.md": (
        "Documentation for SolidSyslog: structured RFC 5424 syslog for embedded "
        "devices, built for CRA and IEC 62443 audit trails."
    ),
    "overview.md": (
        "One-screen orientation for evaluators: what the CRA and IEC 62443 ask of "
        "an audit-logging function, and how SolidSyslog helps."
    ),
    # Adopt
    "hardening-path.md": (
        "Twenty stages from no syslog to mutual TLS and encrypted storage — each "
        "with the question that decides it and its measured flash and RAM cost."
    ),
    "build-integration.md": (
        "Add SolidSyslog to your build: CMake, Make, or an IDE source manifest, "
        "with the adapter capability matrix and compile-time tunables."
    ),
    "structured-data.md": (
        "Author your own RFC 5424 SD-ELEMENT with the safe-by-construction API: "
        "you write the content, the library owns the framing and the escaping."
    ),
    "error-severity.md": (
        "Which severity a SolidSyslog error event carries — the urgency ladder "
        "each emit site picks from, and what each level asks of your handler."
    ),
    # Platforms — each platform's overview page followed by its setup guide,
    # in nav order, with porting last as the other half of the same question.
    "platforms/index.md": (
        "The adapter packs that reach your hardware — POSIX, Windows, FreeRTOS, "
        "lwIP, OpenSSL, Mbed TLS, FatFs and more — and the capabilities each fills."
    ),
    "platforms/posix/index.md": (
        "The POSIX adapter pack — sockets, pthreads, message queues, "
        "clock_gettime and stdio — filling six of the library's roles on Linux."
    ),
    "platforms/posix/setup.md": (
        "Wire the POSIX adapters: what to link, building a sender, "
        "the clock and hostname callbacks, and what threading needs from you."
    ),
    "platforms/windows/index.md": (
        "The Win32 and Winsock adapter pack for MSVC targets, filling the "
        "Resolver, Datagram, Stream, File, Mutex and AtomicCounter roles."
    ),
    "platforms/windows/setup.md": (
        "Wire the Win32 and Winsock adapters: starting Winsock, building "
        "a sender, the callbacks, and what threading needs from you."
    ),
    "platforms/freertos/index.md": (
        "The FreeRTOS adapter pack: kernel primitives filling the Mutex role and "
        "the sysUpTime callback, with networking from Plus-TCP or lwIP."
    ),
    "platforms/freertos/setup.md": (
        "Wire the FreeRTOS kernel primitives: what to link, static allocation, "
        "and putting the mutex under a buffer shared across tasks."
    ),
    "platforms/plustcp/index.md": (
        "The FreeRTOS-Plus-TCP adapter pack, filling the Resolver, Datagram and "
        "Stream roles for networking on FreeRTOS targets."
    ),
    "platforms/plustcp/setup.md": (
        "Wire the FreeRTOS-Plus-TCP adapters: what to link, resolving the "
        "collector, building a sender, and sizing the stack for it."
    ),
    "platforms/lwipraw/index.md": (
        "The lwIP Raw API adapter pack, filling the Resolver, Datagram and Stream "
        "roles — compiled against your lwipopts.h, NO_SYS=1 builds included."
    ),
    "platforms/lwipraw/setup.md": (
        "Wire SolidSyslog to the lwIP Raw API: what the adapter fills, what you "
        "supply, and how it works under NO_SYS=1 beside other lwIP subsystems."
    ),
    "platforms/openssl/index.md": (
        "The OpenSSL adapter pack: TLS transport for the Stream role, and keyed "
        "at-rest crypto for the SecurityPolicy role, on hosted targets."
    ),
    "platforms/openssl/setup.md": (
        "Wire SolidSyslogTlsStream over a TCP stream for RFC 5425 syslog over "
        "TLS: what to link, the layering, the config, and mutual TLS."
    ),
    "platforms/mbedtls/index.md": (
        "The Mbed TLS adapter pack for embedded targets: TLS transport for the "
        "Stream role, and keyed at-rest crypto for the SecurityPolicy role."
    ),
    "platforms/mbedtls/setup.md": (
        "Wire Mbed TLS for RFC 5425 syslog over TLS on an embedded target: the "
        "layering, bringing the library up, and the FreeRTOS sizing traps."
    ),
    "platforms/fatfs/index.md": (
        "The ChaN FatFs adapter pack, filling the File role beneath a BlockDevice "
        "— RTOS-agnostic, for bare-metal, FreeRTOS, Zephyr and NuttX targets."
    ),
    "platforms/fatfs/setup.md": (
        "Give store-and-forward a FatFs file backend: what to link, the disk "
        "I/O driver and configuration you supply, and what durability you get."
    ),
    "platforms/plusfat/index.md": (
        "The FreeRTOS-Plus-FAT adapter pack, filling the File role beneath a "
        "BlockDevice for an all-FreeRTOS-Plus storage and transport stack."
    ),
    "platforms/plusfat/setup.md": (
        "Back store-and-forward with FreeRTOS-Plus-FAT: what the ff_stdio File "
        "adapter needs from your build and media, and how the store sits above it."
    ),
    "platforms/atomics/index.md": (
        "The portable C11 stdatomic.h AtomicCounter — the sequenceId source on "
        "any target with a C11 compiler, with no OS dependency."
    ),
    "platforms/atomics/setup.md": (
        "Wire the C11 atomics sequence-number source: what to link, creating "
        "the counter, and why an unfilled role silently disables gap detection."
    ),
    "porting.md": (
        "Port SolidSyslog to a new RTOS, network stack, filesystem or crypto "
        "library by filling a vtable contract. Core never changes."
    ),
    # Compliance
    "cra.md": (
        "How the EU Cyber Resilience Act's Annex I applies to embedded audit "
        "logging, mapped point-by-point to what SolidSyslog provides and what "
        "stays yours."
    ),
    "iec62443.md": (
        "IEC 62443-4-2 audit-logging controls mapped control-by-control to "
        "what SolidSyslog provides and what stays yours."
    ),
    "rfc-compliance.md": (
        "Requirement-by-requirement status of SolidSyslog against the syslog "
        "RFCs it implements: what is supported, what is partial, what is planned."
    ),
    "security/threat-model.md": (
        "Where SolidSyslog sits across your trust boundaries, and the assets, "
        "attacker goals and mitigations to fold into your own product's model."
    ),
    "security/at-rest-cryptography.md": (
        "How stored records are sealed: the SecurityPolicy trailer, and what "
        "CRC-16, HMAC-SHA-256 and AES-GCM each give you at rest."
    ),
    "security/sbom.md": (
        "The CycloneDX 1.5 product SBOM published with each SolidSyslog release: "
        "what it covers, how it is generated and signed, and how to consume it."
    ),
    "security/triage-runbook.md": (
        "How a reported SolidSyslog vulnerability is handled end to end, from "
        "draft advisory to coordinated disclosure — the counterpart to SECURITY.md."
    ),
    "security/release-verification.md": (
        "Verify a SolidSyslog release came from this repository: cosign signature "
        "checks, SBOM validation and the source-tree hash, in the order to run them."
    ),
    "security/policy.md": (
        "How to report a SolidSyslog vulnerability, what we commit to in "
        "response, and the disclosure process, scope and supported versions."
    ),
    "license.md": (
        "SolidSyslog is licensed under PolyForm Noncommercial 1.0.0: the full "
        "terms, what noncommercial covers, and where commercial use needs a licence."
    ),
    "support.md": (
        "Where to get help with SolidSyslog — the documentation, questions and "
        "bug reports, reporting a security issue, and commercial support."
    ),
    # API reference doorways
    "api-reference/index.md": (
        "The doorway to the SolidSyslog API: which headers your code includes to "
        "log events, to drain them, and to wire an instance at setup."
    ),
    "core/index.md": (
        "What Core provides: the facade you call, the pipeline that formats and "
        "drains a record, and the role implementations that need no platform."
    ),
    "roles/index.md": (
        "The vtable contracts SolidSyslog composes against, what fills "
        "each one, and the Null fallback that keeps an unfilled role safe."
    ),
    "assets/postit/README.md": (
        "The post-it diagram kit behind the SolidSyslog architecture pictures: "
        "what each colour and arrow means, and how the diagrams are generated."
    ),
    # Maintaining
    "builds.md": (
        "The contributor build doc: the CMake preset catalogue for developing "
        "SolidSyslog and reproducing each CI lane locally."
    ),
    "local-checks.md": (
        "What to run locally before raising a pull request and what to leave to "
        "CI — the tiered budget that keeps the pre-push wait to a few minutes."
    ),
    "bdd.md": (
        "How SolidSyslog is accepted end to end: Gherkin scenarios driving real "
        "targets, with syslog-ng and a QEMU FreeRTOS board as the test oracles."
    ),
    "ci.md": (
        "The GitHub Actions pipeline: what each build, analysis, sanitiser, "
        "coverage, integration and BDD lane checks, and which gate a merge."
    ),
    "containers.md": (
        "The container images the devcontainer and CI lanes use, what each one "
        "adds, and the procedure for bumping one across the repository."
    ),
    "NAMING.md": (
        "The identifier naming rules for SolidSyslog, reconciling MISRA C:2012 "
        "rules 5.1 to 5.9 with readable call sites, tier by tier."
    ),
    "misra-deviations.md": (
        "Every deliberate deviation from the MISRA C:2012 rules SolidSyslog "
        "otherwise enforces, each paired with its cppcheck-misra suppression."
    ),
    "release-process.md": (
        "How a SolidSyslog release is cut: versioning, release-please, the signed "
        "SBOM and source-tree hash, and how a security release differs."
    ),
}


def _src_uri(file):
    return getattr(file, "src_uri", None) or file.src_path.replace(os.sep, "/")


def _hand_written_pages(files):
    return {
        _src_uri(file)
        for file in files
        if file.is_documentation_page() and not _src_uri(file).startswith(GENERATED_PREFIX)
    }


def on_nav(nav, config, files, **kwargs):
    written = _hand_written_pages(files)
    faults = []
    missing = sorted(written - set(DESCRIPTIONS))
    if missing:
        faults.append("no description for " + ", ".join(missing))
    stale = sorted(set(DESCRIPTIONS) - written)
    if stale:
        faults.append("described but no such page: " + ", ".join(stale))
    if faults:
        raise PluginError("hooks/page_descriptions.py: " + "; ".join(faults))
    return nav


def on_page_markdown(markdown, page, config, files, **kwargs):
    description = DESCRIPTIONS.get(_src_uri(page.file))
    # A page that sets its own description in front matter keeps it.
    if description and not page.meta.get("description"):
        page.meta["description"] = description
    return markdown
