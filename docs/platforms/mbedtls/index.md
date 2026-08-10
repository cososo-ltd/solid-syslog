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
without `MBEDTLS_FS_IO`. Each handle must remain valid for the lifetime of the
stream.

Rotation follows from that. Because the adapter consumes handles it did not
build, refreshing credentials means parsing the new material and recreating the
stream, or the `SolidSyslogStreamSender` above it, so the next connection uses
them. There is no reload callback and none is needed.

## Coexistence is an auditable contract

`Platform/MbedTls/Source/` calls no process-global Mbed TLS API. It does not call
`mbedtls_platform_setup` or `mbedtls_platform_teardown`, install threading-alt
hooks, call `psa_crypto_init`, reset the global random number generator, or
replace a debug callback. TLS policy is applied per `ssl_config`, so it cannot
affect the ones you build elsewhere. A device that already uses Mbed TLS for
firmware update or a vendor cloud SDK keeps that configuration intact, and the
claim can be checked against the directory.

## Where it differs from the contract

Five differences at 0.1.0, each tracked. Read them before relying on the
corresponding obligation.

### A half-supplied client credential is accepted in silence

A client certificate is presented only when both `ClientCertChain` and
`ClientKey` are supplied. Where either is absent the other is ignored, the
connection proceeds with server-authenticated TLS, and nothing is reported — so a
device configured for mutual TLS can run without presenting its certificate, and
without anyone on the device knowing. The contract requires this to be reported.
Until it is, check for a half-supplied pair before you open the stream. Tracked
as `#718`.

### The key is not checked against its certificate

No local check confirms that `ClientKey` matches `ClientCertChain`, and a failure
to install the pair is not reported either. A mismatch therefore surfaces as a
handshake rejection from the collector rather than as a setup error on the
device, which sends you looking in the wrong place. Tracked as `#719`.

### An expired certificate stops delivery

A peer certificate that is expired or not yet valid fails the handshake, even
where it still chains to a trusted anchor. The contract asks for it to be
reported with delivery continuing, because clock skew is the dominant cause and a
device with a wrong clock is one whose logs you still want. Tracked as `#731`.

### The cipher policy cannot be expressed

The configuration carries no cipher or ciphersuite field, so the ciphersuites
your `mbedtls_config.h` enables, filtered by the preset, are what gets
negotiated. The contract asks for an integrator's policy to be passed through
where the library allows one to be selected. Tracked as `#733`.

### The configuration is not checked when the stream is created

A configuration missing something the stream cannot work without is accepted, and
the fault appears on the first connection attempt rather than at setup. The
random source and the trust chain are installed through calls that return no
status, so a missing one becomes a handshake failure rather than the
configuration error it is. Tracked as `#732`.
