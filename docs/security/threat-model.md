# What to fold into your threat model

In a threat model SolidSyslog is not an element of its own. It is not a process, a
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
supply, or against certificate fingerprints you pin where there is no PKI to
anchor to, checks it against a name you declare, and can present a client
credential so the collector authenticates the device in return. Over plain UDP or TCP there
is no confidentiality, authenticity or integrity, by design and by your choice of
transport.

[TLS obligations](../tls.md) is the contract every TLS platform meets, including
where it departs from general TLS practice and why. Each platform's page records
where it falls short of that contract.

## The storage edge

With store-and-forward configured, records come to rest on a medium that outlives
the process and may be physically accessible.

A SecurityPolicy seals each stored record, and
[at-rest cryptography](at-rest-cryptography.md) states what each one gives you.
No policy is wired unless you wire one, so physical extraction of the medium
discloses whatever was stored until you choose. A seal covers each record, not the
store as a whole, and [what a seal covers](at-rest-cryptography.md#what-a-seal-covers)
says what that leaves undetected.

## What you supply and decide

| You provide | Because |
|---|---|
| The content, and the judgement not to log secrets | The library transports what you give it. It does not inspect, sanitise or redact |
| Certificates, keys, and either a CA bundle or the fingerprints to pin, and the peer name to verify | The library consumes trust material and mints none. It verifies against the name you declare rather than one it infers |
| The destination address, or a resolver you trust | The library connects to whatever address it is given and does not authenticate DNS responses |
| A properly seeded RNG where the TLS backend takes one | A weak RNG silently weakens TLS |
| A mutex and a config lock where concurrency exists | The synchronisation primitives are injected and default to no-ops |
| The storage medium, its permissions, and an at-rest policy if the content is sensitive | The library seals records. Access, retention and deletion on the medium are the platform's |
| The TLS and crypto libraries, including patching and CVE response | SolidSyslog rolls no crypto. It links yours |
| The wall clock, and whatever sets it | Certificate validity is judged against it, and it stamps every record. Whoever can set it can bring an expired collector certificate back into its dates, stop delivery by moving the clock past them, or misdate records |
| The channel that delivers certificates, keys, pins and the peer name, at commissioning and after | The library trusts whatever material it is handed at the next connection. A pin taken from a connection to the collector, or one any task can install, trusts whoever is on the other end |

If you wire a Buffer whose producer and consumer sit in different processes or
privilege domains, you own authenticity and integrity across that gap. The
library treats whatever it reads back as trusted. The circular and passthrough
buffers stay within one address space. The POSIX message-queue buffer does not: its
queue is a kernel object reachable by name, and
[its platform page](../platforms/posix/index.md) says who can reach it.

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
  online responder. Configure it in your TLS library if your assessment needs it;
  [TLS obligations](../tls.md#do-not-require-revocation-checking) says why it is
  outside the contract.
- **Integrity of the store as a whole.** A seal protects each record's content.
  Removing records, or changing which are marked sent, is not detected on the device;
  [what a seal covers](at-rest-cryptography.md#what-a-seal-covers) says what shows it.
- **Constant-time formatting.** Formatting is not constant-time. Syslog content
  is not usually secret-bearing, but treat it as residual if yours is.
- **Anything about a compromised caller.** The library trusts the process it runs
  in. If that process is compromised, so is anything it logs.

## Related

[CRA guide](../cra.md) and [IEC 62443 guide](../iec62443.md) map what the library
provides against those frameworks. [RFC compliance](../rfc-compliance.md) states
what it emits clause by clause. The disclosure process for issues found against
this page is in [Security policy](../../SECURITY.md).

This page is reviewed on any change that alters an edge or the division of
responsibility: a new transport, a new platform, or a new extension point. The
review lands in the pull request that makes the change.
