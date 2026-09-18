# TLS obligations

What a TLS `Stream` must do, whichever library provides it. Read this if you are
choosing between the TLS platforms, assessing the library against a security
standard, or [writing a TLS `Stream` of your own](tls-porting.md).

The obligations here are the contract. What each shipped TLS platform does, and
where it differs, is on its own page; the
[capability matrix](platforms/index.md) shows which platforms fill the role.

## Delivery is preferred to silence

A control that fails closed stops reporting at the moment it matters most: a
device under attack. An attacker who can block the route to a revocation
responder, or make a credential look half-configured, must not be able to stop
the device reporting. So the default is: **report the fault through the error
handler, and keep delivering.**

The rule has a limit: **delivery stops when the peer fails the check the
integrator asked for.** That covers a configuration naming neither trust anchors
nor pinned fingerprints, a certificate that does not validate against the trust
anchors, a certificate matching none of the configured fingerprints, and a
certificate that does not match a declared identity. Continuing through any of
them would send the records to an unverified peer, with nothing to indicate it.

The check is the integrator's to set. Declaring no peer identity is a decision
rather than a failure: it states that chain verification alone is sufficient,
which on a closed network with a private CA it may be. The contract requires
only that the decision is explicit, and that the stream reports when it was
never made at all.

Every other fault leaves the peer still passing the checks the integrator
configured. Those are reported while delivery continues.

Where a store is configured, blocked delivery is delayed delivery: records
accumulate and replay on the next successful connection. A store is finite, and
records replayed later raise no alert at the time, so this bounds the cost of
the exception without removing the reason for the rule.

## The obligations

### Pin the protocol floor and the cryptographic floor

A TLS `Stream` sets its own minimum protocol version rather than inheriting
whatever the TLS library was built to permit, so downgrade resistance is a
property of this library. The floor is TLS 1.2.

No ceiling is required, and setting one would be wrong. RFC 9662, which updates
RFC 5425, asks that TLS 1.3 be supported and preferred where it is implemented;
BCP 195 §3.1.1 says the same for TLS generally.

The cryptographic floor is BCP 195 §4.5: RSA and finite-field Diffie-Hellman of
2048 bits, elliptic curves of 224 bits, and no SHA-1 or MD5 signatures. A
`Stream` pins it where the library lets a caller set it, after any cipher
policy the integrator supplied, so a policy cannot lower it.

### Require a trust anchor or a pinned fingerprint

A peer is authorised in one of two ways, and a `Stream` requires at least one.
Configured with neither, it refuses to connect. That is reported when a
connection is attempted rather than when the stream is created, because trust
anchors are obtained per connection; the timing rule is under *Check the
configuration it cannot work without*.

**Certification path validation.** The peer certificate must chain to trust
anchors the integrator supplies. There is no fallback to a system trust store:
an embedded target may not have one, and on a host that store is a far larger
trust base than a device reporting to one collector needs. Validity dates are
part of this check: RFC 5280 §6.1 makes the validity period an input to path
validation, so a certificate outside its dates does not chain.

**A pinned certificate fingerprint.** Covered below. A certificate matched by
fingerprint needs no chain: RFC 5425 §4.2.1 states that such a certificate "can
be self-signed, and no certification path validation is needed".

Where both are configured, the peer must satisfy both. That is this contract's
choice, not a requirement of RFC 5425: §6.1 recommends that both endpoints be
authenticated and authorised by one of §5.1 or §5.2, rather than that the two
be combined. Requiring both is the safer reading of a configuration that
supplies both.

### Accept a peer authorised by certificate fingerprint

RFC 5425 §5.1 requires that a peer can be authorised by its certificate
fingerprint, not only by a chain and a name. A fingerprint pins one
certificate, which suits a closed network with no PKI.

