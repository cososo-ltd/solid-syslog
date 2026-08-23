# Mbed TLS

`Platform/MbedTls/` wraps [Mbed TLS](https://mbed-tls.readthedocs.io/) for TLS
transport and keyed at-rest cryptography on embedded targets. It fills the
[Stream](../../api/structSolidSyslogStream.md) role with TLS and the
[SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role for at-rest
integrity and confidentiality.

What a TLS stream must do is the same whichever library provides it, and is
stated once under [TLS obligations](../../tls.md). This page covers what this
adapter needs, the coexistence guarantee it makes, and where it does not yet meet
that contract.

## What it ships

## Requirements

The adapter sources compile in your target against your own `mbedtls_config.h`,
so the features you enable are the features it gets.

A `SolidSyslogSleepFunction` is required and has no default.

## Credentials are handles, not paths

Credentials are passed as caller-built, caller-owned handles: a seeded
`mbedtls_ctr_drbg_context` for the handshake, an `mbedtls_x509_crt` trust chain,
and for mutual TLS an `mbedtls_x509_crt` and `mbedtls_pk_context` pair. No part
of the adapter opens a file, which is what allows it to run on targets built
without `MBEDTLS_FS_IO`.

Two lifetimes are in play and they are not the same. The **handle objects** must
stay addressable for as long as the stream might open a connection, because the
adapter reads the pointers it was given on every connect. The **parsed material
inside them** only has to be intact while a connection is open, which is when the
adapter's `ssl_config` holds pointers into it.

Rotation follows from the second lifetime. Call `SolidSyslogSender_Disconnect`,
which releases the `ssl_config` and with it every pointer into the material, then
free and re-parse into the same handle. The next send reconnects with the new
material. Freeing before the disconnect completes is a use-after-free, because
the open connection is still reading it.

The adapter does not say when it has finished with the material, so an integrator
who wants the private key out of RAM between connections has to drive that
sequence themselves rather than being told. That is the gap recorded below.

## Coexistence is an auditable contract

`Platform/MbedTls/Source/` calls no process-global Mbed TLS API. It does not call
`mbedtls_platform_setup` or `mbedtls_platform_teardown`, install threading-alt
hooks, call `psa_crypto_init`, reset the global random number generator, or
replace a debug callback. TLS policy is applied per `ssl_config`, so it cannot
affect the ones you build elsewhere. A device that already uses Mbed TLS for
firmware update or a vendor cloud SDK keeps that configuration intact, and the
claim can be checked against the directory.

## Where it differs from the contract

Four differences, each tracked. Read them before relying on the corresponding
obligation.

### A peer cannot be authorised by certificate fingerprint

Only certification path validation is offered, so a deployment with no PKI has no
way to pin the collector's certificate. Tracked as
[#753](https://github.com/cososo-ltd/solid-syslog/issues/753).

### Credential material must stay parsed for the life of the stream

The adapter binds the handles into its `ssl_config` on each connection and drops
them on close, but it never says so, so every handle has to remain valid and
parsed for as long as the stream exists. A device that connects rarely still
holds its private key in RAM continuously, and there is no point at which the
adapter invites the integrator to release it. Tracked under
[E39](https://github.com/cososo-ltd/solid-syslog/issues/782).

### The cipher policy cannot be expressed

The configuration carries no cipher or ciphersuite field, so the ciphersuites
your `mbedtls_config.h` enables, filtered by the preset, are what gets
negotiated. The contract asks for an integrator's policy to be passed through
where the library allows one to be selected. Tracked as
[#733](https://github.com/cososo-ltd/solid-syslog/issues/733).

### A missing trust chain is not reported as a configuration fault

`mbedtls_ssl_conf_ca_chain` returns no status, so a configuration carrying no
trust anchors is accepted, and the fault surfaces only once the peer's
certificate finds nothing to chain to. What is reported is an untrusted peer
rather than the missing trust material. Tracked
as [#753](https://github.com/cososo-ltd/solid-syslog/issues/753), which is where
a peer authorised by fingerprint instead of by trust anchor is settled.
