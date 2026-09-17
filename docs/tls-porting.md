# Port a TLS stream

How to fill the [Stream](api/structSolidSyslogStream.md) role with a TLS
library the shipped packs do not cover. [Porting](porting.md) covers what every
adapter shares: the instance shape, the static pool, the slot walk and the
build wiring. This page covers what a TLS stream adds to that, and what
[TLS obligations](tls.md) requires of it.

## What a TLS stream is made of

A TLS pack is one Stream adapter and one or more credentials sources, over a
Core module they share.

**The stream** fills the Stream vtable: `Open` connects the transport it was
wired to, brings the library up, asks its credentials source for material,
drives the handshake to a deadline, and refuses or accepts the peer; `Send` and
`Read` move records over the session; `Close` sends `close_notify`, tears the
session down and tells the credentials source it has finished; `Version`
returns the integrator's configuration version. The stream holds wiring only.
Everything a connection is made with arrives through the credentials source and
a profile callback the integrator supplies, so nothing on the stream goes stale
when a deployment changes.

**The credentials role** is the pack's own, because its `Install` takes the
library's context type. It is a vtable of two calls: `Install`, called from
`Open` after the transport connects, puts the trust anchors and the client
credential onto the library's context and reports through a
[`SolidSyslogTlsCredentialsInstalled`](api/SolidSyslogTlsCredentialsInstalled_8h.md)
what the stream cannot read back - whether anchors were installed, and which
fingerprints are pinned; `Release`, called from `Close` exactly once for every
`Install` whatever it returned, frees whatever `Install` acquired. A pack ships
at least one source and a Null one; an integrator with a secure element or a
key store writes another against the same vtable.

**The profile** is a struct the stream zeroes and hands to the integrator's
callback at each `Open`: the expected peer name, and whatever cipher policy the
library lets a caller select, typed to the library. The stream reads it for
that connection and stores nothing.

**The fingerprint module** is Core, in
[`SolidSyslogTlsFingerprint.h`](api/SolidSyslogTlsFingerprint_8h.md), and the
pack calls it at three points: `SolidSyslogTlsFingerprint_ListIsPresent` from
every credentials source's `Create`, so an unreadable pin list is reported
before the first connection; `SolidSyslogTlsFingerprint_InspectList` from
`Open` before the handshake, so a malformed pin refuses the connection and a
`sha-1` pin warns; and `SolidSyslogTlsFingerprint_Authorise` from the library's
verify callback, with a digest callback the pack supplies over the peer's DER
certificate. Parsing, comparison and the walk over the list are Core's; the
pack only produces a digest and acts on the verdict.

**The codes** are portable. A TLS stream reports
[`SolidSyslogTlsStreamErrors`](api/SolidSyslogTlsStreamErrors_8h.md) under the
categories in
[`SolidSyslogTlsStreamCategories.h`](api/SolidSyslogTlsStreamCategories_8h.md),
and a credentials source reports
[`SolidSyslogTlsCredentialsErrors`](api/SolidSyslogTlsCredentialsErrors_8h.md),
so a handler written against one pack keeps working on another. The pack's own
`*Errors.h` declares an `ErrorSource` per class and nothing else. A code the
pack cannot raise is left unraised and listed as such on the platform page.

## What the stream enforces, and what it leaves to the library

The library validates the chain, checks the dates and matches the name. The
stream sets the floor, installs the verify callback, and decides what a
fingerprint waives: the chain-trust objection alone, and never validity or the
name. Where both anchors and pins are configured, both must pass. Where pins
alone are configured, the library has no anchor to validate against, and the
stream is the enforcement point for everything the library would otherwise have
refused on. How a given library behaves in that mode decides where the refusal
has to be made; find out before the client credential is sent, not after.

The refusal names one check, in the order the contract fixes under
[Report every one of these](tls.md#report-every-one-of-these-and-name-the-check-that-failed).
A library reports its verdict in its own way - one code, or accumulated flags -
and it may no longer carry the reason by the time the stream reads it. Record
what the callback decided as it decides it; do not deduce it afterwards.

## The obligations, as a checklist

Each is stated in full under [TLS obligations](tls.md). Against the code:

- Set the minimum version to TLS 1.2 on every context you build. Set no
  maximum.
- Refuse at `Open`, with `NO_PEER_AUTHORISATION`, when `Install` reports
  neither anchors nor pins. Never fall back to a system trust store.
- Accept a peer by fingerprint alone, with no chain. Do not let a pin waive
  expiry, not-yet-valid or a name mismatch.
- Verify `ServerName` where one is given, send it as SNI, treat `""` as an
  explicit opt-out, and report `SERVER_NAME_NOT_SET` for NULL only where no pin
  is configured.
- Obtain material through `Install` at every `Open`, and answer every `Install`
  with one `Release` at `Close`, whatever `Install` returned. Build the library
  context afresh per connection; hold nothing between them.
- Report every client-credential fault at `WARNING` and continue
  server-authenticated. Fail the connection on a fault in what authorises the
  peer.
- Pass the profile's cipher policy through unchanged, for every version that
  can be negotiated.
- Do not resume sessions.
- Do not perform or require revocation checking.
- Bound the handshake with the deadline from `GetHandshakeTimeoutMs`, sleeping
  through the injected `Sleep` between polls, and report `HANDSHAKE_TIMEOUT`.
- Attempt `close_notify` once in `Close`, without blocking.
- Check wiring at `Create` and return the Null stream on a NULL. Check material
  at `Open`.
- Refuse a peer that presented no certificate, whatever the library
  negotiated.
- Send nothing of the client's to a peer the stream is about to refuse.

## What to prove

Unit tests against a fake of the library, so every branch of `Open` and every
report site is reached without a network. Integration tests against the real
library and a real server, one per row of the decision table: each
anchor-and-pin combination against a clean, untrusted, expired, misnamed and
mismatched peer, plus a rotation of each kind of material. The
[BDD matrix](bdd.md) then exercises the pack on every target it builds for.
Break the production code and watch each test fail before trusting it; a test
of a refusal passes for several reasons, and only one of them is the right one.

## What to write down

The platform page states what the pack needs, which codes it never raises and
why, and where it falls short of the contract, with an issue for each
shortfall. The setup page walks a user from the library to a verified
connection. Neither names another platform.
