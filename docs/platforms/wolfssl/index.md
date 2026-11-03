# wolfSSL

`Platform/WolfSsl/` wraps [wolfSSL](https://www.wolfssl.com/) for keyed at-rest
cryptography on embedded targets, filling the
[SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role over
wolfCrypt. The [Stream](../../api/structSolidSyslogStream.md) role, TLS over an
injected transport, is outstanding under
[S34.02](https://github.com/cososo-ltd/solid-syslog/issues/1121); until it lands
the pack carries no TLS and the capability matrix says so.

This page covers what this adapter needs, which of your wolfSSL build's choices
it depends on, and the coexistence guarantee it makes.

## What it ships

<!-- Filled at build time from this pack's Interface directory; empty here by
     design. See hooks/platform_backlinks.py. -->

## Requirements

The adapter sources compile in your target against your own wolfSSL build. A
`./configure` build describes itself in `wolfssl/options.h`, and the adapter
includes it; a `user_settings.h` build defines `WOLFSSL_USER_SETTINGS`, and the
adapter reads that file through wolfSSL's own `settings.h` instead. Either way
the features your build enables are the features it gets, and the adapter asks
for no configure flag.

wolfSSL 5.x. The pack is tested against 5.9.4 twice in CI: once against a build
with the options it asks for, and once against a bare `./configure`, so it is
known to compile, link and work on the build an integrator has rather than only
on the one it prefers.

wolfSSL is dual-licensed, GPLv3 or commercial, since 5.8.2; earlier releases
were GPLv2. SolidSyslog links your build and ships none of it, so the licence
choice stays with you.

An initialised `WC_RNG` is required by the AES-GCM policy, for its nonces. The
HMAC policy needs none. wolfCrypt seeds the RNG from your build's seed source
when you initialise it, so seeding is not something this adapter can get wrong
or check.

## What it depends on in your build

- `HAVE_AESGCM`, on by default, for the AES-GCM policy. A build without it has
  no `wc_AesGcmEncrypt` to link.
- `wc_ForceZero`, which wipes key material after every operation. A build that
  defines `WOLFSSL_NO_FORCE_ZERO` supplies its own, as wolfSSL documents.

Neither policy reaches a feature a bare build lacks.

One build is refused rather than used. `--enable-afalg=xilinx` and
`--enable-afalg=xilinx-aes` define `WOLFSSL_AFALG_XILINX_AES`, which routes
AES-GCM through the Linux AF_ALG interface to the Zynq accelerator. On that port
`wc_AesGcmEncrypt` and `wc_AesGcmDecrypt` read and write `WC_AES_BLOCK_SIZE`
bytes beyond the body they are given, as wolfSSL's API documentation for the two
functions states. The policy encrypts the record body in place, and the bytes
after it are the record's nonce and tag, so the port would overwrite them during
every seal. The pack therefore withholds
`SolidSyslogWolfSslAesGcmPolicy_GcmEncrypt` and
`SolidSyslogWolfSslAesGcmPolicy_GcmDecrypt` on that build, and the link fails
naming them, rather than sealing a record that could never be opened.

The padded versions are not written because there is no board here to run them
on. An integrator who has one supplies those two functions, declared in
`SolidSyslogWolfSslAesGcmPolicyPrivate.h`: copy the body into a buffer with
`WC_AES_BLOCK_SIZE` bytes of room after it, aligned to `WOLFSSL_XILINX_ALIGN`,
make the wolfCrypt call on that buffer with the nonce, tag and header arguments
the shipped versions pass, and copy the body back on success. The HMAC policy is
unaffected: the port takes nothing but AES.

## Coexistence is an auditable contract

`Platform/WolfSsl/Source/` calls no process-global wolfSSL API. It does not call
`wolfSSL_Init` or `wolfSSL_Cleanup`, install allocator or logging hooks, or
turn debugging on. Every wolfCrypt context it uses is built for one operation
and freed after it. A device that already uses wolfSSL for firmware update or a
vendor cloud SDK keeps that configuration intact, and the claim can be checked
against the directory.

## What it reports

Every class in this pack reports portable detail codes, so a handler written
against them keeps working if the crypto backend underneath changes. What stays
specific to this pack is `event->Source`, which names the class that reported.

The at-rest policies raise every code their roles define. The
[setup page](setup.md#what-the-handler-sees) lists them.
