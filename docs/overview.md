# Compliance in one page

SolidSyslog is a client-side structured-syslog library that helps you implement
the audit-logging and product-security controls the EU Cyber Resilience Act
(CRA) and IEC 62443 expect of industrial and connected products. You wire
the roles your deployment needs; the library handles the RFC 5424 formatting,
reliable delivery, store-and-forward survival, at-rest record protection, and
evidence metadata that those frameworks expect an audit-logging function to
provide, on any embedded RTOS or bare-metal target (bring your own network
stack, TLS library, and filesystem, or use the shipped reference adapters), and
on POSIX and Windows hosts too.

This page is the evaluator's one-screen orientation. It links out to the
control-by-control detail rather than restating it.

> [!NOTE]
> SolidSyslog is a component, not a product. IEC 62443 certifies systems, and CRA
> obligations fall on the manufacturer placing a product on the market. What follows is
> our account of how the library helps you meet the audit-logging parts of those
> frameworks. It is guidance, not a guarantee of compliance, and no substitute for
> assessment of your finished product.

## Neither framework hands you a parts list

Both state capabilities and leave the realisation to you.

The CRA gates its Annex I product requirements on the manufacturer's own risk
assessment, and applies them "where applicable" — so two conforming products can
implement them very differently. IEC 62443 assigns Security Levels to a system in its
deployment and its assessment, not to a component; the same library can appear in
deployments assessed at different levels.

What your device needs from its audit trail therefore follows from your threat model,
your documented intended purpose and your resources.
[Building up the protection you need](hardening-path.md) walks the capabilities in the
order an integration usually adds them, with the question that decides each one and an
indication of what it costs, so you can see where your own answer lands.

## The two maps

| Framework | What the map covers |
|---|---|
| [CRA](cra.md) | Annex I Part I (2)(l), the requirement that names recording and monitoring internal activity; the Part I points an audit trail contributes to; and what the project publishes for your Part II vulnerability handling |
| [IEC 62443](iec62443.md) | The audit-logging-relevant Component and System Requirements from 62443-4-2 and 62443-3-3, the levels each helps with, and the components that address them |

## Go deeper

- [Building up the protection you need](hardening-path.md): the integration path, and the question that drives each step along it.
- [CRA guide](cra.md): the Annex I map underneath this page.
- [IEC 62443 compliance guide](iec62443.md): the control-by-control map underneath this page.
- [RFC compliance matrix](rfc-compliance.md): sender-side coverage of RFC 5424 / 5426 / 6587 / 5425.
- [Security documentation](README.md#compliance): threat model, at-rest crypto, SBOM, triage, release verification.
- [Adding it to your build](build-integration.md): when you are ready to wire it up.
