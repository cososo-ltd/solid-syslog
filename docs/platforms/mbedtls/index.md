# Mbed TLS

`Platform/MbedTls/` wraps [Mbed TLS](https://mbed-tls.readthedocs.io/) for TLS
transport and keyed at-rest cryptography on embedded targets. It fills the
[Stream](../../api/structSolidSyslogStream.md) role with TLS and the
[SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role for at-rest
integrity and confidentiality.

What a TLS stream must do is the same whichever library provides it, and is
stated once under [TLS obligations](../../tls.md). This page covers what this
adapter needs, how credentials reach it, the coexistence guarantee it makes, and
where it does not yet meet that contract.

## What it ships

## Requirements

The adapter sources compile in your target against your own `mbedtls_config.h`,
so the features you enable are the features it gets.

A `SolidSyslogSleepFunction` is required and has no default.

## Credentials come from a credentials source

Where trust anchors, pinned peer fingerprints and the mutual-TLS client
credential come from is the integrator's choice rather than this adapter's. The
stream is wired to a `SolidSyslogMbedTlsCredentials`, asked once per connection
to install its material on the `mbedtls_ssl_config` and told once per connection
when that material is no longer needed. A source backed by a security element, a
PSA opaque key or an encrypted store is a class implementing that role, and needs
no change here.

Two sources ship with the pack, and neither opens a file - which is what allows
the adapter to run on targets built without `MBEDTLS_FS_IO`.

`SolidSyslogMbedTlsHandleCredentials` carries caller-built, caller-owned handles:
an `mbedtls_x509_crt` trust chain, and for mutual TLS an `mbedtls_x509_crt` and
`mbedtls_pk_context` pair. The integrator parses the material and keeps it
parsed.

`SolidSyslogMbedTlsPemBufferCredentials` carries PEM held in memory and parses it
per connection. Between connections nothing but the integrator's own PEM is in
memory, which may be read-only flash. What Release frees, Mbed TLS wipes:
`mbedtls_pk_free` zeroises the key context and every limb of the private key,
and `mbedtls_x509_crt_free` zeroises the DER it decoded. This library copies the
PEM nowhere, so it holds no second copy to wipe.

The credential window is explicit. Install is called after the transport
connects; Release answers every Install, once, after the `ssl_config` has been
freed and with it every pointer into the material. That window is what a source
reaching a secure element or an encrypted store needs, and the PEM-buffer source
is the worked example of using it.

Because it parses into storage of its own, one PEM-buffer source serves one
connection at a time. Wire a second instance for a second stream: a stream
opening while another still holds the source is reported and its attempt fails,
rather than parsing over material the live connection is using, and its sender
connects on a later pass once the first stream has closed.

With the PEM-buffer source, replacing the buffer and moving the stream's
configuration version is enough - nothing is freed, and the next connection
parses whatever the buffer then points at.

The handle source is different, because rotating it means freeing material the
open connection is still reading. Moving the version applies the change but does
not say when the old handle stops being read, so the free and the re-parse belong
after the connection has closed: either call `SolidSyslogSender_Disconnect` from
the task that services the library and re-parse once it returns, or put the free
and the re-parse in a credentials source's `Release`, which the stream calls when
it has finished with the material.

A `Release` reached by destroying the stream runs inside the configuration lock,
because pool cleanup holds it. Freeing and re-parsing your own material there is
fine; creating or destroying any SolidSyslog object from `Release` is not, since
that takes the same lock and a non-recursive one deadlocks.

## Coexistence is an auditable contract

`Platform/MbedTls/Source/` calls no process-global Mbed TLS API. It does not call
`mbedtls_platform_setup` or `mbedtls_platform_teardown`, install threading-alt
hooks, call `psa_crypto_init`, reset the global random number generator, or
replace a debug callback. TLS policy is applied per `ssl_config`, so it cannot
affect the ones you build elsewhere. A device that already uses Mbed TLS for
firmware update or a vendor cloud SDK keeps that configuration intact, and the
claim can be checked against the directory.

## What it reports

Every class in this pack reports portable detail codes, so a handler written
against them keeps working if the crypto backend underneath changes. What stays
specific to this pack is `event->Source`, which names the class that reported.

Each role's codes are the whole vocabulary that role can express, so some
describe faults this pack cannot have and it never raises them.

From the TLS-stream codes it does not raise `CONTEXT_INIT_FAILED`. There is no
separate context to build here - the configuration is brought up from a library
preset, and a failure at that point is `DEFAULTS_NOT_APPLIED`.

From the credentials codes it does not raise `TRUST_ANCHORS_NOT_LOADED`. Neither
shipped source loads anchors from anywhere: one is handed material the integrator
has already parsed, and the other parses a buffer and reports
`TRUST_ANCHORS_NOT_PARSED` if that fails. Which of the two you wire decides what
else you can see - the parsing source alone can raise `PEM_NOT_TERMINATED`,
`TRUST_ANCHORS_NOT_PARSED` and `CLIENT_CREDENTIAL_NOT_PARSED`, because it is the
only one doing the parsing.

The at-rest policies raise every code their roles define.

## What a connection is made with

The stream asks for a profile once per connection, and takes the expected peer
name and the ciphersuite policy from it. Nothing is stored between connections,
so a change is a matter of returning something different and moving the stream's
version.

The ciphersuite policy is one list covering both TLS versions, given as a
0-terminated array of IANA identifiers - the `MBEDTLS_TLS_*` and
`MBEDTLS_TLS1_3_*` macros. Mbed TLS does not copy it, so the array must stay
valid for as long as the connection. Leave it unset and every ciphersuite the
build enables is offered, which on a trimmed `mbedtls_config.h` is whatever was
compiled in rather than a curated set; naming a policy is how that becomes a
decision rather than a side effect of the build.

Setting the list cannot fail here - `mbedtls_ssl_conf_ciphersuites` returns
nothing - so a policy naming only suites the build does not carry surfaces as a
refused handshake rather than as a configuration error.

Key-exchange groups and signature algorithms are not selectable here. TLS 1.3
moved both out of the ciphersuite, so a policy naming a curve has nowhere to go
yet.

## Certificate validity depends on your build carrying a clock

`MBEDTLS_HAVE_TIME_DATE` is what makes Mbed TLS check the dates on a
certificate. Without it `mbedtls_x509_time_is_past` and
`mbedtls_x509_time_is_future` compile to `return 0`, the expired and not-yet-valid
flags are never set, and a certificate outside its validity period is accepted -
silently, because nothing failed. On a board with no real-time clock it is
tempting to leave the macro off for exactly that reason, and doing so gives up
the last time-based control the contract has.

Defining it obliges the build to satisfy three separate contracts, and a
bare-metal target satisfies each one differently:

| Contract | Supplied by | Where a bare-metal target has to step in |
|---|---|---|
| Wall clock, from `MBEDTLS_HAVE_TIME` | `mbedtls_time`, by default libc `time()` | `MBEDTLS_PLATFORM_TIME_ALT`, then install a source with `mbedtls_platform_set_time` - a target with no syscall behind `time()` needs this |
| Calendar conversion, from `MBEDTLS_HAVE_TIME_DATE` | `mbedtls_platform_gmtime_r`, by default `gmtime_r` or `gmtime_s` | `MBEDTLS_PLATFORM_GMTIME_R_ALT`, then supply the function - a libc offering neither needs this |
| Monotonic milliseconds, also from `MBEDTLS_HAVE_TIME` | `mbedtls_ms_time` | `MBEDTLS_PLATFORM_MS_TIME_ALT`, then supply it from a tick counter - every implementation Mbed TLS ships needs a hosted operating system underneath it, so a bare-metal build has no default to fall back on |

Only the first two bear on certificates. The millisecond hook is a separate
obligation that comes along with `MBEDTLS_HAVE_TIME` and has nothing to do with
validity; it is listed because the build will not link without it.

A coarse clock is enough. X.509 asks only which side of a window the device is
on, so a time fed by SNTP, or seeded at provisioning and advanced by an uptime
counter, answers the question a battery-backed RTC would. Time for certificates
is also independent of the timestamp a record carries: leaving
`SolidSyslogConfig.Clock` unset still emits `NILVALUE` in the message.
