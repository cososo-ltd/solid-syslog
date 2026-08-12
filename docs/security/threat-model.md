# What to fold into your threat model

In threat-modelling terms SolidSyslog is not a component. It is not a process, a
device or a data store, and it has no privileges, identity or trust boundary of
its own. It is compiled into your product and runs with your trust and your
privileges, so the threat model that matters is the one you write for that
product.

This page is the input a supplier owes that exercise: where the library takes
your records, what it does at each edge it crosses, and what it leaves to you to
decide. It is not a compliance certificate, and it is citeable as
supplier-diligence evidence.

## Where records go

```text
your code ──Log()──▶ buffer ──Service()──▶ store ──▶ sender ──▶ TLS/TCP/UDP ──▶ collector
              (in-process)      (optional persistence)     (network)
```

Everything up to the sender happens inside your process, at your privilege. Two
edges leave it, and both are edges of your product rather than of the library.

## The network edge

Records cross an untrusted network to reach the collector.

Over TLS the library authenticates the collector against trust anchors you
supply, checks it against a name you declare, and can present a client credential
so the collector authenticates the device in return. Over plain UDP or TCP there
is no confidentiality, authenticity or integrity, by design and by your choice of
transport.

[TLS obligations](../tls.md) is the contract every TLS platform meets, including
where it departs from general TLS practice and why. Each platform's page records
where it falls short of that contract today.

## The storage edge

With store and forward configured, records come to rest on a medium that outlives
the process and may be physically accessible.

A SecurityPolicy seals each stored record, and
[at-rest cryptography](at-rest-cryptography.md) states what each one gives you.
No policy is wired unless you wire one, so physical extraction of the medium
discloses whatever was stored until you choose.

## What you supply and decide

| You provide | Because |
|---|---|
| The content, and the judgement not to log secrets | The library transports what you give it. It does not inspect, sanitise or redact |
| Certificates, keys and the CA bundle, and the peer name to verify | The library consumes trust material and mints none. It verifies against the name you declare rather than one it infers |
| The destination address, or a resolver you trust | The library connects to whatever address it is given and does not authenticate DNS responses |
| A properly seeded RNG where the TLS backend takes one | A weak RNG silently weakens TLS |
| A mutex and a config lock where concurrency exists | The synchronisation primitives are injected and default to no-ops |
| The storage medium, its permissions, and an at-rest policy if the content is sensitive | The library seals records. Access, retention and deletion on the medium are the platform's |
| The TLS and crypto libraries, including patching and CVE response | SolidSyslog rolls no crypto. It links yours |

If you wire a Buffer whose producer and consumer sit in different processes or
privilege domains, you own authenticity and integrity across that gap. The
library treats whatever it reads back as trusted. The shipped buffers stay within
one address space, and the POSIX message queue is created owner-only.

## What the library does not do

An integrator might reasonably assume otherwise about these, so they are stated
rather than left out.

- **End-to-end integrity through relays.** TLS protects each hop. Once a relay
  terminates the connection, nothing cryptographic binds the records that leave
  it to the device that raised them.
- **Anti-replay beyond the TLS session.** Anyone able to re-inject records
  downstream of a terminated hop can replay valid ones. Receiver-side
  de-duplication on the sequence number and timestamp is the mitigation; the
  sequence number is informational, not cryptographically bound.
- **Certificate revocation.** No shipped backend checks a revocation list or an
  online responder. Configure it in your TLS library if your assessment needs it.
- **Constant-time formatting.** Formatting is not constant-time. Syslog content
  is not usually secret-bearing, but treat it as residual if yours is.
- **Anything about a compromised caller.** The library trusts the process it runs
  in. If that process is owned, so is anything it logs.

## Related

[CRA guide](../cra.md) and [IEC 62443 guide](../iec62443.md) map what the library
provides against those frameworks. [RFC compliance](../rfc-compliance.md) states
what it emits clause by clause. The disclosure process for issues found against
this page is in [Security policy](policy.md).

This page is reviewed on any change that alters an edge or the division of
responsibility: a new transport, a new platform, or a new extension point. The
review lands in the pull request that makes the change.
