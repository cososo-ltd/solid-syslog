# TLS obligations

What a TLS `Stream` must do, whichever library provides it. Read this if you are
choosing between the TLS platforms, assessing the library against a security
standard, or writing a TLS `Stream` of your own.

The obligations here are the contract. What each shipped TLS platform actually
does, and where it differs, is on its own page — the
[capability matrix](platforms/index.md) shows which platforms fill the role.

## Delivery is preferred to silence

Syslog is how a device reports what happened to it. The moment the reporting
matters most is the moment the device is under attack, and that is also the
moment a security control that fails closed becomes a way to blind the collector.
An attacker who can move a clock forward, block the route to a revocation
responder, or wait for a certificate to lapse should not thereby be able to stop
the device reporting.

So the default is: **report the fault through the error handler, and keep
delivering.** A fault that an operator can see and act on is worth more than a
connection that refuses to open for a reason nobody is watching.

The rule has a limit, and it is one line rather than a list of exceptions:
**delivery stops when the peer fails the check the integrator asked for.** No
trust anchors to load, a certificate that does not chain to them, and a
certificate that does not match a declared identity are all that case.
Continuing through any of them would hand the records to whoever answered
instead, which loses the confidentiality of the log *and* the audit trail at the
same time, and does so without anyone noticing.

The check is the integrator's to set. Declaring no peer identity is a decision
rather than a failure — it says chain verification alone is enough here, which on
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

### Require a trust anchor, and take it from the integrator

The peer certificate must chain to trust anchors the integrator supplies, and a
`Stream` that cannot load them fails to open. There is no fallback to a system
trust store: an embedded target may not have one, and on a host that store is a
far larger trust base than a device reporting to a single collector needs.

### Treat endpoint identity as declared, not assumed

The integrator declares the peer identity they expect. A `Stream` verifies it
when one is declared, accepts an explicit decision not to check a name, and
reports when nothing was declared at all — because that last case is a peer that
is chain-verified but otherwise unidentified, which is the case an attacker with
any trusted certificate walks through. BCP 195 §7.1 puts it plainly: without the
name check, TLS proves the certificate is valid and that the peer holds its key,
but not that you reached the endpoint you wanted.

The three states, and what each means, are documented on each platform's
configuration field.

### Accept a peer authorised by certificate fingerprint

RFC 5425 §5.1 requires that a peer can be authorised by its certificate
fingerprint, not only by a chain to a trust anchor and a name. The two are
different tools: a fingerprint pins one certificate, which suits a closed network
with no PKI, where issuing and rotating a CA is more machinery than the deployment
wants.

**No shipped platform meets this today** — see [#753](https://github.com/cososo-ltd/solid-syslog/issues/753).

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

Delivery continues. The receiver is the enforcement point for our credential — a
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
the same shape — ECDHE with AES-GCM — for TLS 1.2. Both prefer it over the 2009
mandatory suite, which offers no forward secrecy.

Since no ceiling is set, the version negotiated may be later than the floor, and a
policy that binds only up to the floor does not bind the connection in use.
Passing the integrator's choice through means passing it through for whichever
version is negotiated.

Where the library does not allow it, its own defaults apply. What each platform
can and cannot select is on its page.

### Take rotated credentials and a changed identity on the next connection

A device issued new credentials while it is running uses them without being
restarted. Trust anchors, the client credential and the expected peer identity
are read when a connection is made, not remembered from when the stream was
created, so replacing them and reconnecting is all it takes. Forcing that
reconnection with `SolidSyslogSender_Disconnect` makes it immediate.

The expected identity travels with the destination. Where the destination can be
changed at runtime, redirecting a device to a different collector must carry the
identity its certificate is checked against, or the redirection quietly moves the
device to a peer nobody is verifying.

Each platform documents the sequence its own credential model requires, because
replacing material a stream is holding is not safe at every moment.

### Report an unusable certificate, and keep delivering

A certificate that is expired, not yet valid, or otherwise unusable while still
chaining to a trusted anchor is reported, and delivery continues. Clock skew is
the dominant real cause: a device without a real-time clock that boots at the
epoch, or one whose time source has been tampered with, is precisely the device
whose logs you want to keep receiving.

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
nor prevents it — so an assessment that needs the obligation met should say where
it is met, rather than assume this library meets it.

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

A `Stream` given a configuration it has no way to use — no sleep to poll the
handshake with, no trust anchors to verify against — reports a bad configuration
and returns the Null object. It does not accept the configuration and then fail
on the first connection, and it does not dereference what is missing. What else a
given platform cannot work without is on its own page.

This is the library-wide rule for anything that reaches the wire rather than
anything specific to TLS: a failure an integrator caused at setup is reported at
setup, where they are still looking.

### Report every one of these

All of the above surface through the error handler rather than a return code an
integrator may not read. [Error handling](error-severity.md) covers what each
severity is telling you; the short form is that `CRITICAL` at create time means
the `Stream` fell back to the Null object and nothing will be delivered.

### Key custody is yours

The library holds no key material of its own and uses whatever it is given. File
permissions on a private key, whether it lives in a hardware security module, and
how it is rotated are properties of your deployment, not of this contract.

## Where this stands at 0.1.0

These obligations are the target, and they are not yet met uniformly. At 0.1.0
the shipped TLS platforms diverge on several of them, and each divergence is
recorded on that platform's page and tracked as an issue. Read the page for the
platform you are wiring before you rely on any obligation above.

Certificate validity is the one both fall short of the same way: an expired
certificate refuses the connection rather than being reported while delivery
continues.

A partially configured client credential matters more, because the two platforms
differ. One refuses the connection, which is safe but stricter than the contract.
The other accepts it in silence and connects without the client certificate, so a
device configured for mutual TLS can run without ever presenting one. If you rely
on mutual TLS, read your platform's page before you rely on this obligation.

Configuration checking at create time is the third shortfall, and it is not
confined to TLS.

Fingerprint-based peer authorisation is the fourth, and both platforms fall short
of it the same way: neither offers it at all. Until
[#753](https://github.com/cososo-ltd/solid-syslog/issues/753) lands, a peer is
authorised by trust anchor and name.