The form is RFC 5425 §4.2.2: an IANA hash label, a colon, then the hash of the
DER-encoded certificate as colon-separated hex pairs. §4.2.2 publishes the pairs
in upper case; a `Stream` accepts either case, because a pin reaches a device
through an engineer transcribing it. Labels are from the IANA
[Hash Function Textual Names](https://www.iana.org/assignments/hash-function-text-names/hash-function-text-names.xhtml)
registry, hyphenated: `sha-256` and `sha-1`. Both are accepted. §4.2.2 makes
SHA-1 mandatory to support; `sha-256` is the one to configure where the
collector offers a choice, and a `sha-1` pin is reported on every connection
that uses one.

A **list** of fingerprints is accepted, and any one of them authorises the
peer. A fingerprint covers the whole certificate, so it changes on every
renewal; pinning the old and the new together is what lets a fleet cross a
renewal without every device stopping at once. A pin naming a hash the build
cannot compute is skipped rather than stopping the walk, and reported as such
where nothing else matched.

Where fingerprints are configured and the peer's certificate matches none of
them, the connection stops, whatever the chain says.

**A pin does not extend the validity period.** A pinned certificate outside its
dates is refused, in fingerprint-only mode as in any other. The pin establishes
which peer this is; the validity period establishes whether the issuer still
stands behind its certificate.

**Pinning makes the collector's expiry a fleet-wide event.** Every device pinned
to a certificate stops at the same `notAfter`, and a device that missed the pin
update stays stopped. Where the deployment controls its own CA, RFC 5280
§4.1.2.5 provides for a certificate with no well-defined expiry,
`99991231235959Z`, and gives an embedded device as its example. Otherwise pin
the next certificate alongside the current one before the renewal. A configured
store turns a missed renewal into delayed delivery, up to the point the store
fills.

### Treat endpoint identity as declared, not assumed

The integrator declares the peer identity they expect. A `Stream` verifies it
when one is declared, accepts an explicit decision not to check a name, and
reports when nothing was declared - because that last case is a peer that is
chain-verified but otherwise unidentified, which an attacker holding any
certificate from a trusted CA can exploit. BCP 195 §7.1: without the name
check, TLS proves the certificate is valid and that the peer holds its key, but
not that the intended endpoint was reached.

A configured fingerprint is itself a declaration of identity, and a stronger
one than a name. So a `Stream` given fingerprints and no name does not report
an unidentified peer.

How a name is matched:

- A DNS name is matched against the certificate's `dNSName` entries. Where the
  certificate carries no subject alternative name at all, the Common Name is
  used, as RFC 5425 §5.2 recommends. RFC 9525 §4.1 forbids the Common Name;
  a deployment that wants that stricter rule issues collector certificates with
  a subject alternative name, which is then the only thing matched.
- A wildcard matches only as the whole of the left-most label, as RFC 5425 §5.2
  and RFC 9525 §6.3 require.
- An address literal is matched against the certificate's `iPAddress` entries.
  Whether the library also matches it against a DNS name spelling the same
  digits is on each platform page.
- The declared name is also sent as the server name indication, including an
  address literal, which RFC 6066 §3 does not permit there. A collector that
  refuses such an indication needs a DNS name.

The states a declaration can take are documented on each platform's profile
field.

### Obtain credentials per connection, and announce when they are released

Trust anchors, the client credential and the expected peer identity are
obtained when a connection is made, and the `Stream` says when it has finished
with them. Where credentials come from is the integrator's choice: a file, a
handle the integrator built, a secure element, an encrypted store. Each
platform page documents what it offers.

**A device issued new credentials while it is running uses them on its next
connection** without a restart. Moving the stream's configuration version
makes that immediate.

**The window in which a credentials source must keep material intact is the
connection**, not the lifetime of the stream: the `Stream` asks its source for
the material at `Open` and tells it at `Close` that it has finished. That is a
call into the source rather than a callback to the application, so an
integrator who writes their own source can key custody off it. A source this
library ships states its own lifetime requirement on its platform page.

**It does not follow that the material is out of RAM for most of the time.**
That depends on how long a connection lasts, covered below, and on the source:
one that hands over a pointer to material already parsed has nothing to
release, one that parses on demand and wipes on release does. The TLS library
itself holds the parsed certificate and key for the duration of the session,
the private key included, because clearing them mid-session would break it.

The expected identity travels with the destination. Where the destination can
change at runtime, redirecting a device to a different collector must carry the
identity its certificate is checked against, or the redirection moves the
device to an unverified peer.

### Apply a change by moving a version, not by reaching into the connection

Every `Stream` reports a configuration version, and the sender reads it on
every record. Moving that version is how a change to the material, the expected
peer or the cipher policy is applied: the sender closes the connection on its
next pass and opens a new one, so the change is in force from the following
record.

This is the only lever an integrator needs from outside the task that services
the library. A version is a value the integrator owns, read back at a point of
the library's choosing; `SolidSyslogSender_Disconnect` touches the sender's own
connection state and belongs to the servicing task.

A stream whose configuration cannot change at runtime reports one version for
its lifetime.

A version moved from another task says nothing about when the connection
actually closed, so material the integrator must free rather than overwrite is
still governed by the release announcement above.

### A connection is long-lived, and the integrator bounds it

A `Stream` opens on the first record that needs it and stays open. It closes
when a send fails, when the destination's version moves, when the stream's own
version moves, when the integrator calls `SolidSyslogSender_Disconnect`, or
when the stream is destroyed. There is no idle timeout and no maximum lifetime:
a syslog client that reconnects on a timer costs a handshake each time and
gains nothing for a device that logs steadily.

The consequence: **on a device that logs continuously, one connection may last
for the device's uptime, and the credential material stays resident for all of
it.** The private key is needed once, to sign during the handshake, and the TLS
library retains it for the session.

Bounding that window is the integrator's. A deployment that wants the material
resident for minutes rather than months moves the configuration version on its
own schedule. The same lever serves RFC 5425 §4.4's requirement that a sender
close a connection it does not expect to carry more messages. Where the key
must not be in application memory at all, a source backed by a secure element
never hands it over.

### Report a client credential that will not be presented

Mutual TLS is all-or-nothing: a certificate without its key, or a key without
its certificate, is a configuration error and is reported as one. It is never
silently treated as a decision to use server-authenticated TLS.

A key that does not match its certificate is the same mistake and is detectable
without going near the network, so it is reported at the same point. Left to
the handshake, it surfaces as a rejection from the collector and sends the
integrator to the wrong end.

A credential the TLS library will not take - a file that does not load, memory
it cannot allocate - is reported on the same terms. A passphrase is never
prompted for, so an encrypted key is a credential that will not load.

Delivery continues in every case. The receiver is the enforcement point for the
client's credential: a collector that requires one will refuse the handshake,
and one that does not was never going to check. The rule stops at the client's
own credential. A fault in the material a `Stream` presents never stops
delivery; a failed check on the peer always does.

### Permit the cryptographic level to be chosen

Where the underlying library allows the cipher policy to be selected, a
`Stream` passes the integrator's choice through unchanged and pins none of its
own above the floor. The appropriate policy depends on the build present on the
target and on the profile the deployment is held to, and neither is knowable
here.

For a deployment with no policy of its own, RFC 9662 §4 asks that
`TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` be offered, and BCP 195 §4.2 recommends
the same shape - ECDHE with AES-GCM - for TLS 1.2. Forward-secret suites are
preferred, and the others are not forbidden: BCP 195 §4.1 says they SHOULD NOT
be negotiated, RFC 9662 §4 still permits the 2009 mandatory suite, and a
deployment that must exclude static key exchange states a policy that does.

Since no ceiling is set, the version negotiated may be later than the floor, so
the policy binds whichever version is negotiated. Where a library splits the
choice across more than one setting, every one of them is the integrator's.

A policy the library rejects is reported as a configuration fault where the
library reports the rejection, and surfaces as a refused handshake where it
does not; each platform page says which.

The policy is asked for once per connection, alongside the expected peer
identity, so a deployment whose policy changes states the new one and moves
the stream's version.

### Do not resume a session under weaker terms

A resumed session carries the security parameters of the session it resumes,
so a `Stream` that resumes must check those parameters against what the current
configuration requires and complete a full handshake where they fall short.
RFC 5425 §4.2.3 recommends that check; this contract requires it.

A `Stream` that never resumes meets this by construction, and it must not be
possible to start resuming without revisiting the check.

### Do not send early data

RFC 9662 §6 forbids TLS 1.3 early data for syslog: a record sent before the
handshake completes can be replayed. A `Stream` never sends it.

### Do not renegotiate

A TLS 1.2 peer that does not acknowledge secure renegotiation is refused at
the handshake, as BCP 195 §3.5 requires. A request from the peer to renegotiate
an established connection is declined.

### Do not require revocation checking

Revocation checking is outside the contract. Many industrial deployments have
no route to a certificate revocation list or an OCSP responder, and a control
that depends on reaching one fails closed exactly when the network is under
attack. An integrator who needs it configures it in their own TLS library.

This is a deliberate deviation from BCP 195 §7.5, which requires a strategy to
distrust revoked certificates. The obligation moves rather than disappears: an
assessment that needs it met should record where, rather than assume this
library meets it.

It is also why certificate validity is enforced rather than tolerated. Without
a revocation check, the validity period is the only mechanism by which a
certificate that was acceptable stops being so.

### Bound the handshake

A handshake cannot stall the servicing pass. It runs against a deadline, over a
non-blocking transport, and gives up with a report when the budget expires.
That requires an injected sleep, which is why every TLS `Stream` asks for one.

### Send `close_notify` before tearing down

Closing sends the TLS close notification before the connection goes away, so
the collector can distinguish an orderly shutdown from a truncated session.
RFC 5425 §4.4 requires it.

It is attempted once and does not delay the close. The transport is
non-blocking, so a send buffer with no room for the alert drops it, and a
connection already broken cannot carry it. A collector must therefore still
treat a session that ends without one as truncated; what this obligation buys
is that an orderly close says so whenever the connection can still carry it.

### Check the configuration it cannot work without

**Wiring is checked when the stream is created.** A transport, a sleep, a
source of credentials, and whatever else the platform cannot operate without:
given a configuration missing one of these, a `Stream` reports a bad
configuration and returns the Null object. It does not accept the configuration
and fail on the first connection. What each platform cannot work without is on
its own page.

**Material is checked when a connection is made**, because that is when it is
obtained. A configuration naming neither trust anchors nor fingerprints, trust
anchors that cannot be produced, and a fingerprint that is not well formed are
reported then, and that connection attempt fails. The sender retries on its
next pass, so a transient source recovers on its own.

A client credential is checked at the same point and does not fail the
connection, for the reasons under *Report a client credential that will not be
presented*.

### Report every one of these, and name the check that failed

All of the above surface through the error handler rather than a return code an
integrator may not read. [Error handling](error-severity.md) covers what each
severity is telling you; `CRITICAL` at create time means the `Stream` fell back
to the Null object and nothing will be delivered.

Where a connection is refused, the report names which check refused it. An
integrator whose device will not connect needs to know which, because a generic
handshake failure sends them looking at the network for a fault that is on a
certificate.

**One fault is named, and this is which one.** A certificate can fail several
checks at once, and the report carries a single code, so the order is part of
the contract rather than an accident of whichever backend is linked:

1. A fault in the configuration itself - a fingerprint that is not well formed -
   before any fault in the peer, under the bad-configuration category rather
   than the handshake one, because no handshake was attempted.
2. A fingerprint that matched nothing, or that could not be computed.
3. A chain that reaches no trust anchor.
4. A name that does not match.
5. A validity period that has closed or not yet opened.

A fault saying *this is not the peer you expected* is reported ahead of one
saying *the expected peer's certificate is in a poor state*. The first may be
an attack; the second is an operational lapse.

Two consequences. A **matching pin waives nothing**: a peer whose fingerprint is
pinned is still refused for a name that does not match or a certificate outside
its dates. And the code names **the fault, not which certificate carries it**:
an issuer whose own dates have lapsed is reported as an expired certificate
even where the leaf is current.

**A peer that rejects us reports nothing more than that it did.** Where the
collector refuses the device - a client certificate it will not accept, or none
where it required one - the refusal arrives as a TLS alert, and what that alert
says is not portably knowable. The refusal is reported as a rejected handshake.

Under TLS 1.3 the collector can reach that decision after the device's first
record has left, in which case that record is gone. The device is told either
way, but which record surfaces the fault is a race. Do not build on the fault
appearing against any particular record.

### Key custody stays with the integrator

The library mints no key material and copies none into storage it keeps beyond
a connection. File permissions on a private key, whether it lives in a hardware
security module, and how it is rotated are properties of the deployment. The
per-connection obligation above is what makes those choices reachable: the
`Stream` asks for material when it needs it and reports when it is done.

A source that parses on demand holds what it parsed for the duration of the
connection, in storage belonging to the library, and wipes it on release. That
is the cost of parsing at all, and it is bounded by the connection rather than
by the life of the device. A source handed material already parsed holds a
pointer and no more. Each platform page says which its sources are.

Where a library takes its random number generator from the integrator, the
`Stream` cannot tell a seeded one from an unseeded one. An unseeded generator
makes every session's keys predictable, and nothing reports it. The platform
page says what the library needs.

### What falls to the device

RFC 5425 places obligations on the syslog sender as a whole that this library
does not meet and does not claim to: §4.2.1 requires a means to generate a key
pair and a self-signed certificate, §4.2.2 requires the certificate's
fingerprint to be available through a management interface, and §4.2.1
recommends recording the peer's end-entity certificate. The library exposes no
API for any of them, and no API that reads back the certificate a connection
was authorised on. An assessment against RFC 5425 records where the device
meets each.
