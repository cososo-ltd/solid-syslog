# TLS obligations

What a TLS `Stream` must do, whichever library provides it. Read this if you are
choosing between the TLS platforms, assessing the library against a security
standard, or writing a TLS `Stream` of your own.

The obligations here are the contract. What each shipped TLS platform actually
does, and where it differs, is on its own page - the
[capability matrix](platforms/index.md) shows which platforms fill the role.

## Delivery is preferred to silence

Syslog is how a device reports what happened to it. The moment the reporting
matters most is the moment the device is under attack, and that is also the
moment a security control that fails closed becomes a way to blind the collector.
An attacker who can block the route to a revocation responder, or who can make a
credential look half-configured, should not thereby be able to stop the device
reporting.

So the default is: **report the fault through the error handler, and keep
delivering.** A fault that an operator can see and act on is worth more than a
connection that refuses to open for a reason nobody is watching.

The rule has a limit: **delivery stops when the peer fails the check the
integrator asked for.** That covers a configuration naming neither trust anchors
nor pinned fingerprints, a certificate that does not validate against the trust
anchors, a certificate matching none of the configured fingerprints, and a
certificate that does not match a declared identity. Continuing through any of
them would hand the records to whoever answered instead, which loses the
confidentiality of the log and the audit trail at the same time, and does so
without anyone noticing.

The check is the integrator's to set. Declaring no peer identity is a decision
rather than a failure - it says chain verification alone is enough here, which on
a closed network with a private CA it may be. What the contract requires is that
the decision is explicit, and that the stream says so when it was never made at
all.

Everything else leaves you talking to the peer you trusted, holding a credential
you can no longer fully attest. Those are the faults that are reported while
delivery continues.

Where a store is configured, blocked delivery is delayed delivery rather than
lost delivery: records accumulate and replay on the next successful connection.
That bounds the cost of the exception without removing the reason for the rule,
because a store is finite and a SIEM that is blind now cannot alert now.

## The obligations

### Pin the protocol floor

A TLS `Stream` sets its own minimum protocol version rather than inheriting
whatever the TLS library was built to permit. Downgrade resistance is then a
property of this library rather than of the integrator's build of another one.
The floor is TLS 1.2.

No ceiling is required, and setting one would be wrong. RFC 9662, which updates
RFC 5425, asks that TLS 1.3 be supported and **preferred** where it is
implemented, so a stream that pinned a ceiling to constrain something else would
breach that. BCP 195 §3.1.1 says the same for TLS generally.

### Require a trust anchor or a pinned fingerprint

A peer is authorised in one of two ways, and a `Stream` requires at least one of
them to be configured. Given neither, it reports a bad configuration and returns
the Null object rather than connecting to a peer it cannot check.

**Certification path validation.** The peer certificate must chain to trust
anchors the integrator supplies. There is no fallback to a system trust store: an
embedded target may not have one, and on a host that store is a far larger trust
base than a device reporting to a single collector needs. Validity dates are part
of this check, not separate from it - RFC 5280 §6.1 makes the validity period an
input to path validation, so a certificate outside its own dates does not chain
and the connection stops.

**A pinned certificate fingerprint.** Covered in its own obligation below. A
certificate matched by fingerprint needs no chain: RFC 5425 §4.2.1 states that
such a certificate "can be self-signed, and no certification path validation is
needed".

Where both are configured, the peer must satisfy both. RFC 5425 §6.1 names that
combination as the recommended default policy.

### Accept a peer authorised by certificate fingerprint

RFC 5425 §5.1 requires that a peer can be authorised by its certificate
fingerprint, not only by a chain to a trust anchor and a name. The two are
different tools: a fingerprint pins one certificate, which suits a closed network
with no PKI, where issuing and rotating a CA is more machinery than the
deployment wants.

