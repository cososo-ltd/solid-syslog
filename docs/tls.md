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

There is one exception. Where the integrator has declared which peer they expect,
a mismatch stops delivery. Continuing would hand the records to whoever answered
instead, which loses the confidentiality of the log *and* the audit trail at the
same time, and does so without anyone noticing. Every other failure below leaves
you talking to the peer you trusted with a credential you can no longer fully
attest; that one leaves you talking to someone else.

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

No ceiling is required. A peer that offers a later version is offering a better
one.

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
any trusted certificate walks through.

The three states, and what each means, are documented on the configuration field
itself.

### Report a partially configured client credential

Mutual TLS is all-or-nothing: a certificate without its key, or a key without its
certificate, is a configuration error and is reported as one. It is never
silently treated as a decision to use server-authenticated TLS, because the
integrator who supplied half a credential believes they have mutual
authentication and does not have it.

Delivery continues. The receiver is the enforcement point for our credential — a
collector that requires a client certificate will refuse the handshake, and one
that does not was never going to check. Blocking here would deny the audit trail
without changing what the collector decides.

### Permit the cryptographic level to be chosen

Where the underlying library allows the cipher policy to be selected, a `Stream`
passes the integrator's choice through unchanged and pins none of its own. The
appropriate policy depends on the build present on the target and on the profile
the deployment is held to, and neither is knowable here.

Where the library does not allow it, its own defaults apply. What each platform
can and cannot select is on its page.

### Take rotated credentials without a restart

Replacing credential material takes effect without restarting the process, on the
next connection at the latest. Rotation is a deployment operation, so no reload
call is part of the API; forcing a reconnection with
`SolidSyslogSender_Disconnect` is enough to make it immediate.

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
handshake with, no trust anchors, no random source — reports a bad configuration
and returns the Null object. It does not accept the configuration and then fail
on the first connection, and it does not dereference what is missing.

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

The two that differ most today are the handling of a partially configured client
credential and the certificate-validity rule, where the current behaviour is to
refuse the connection rather than to report and continue. Configuration checking
at create time is the other known shortfall, and it is not confined to TLS.
