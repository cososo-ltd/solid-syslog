# OpenSSL

`Platform/OpenSsl/` wraps [OpenSSL](https://docs.openssl.org/) for TLS transport
and keyed at-rest cryptography on hosted targets. It fills the
[Stream](../../api/structSolidSyslogStream.md) role with TLS and the
[SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role for at-rest
integrity and confidentiality.

What a TLS stream must do is the same whichever library provides it, and is
stated once under [TLS obligations](../../tls.md). This page covers what this
adapter needs, how credentials reach it, and how it meets that contract.

## What it ships

<!-- Filled at build time from this pack's Interface directory; empty here by
     design. See hooks/platform_backlinks.py. -->

## Requirements

OpenSSL 3.0 or later. The CMake configure fails below that rather than the build,
so an older libssl is caught before anything compiles.

A `SolidSyslogSleepFunction` is required and has no default.

## Credentials come from a credentials source

Where trust anchors, pinned peer fingerprints and the mutual-TLS client
credential come from is the integrator's choice rather than this adapter's. The
stream is wired to a `SolidSyslogOpenSslCredentials`, asked once per connection
to install its material on the `SSL_CTX` and told once per connection when that
material is no longer needed. A source backed by a hardware key store, a
keyring or an encrypted store is a class implementing that role, and needs no
change here.

One source ships with the pack: `SolidSyslogOpenSslPemFileCredentials`, which
names its material by file path. It performs no file handling of its own -
the paths go to OpenSSL, which opens and parses them, so PEM bytes never pass
through this library. The key must not be encrypted: a passphrase is never
prompted for, so an encrypted key fails to load and is reported as
`CLIENT_CREDENTIAL_NOT_INSTALLED`.

The `SSL_CTX` is rebuilt on every open and freed on close, and the credentials
source is asked again each time. Nothing is held between connections. Rotation
is therefore a replacement and a reconnection: put the new material in place,
and it is in force on the next connection, either through ordinary reconnection
after an outage or immediately by moving the stream's configuration version.
Nothing has to be freed to rotate the shipped source, which names a path that
OpenSSL reads afresh on each connection, so the version is the whole of it.

## What it reports

Every class in this pack reports portable detail codes, so a handler written
against them keeps working if the crypto backend underneath changes. What stays
specific to this pack is `event->Source`, which names the class that reported.

Each role's codes are the whole vocabulary that role can express, so some
describe faults this pack cannot have and it never raises them.

From the TLS-stream codes it does not raise `DEFAULTS_NOT_APPLIED`, because
nothing here applies a library preset, or `NULL_RNG`, because OpenSSL carries its
own entropy source and the configuration asks for none. Nor
`LIBRARY_OUT_OF_MEMORY`: OpenSSL's handshake raises an allocation failure as the
same generic error its other crypto failures use, so this pack cannot tell one
from the other and does not guess.

From the credentials codes it does not raise `NULL_RNG` for the same reason, nor
`PEM_NOT_TERMINATED`, `TRUST_ANCHORS_NOT_PARSED` or `CLIENT_CREDENTIAL_NOT_PARSED`:
the shipped source names a path and hands it to OpenSSL, which reads and parses
the file itself, so a failure there arrives as `TRUST_ANCHORS_NOT_LOADED` or
`CLIENT_CREDENTIAL_NOT_INSTALLED` rather than as a parse of its own. Nor
`ALREADY_IN_USE`: the shipped source holds nothing between connections, so two
streams may share one.

The at-rest policies raise every code their roles define.

## What a connection is made with

The stream asks for a profile once per connection, and takes the expected peer
name and the cipher policy from it. Nothing is stored between connections, so a
change is a matter of returning something different and moving the stream's
version.

Both of OpenSSL's cipher lists are selectable, because it keeps two: one governs
TLS 1.2 and below, the other TLS 1.3, and since no protocol ceiling is pinned the
second is usually the one in force. Leave either unset and OpenSSL's own default
stands - for TLS 1.3 that is the suite RFC 8446 makes mandatory plus the two it
recommends. A list OpenSSL rejects fails `Open` before any handshake and is
reported as `CIPHER_POLICY_REJECTED`, rather than falling back. An empty
`CipherSuites` is not rejected: it is OpenSSL's way of turning TLS 1.3 off.

The security level and the minimum protocol version are floors, not settings:
level 2 and TLS 1.2. The stream reads what the context holds and raises only a
weaker value, so a stricter one is kept. The minimum protocol comes from the
build and `openssl.cnf`. The security level comes from those too, unless the
`CipherList` carries `@SECLEVEL=n`: that overrides the host's level, higher or
lower, as it does anywhere in OpenSSL. The stream reads the level once the
policy is applied, so a list can move it between levels but never below 2.

Key-exchange groups and signature algorithms are not selectable here. TLS 1.3
moved both out of the ciphersuite, so a policy naming a curve has nowhere to go
yet.

Each connection is a fresh session with none supplied to resume, and the stream
never writes early data, so it meets the contract's resumption and early-data
obligations by construction. It refuses a request from the collector to
renegotiate.

An expected identity given as an address literal is matched against the
certificate's `iPAddress` entries only, never against a DNS name or Common Name
spelling the same digits. The stream refuses a TLS 1.2 peer that does not
acknowledge secure renegotiation, and sets that itself rather than relying on
OpenSSL's default, so neither `UnsafeLegacyServerConnect` nor
`UnsafeLegacyRenegotiation` in `openssl.cnf` turns it off.

A send first reads what the collector has sent, so that a session the collector
has closed refuses the record rather than taking it, as the
[Stream](../../api/structSolidSyslogStream.md) contract requires. A write alone
would not notice: OpenSSL takes the record into the session regardless of the
close_notify waiting unread. When the read meets the close, or the transport
beneath has gone, the send fails, the stream closes itself, and your store
replays the record on the next connection. Syslog over TLS carries nothing from
collector to device, so the read normally finds nothing; application data it
does meet is discarded, not kept for a later read, and the probe makes at most
eight reads of up to 16 bytes each, so a close behind more than that goes
unnoticed.

## Where it falls short of the contract

Every obligation under [TLS obligations](../../tls.md) is met by this pack as
shipped.