The accepted form is the one RFC 5425 §4.2.2 defines: an ASCII hash label, a
colon, then the hash of the DER-encoded certificate as colon-separated uppercase
hex pairs. Labels come from the IANA
[Hash Function Textual Names](https://www.iana.org/assignments/hash-function-text-names/hash-function-text-names.xhtml)
registry and are hyphenated, so `sha-256` and `sha-1` rather than `sha256` or
`sha1`. A `Stream` accepts both of those algorithms. §4.2.2 makes SHA-1 mandatory
to support; `sha-256` is the one to configure where the collector offers a
choice.

A **list** of fingerprints is accepted, and any one of them authorises the peer.
A fingerprint covers the whole DER certificate, so it changes every time the
collector's certificate is renewed. Pinning the old and the new together is what
lets a fleet cross a renewal without every device stopping at once.

Where fingerprints are configured and the peer's certificate matches none of
them, the connection stops, whatever the chain says.

### Treat endpoint identity as declared, not assumed

The integrator declares the peer identity they expect. A `Stream` verifies it
when one is declared, accepts an explicit decision not to check a name, and
reports when nothing was declared at all - because that last case is a peer that
is chain-verified but otherwise unidentified, which is the case an attacker with
any trusted certificate walks through. BCP 195 §7.1 puts it plainly: without the
name check, TLS proves the certificate is valid and that the peer holds its key,
but not that you reached the endpoint you wanted.

A configured fingerprint is itself a declaration of identity, and a stronger one
than a name: it names the exact certificate expected rather than a subject within
a trusted CA's namespace. So a `Stream` given fingerprints and no name does not
report an unidentified peer.

The states, and what each means, are documented on each platform's configuration
field.

### Obtain credentials per connection, and release them after

Trust anchors, the client credential and the expected peer identity are obtained
when a connection is made and released when it closes. Nothing is held between
connections, so an integrator can keep material in a secure element, an encrypted
store or a key ring and have it exist in RAM only while a connection is being
established.

Two things follow. A device issued new credentials while it is running uses them
on its next connection without being restarted, and forcing that reconnection
with `SolidSyslogSender_Disconnect` makes it immediate. And the window in which
the integrator must keep material alive is the connection, not the lifetime of
the stream, because the `Stream` says when it is finished with it.

What the underlying TLS library holds during a connection is a property of that
library. Every one of them keeps the parsed certificate and key for the duration
of the session, the private key included, and no `Stream` can change that. The
obligation is about the window, not about the handshake.

The expected identity travels with the destination. Where the destination can be
changed at runtime, redirecting a device to a different collector must carry the
identity its certificate is checked against, or the redirection quietly moves the
device to a peer nobody is verifying.

Where credentials come from is the integrator's choice, and each platform
documents the mechanism it offers.

### Report a partially configured client credential

Mutual TLS is all-or-nothing: a certificate without its key, or a key without its
certificate, is a configuration error and is reported as one. It is never
silently treated as a decision to use server-authenticated TLS, because the
integrator who supplied half a credential believes they have mutual
authentication and does not have it.

A key that does not match the certificate it was supplied with is the same
mistake reached differently, and is detectable without going near the network, so
it is reported at the same point. Left to the handshake, it comes back as a
rejection from the collector, which sends the integrator looking at the collector
for a fault that is on the device.

Delivery continues. The receiver is the enforcement point for our credential - a
collector that requires a client certificate will refuse the handshake, and one
that does not was never going to check. Blocking here would deny the audit trail
without changing what the collector decides.

### Permit the cryptographic level to be chosen

Where the underlying library allows the cipher policy to be selected, a `Stream`
passes the integrator's choice through unchanged and pins none of its own. The
appropriate policy depends on the build present on the target and on the profile
the deployment is held to, and neither is knowable here.

For a deployment with no policy of its own, RFC 9662 §4 asks that
`TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` be offered, and BCP 195 §4.2 recommends
the same shape - ECDHE with AES-GCM - for TLS 1.2. Both prefer it over the 2009
mandatory suite, which offers no forward secrecy.

Since no ceiling is set, the version negotiated may be later than the floor, and
a policy that binds only up to the floor does not bind the connection in use.
Passing the integrator's choice through means passing it through for whichever
version is negotiated.

Where the library does not allow it, its own defaults apply. What each platform
can and cannot select is on its page.

### Do not resume a session under weaker terms

A resumed session carries the security parameters of the session it resumes, so a
`Stream` that resumes must check those parameters against what the current
configuration requires and complete a full handshake instead where they fall
short. RFC 5425 §4.2.3 requires it.

A `Stream` that never resumes meets this by construction, and it must not be
possible to start resuming without revisiting the check.

### Do not require revocation checking

Revocation checking is outside the contract. Many industrial deployments have no
route to a certificate revocation list or an OCSP responder, and a control that
depends on reaching one fails closed exactly when the network is the thing under
attack.

An integrator who needs it configures it in their own TLS library and verifies it
themselves. The library neither performs the check nor reports on whether one is
in force.

This is a deliberate deviation, and worth naming as one. BCP 195 §7.5 requires a
TLS implementation to implement a strategy to distrust revoked certificates, and
no stream here implements one. The reasoning is above; what makes it tolerable is
that the obligation moves rather than disappears. An integrator's own TLS library
can be configured for CRL or OCSP, and this library neither performs that check
nor prevents it - so an assessment that needs the obligation met should say where
it is met, rather than assume this library meets it.

It is also why certificate validity is enforced rather than tolerated. With no
revocation check, the validity period is the only mechanism by which a
certificate ever stops being accepted.

### Bound the handshake

A handshake cannot stall the servicing pass indefinitely. It runs against a
deadline, over a non-blocking transport, and gives up with a report rather than
blocking when the budget expires. That requires an injected sleep, which is why
every TLS `Stream` asks for one.

### Send `close_notify` before tearing down

Closing sends the TLS close notification before the connection goes away, so the
collector can distinguish an orderly shutdown from a truncated session. RFC 5425
§4.4 requires it.

### Check the configuration it cannot work without

A `Stream` is given two kinds of thing, and each is checked at the point it can
be.

**Wiring is checked when the stream is created.** A transport, a sleep, a source
of credentials, and whatever else the platform cannot operate without: given a
configuration missing one of these, a `Stream` reports a bad configuration and
returns the Null object. It does not accept the configuration and then fail on
the first connection, and it does not dereference what is missing. What each
platform cannot work without is on its own page.

**Material is checked when a connection is made**, because that is when it is
obtained. Credentials that cannot be produced, a configuration naming neither
trust anchors nor fingerprints, and a fingerprint that is not well formed are all
reported then, and that connection attempt fails. The sender retries on its next
pass, so a transient source recovers on its own.

This is the library-wide rule for anything that reaches the wire rather than
anything specific to TLS: a failure an integrator caused at setup is reported at
setup, where they are still looking.

### Report every one of these, and name the check that failed

All of the above surface through the error handler rather than a return code an
integrator may not read. [Error handling](error-severity.md) covers what each
severity is telling you; the short form is that `CRITICAL` at create time means
the `Stream` fell back to the Null object and nothing will be delivered.

Where a connection is refused, the report names which check refused it - an
expired certificate, one not yet valid, a chain that does not validate, a
fingerprint that matched nothing, a name that did not match. An integrator whose
device will not connect needs to know which of those it is, because a generic
handshake failure sends them looking at the network for a fault that is on a
certificate.

### Key custody is yours

The library holds no key material of its own and uses whatever it is given. File
permissions on a private key, whether it lives in a hardware security module, and
how it is rotated are properties of your deployment, not of this contract. The
per-connection obligation above is what makes those choices reachable: the
`Stream` asks for material when it needs it and tells you when it is done, so
where the material rests in between is yours to decide.

## Where this stands

These obligations are the target and are not yet met uniformly. Each shortfall is
recorded on the affected platform's page and tracked as an issue. Read the page
for the platform you are wiring before you rely on any obligation above.
[E39](https://github.com/cososo-ltd/solid-syslog/issues/782) carries the work and
the design behind it.

**Fingerprint authorisation is not offered by any shipped platform.** A peer is
authorised by trust anchor and name alone, so a deployment with no PKI has no way
to pin a collector. Tracked as
[#753](https://github.com/cososo-ltd/solid-syslog/issues/753).

**A refused handshake does not say which check refused it.** Both shipped
platforms fail the connection correctly and report it without naming the cause.
Tracked as [#731](https://github.com/cososo-ltd/solid-syslog/issues/731).

**Credentials are held for the lifetime of the stream, not per connection.** Both
shipped platforms read their material on every connection, so rotation works, but
neither releases it afterwards and neither offers a choice of where it comes
from. Tracked under
[E39](https://github.com/cososo-ltd/solid-syslog/issues/782).

**A partially configured client credential is handled differently on each
platform**, and neither matches the contract. One refuses the connection, which
is safe but stricter than this page requires; the other accepts it in silence and
connects without the client certificate, so a device configured for mutual TLS
can run without ever presenting one. If you rely on mutual TLS, read your
platform's page. Tracked as
[#718](https://github.com/cososo-ltd/solid-syslog/issues/718),
[#719](https://github.com/cososo-ltd/solid-syslog/issues/719) and
[#734](https://github.com/cososo-ltd/solid-syslog/issues/734).

**A cipher policy does not bind the connection actually negotiated.** Tracked as
[#733](https://github.com/cososo-ltd/solid-syslog/issues/733).

**Configuration is not checked when the stream is created**, on either platform.
Tracked as [#732](https://github.com/cososo-ltd/solid-syslog/issues/732).

No shipped `Stream` resumes a session, so the resumption obligation is met and
there is nothing to track against it.
