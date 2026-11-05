# wolfSSL

`Platform/WolfSsl/` wraps [wolfSSL](https://www.wolfssl.com/) for embedded
targets: TLS over an injected transport, filling the
[Stream](../../api/structSolidSyslogStream.md) role, and keyed at-rest
cryptography over wolfCrypt, filling the
[SecurityPolicy](../../api/structSolidSyslogSecurityPolicy.md) role.

This page covers what this adapter needs, which of your wolfSSL build's choices
it depends on, how it meets the [TLS obligations](../../tls.md), where it falls
short of them, and the coexistence guarantee it makes.

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
on the one it prefers. The build choices that change what it does are listed
below.

wolfSSL is dual-licensed, GPLv3 or commercial, since 5.8.2; earlier releases
were GPLv2. SolidSyslog links your build and ships none of it, so the licence
choice stays with you.

`wolfSSL_Init` is yours to call, once, before the first connection, and
`wolfSSL_Cleanup` yours to call after the last; the stream calls neither, as
the coexistence contract below says. No random number generator is handed to
the stream: wolfSSL seeds its own from your build's seed source.

An initialised `WC_RNG` is required by the AES-GCM policy, for its nonces. The
HMAC policy needs none. wolfCrypt seeds the RNG from your build's seed source
when you initialise it, so seeding is not something this adapter can get wrong
or check.

## Credentials come from a credentials source

Where trust anchors, pinned peer fingerprints and the mutual-TLS client
credential come from is the integrator's choice rather than this adapter's. The
stream is wired to a `SolidSyslogWolfSslCredentials`, asked once per connection
to install its material on the `WOLFSSL_CTX` built for that connection and told
once per connection when that material is no longer needed. A source backed by
a secure element or a key store is a class implementing that role, and needs no
change here.

One source ships with the pack, and it opens no file - which is what allows the
adapter to run on targets built with `NO_FILESYSTEM`.

`SolidSyslogWolfSslPemBufferCredentials` carries PEM held in memory and hands it
to wolfSSL per connection. Nothing is parsed at Create, and nothing is held
between connections: the material lives in the context the stream builds for a
connection, which the stream frees at Close. Because the source holds nothing of
its own, one instance can serve several streams at once, and rotating the
material is a matter of writing the new PEM over the buffer and moving the
stream's configuration version - the next connection reads the new bytes. Make
the change on the task that calls `SolidSyslog_Service`, so a connection never
reads a half-written buffer.

The credential window is explicit. Install is called after the transport
connects and before the session is created, so what it installs is what the
session inherits; Release answers every Install, once, after the context has
been freed and with it every pointer into the material. That window is what a
source reaching a secure element or an encrypted store needs.

A `Release` reached by destroying the stream runs inside the configuration lock,
because pool cleanup holds it. Freeing your own material there is fine; creating
or destroying any SolidSyslog object from `Release` is not, since that takes the
same lock and a non-recursive one deadlocks.

## What it depends on in your build

The pack compiles against the builds below. What a choice costs is stated
beside it; a choice the pack refuses is refused at build time, so the cost is
never a silent one.

| Build choice | What the pack does with it |
|---|---|
| `HAVE_SNI` (on in a `./configure` build) | Sends the expected peer name as the server name indication. Without it the indication cannot be set, and a connection that declares a name fails at Open as `SERVER_NAME_NOT_APPLIED`; a connection that declares none is unaffected |
| `WOLFSSL_IP_ALT_NAME` (`--enable-ip-alt-name`) | Matches an expected address literal against the certificate's `iPAddress` entries. Without it no address ever matches, and a peer expected by address is refused as `PEER_NAME_MISMATCHED` on every connection; expect the peer by name instead |
| `WOLFSSL_ALWAYS_VERIFY_CB`, `KEEP_PEER_CERT` (define either) | Decide how a pin is checked where trust anchors are installed too, below. With `WOLFSSL_ALWAYS_VERIFY_CB` it is checked in the verify callback, before the client's credential is sent. With only `KEEP_PEER_CERT` it is checked once the handshake has completed, against the certificate the library kept. With neither, anchors and pins together are refused at Open as `PINS_WITH_ANCHORS_UNSUPPORTED`; pins alone, and anchors alone, work on any build |
| `NO_SHA`, `NO_SHA256` | Compile out the hash a pin names. A pin under a hash the build lacks is skipped, and a peer that no other pin names is refused as `FINGERPRINT_DIGEST_UNAVAILABLE` |
| `WOLFSSL_TLS13` (`--enable-tls13`, on in a `./configure` build) | Negotiates TLS 1.3 where the collector offers it. Without it the connection is TLS 1.2, which the floor permits |
| `NO_ASN_TIME` | Compiles the validity-period check out. The adapter will not build with it set, below |
| RSA, finite-field Diffie-Hellman and ECC (`NO_RSA`, `NO_DH`, `HAVE_ECC`) | The pack sets the key-size floor for all three, so a build that trims any of them out does not compile the stream. A trimmed build is not supported |
| `HAVE_AESGCM` (on by default) | The AES-GCM policy. A build without it has no `wc_AesGcmEncrypt` to link |
| `wc_ForceZero` | Wipes key material after every policy operation. A build that defines `WOLFSSL_NO_FORCE_ZERO` supplies its own, as wolfSSL documents |

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
turn debugging on. TLS policy is applied per session, on a context built for
that one connection, so it cannot affect a context you build elsewhere. Every
wolfCrypt context the policies use is built for one operation and freed after
it. A device that already uses wolfSSL for firmware update or a vendor cloud
SDK keeps that configuration intact, and the claim can be checked against the
directory.

## What it reports

Every class in this pack reports portable detail codes, so a handler written
against them keeps working if the crypto backend underneath changes. What stays
specific to this pack is `event->Source`, which names the class that reported.

Each role's codes are the whole vocabulary that role can express, so some
describe faults this pack cannot have and it never raises them.

From the TLS-stream codes it does not raise `NULL_RNG`: no generator is handed
to the stream. `HANDSHAKE_REJECTED` covers what the peer or the protocol refused and no check
of this stream names, which includes a Diffie-Hellman group or a ciphersuite
below the floor.

From the credentials codes it does not raise `NULL_RNG` or `ALREADY_IN_USE`:
the PEM-buffer source needs no generator, and holds nothing between
connections, so it is never in use. Nor `PEM_NOT_TERMINATED`: wolfSSL reads a buffer to the
length it is given and needs no terminator. Nor `TRUST_ANCHORS_NOT_LOADED`: the
source parses a buffer and reports `TRUST_ANCHORS_NOT_PARSED` if that fails.
Nor `CLIENT_CREDENTIAL_MISMATCHED`: whether a client key matches its
certificate is not checked, because the only wolfSSL call that checks it lives
in the compatibility layer wolfSSL offers for another library's API, which this
pack never enables. A mismatch surfaces when the collector refuses the
handshake.

The at-rest policies raise every code their roles define.

## What a connection is made with

The stream asks for a profile once per connection, and takes the expected peer
name and the cipher policy from it. Nothing is stored between connections, so a change is a matter of
returning something different and moving the stream's version.

A name is validated by wolfSSL as a hostname when it is declared: labels of
letters, digits and hyphens, at least two of them, the last not all digits.
`localhost` is accepted as itself. A name wolfSSL will not take - a single
label, a wildcard, a leading dot - fails the connection at Open as
`SERVER_NAME_NOT_APPLIED`, which names the configuration rather than the peer.
A collector reached by a bare host name is declared by its fully qualified one.

A pinned peer needs no name. A pin declares the peer's identity more strongly
than a name does, so a connection that pins the peer and declares no name is
not reported as unnamed; one that declares a name still has it checked.

An expected identity that is an address literal - digits and dots, or anything
with a colon in it - is declared to wolfSSL as an address and matched against
the certificate's `iPAddress` entries alone, never against a DNS name or Common
Name spelling the same digits; RFC 9525 section 6.2 asks for exactly that. On a
build without `WOLFSSL_IP_ALT_NAME` nothing matches it, as the table above says.
The literal is sent as the server name indication as well, as the contract
requires of every declared name.

Of the cryptographic floor, the stream pins the protocol version at TLS 1.2 and
the key sizes BCP 195 section 4.5 names: 2048 bits for RSA and for finite-field
Diffie-Hellman, 224 bits for elliptic curves, applied to the peer's certificate
and to the handshake alike. A certificate whose key is below the floor is
refused as `PEER_CERTIFICATE_UNTRUSTED`, with the rest of what the chain fails
on. No ceiling is set: RFC 9662 requires TLS 1.3 to be preferred wherever it is
implemented, and a build with `WOLFSSL_TLS13` negotiates it.

The cipher policy is one list for both TLS versions, in the form wolfSSL takes:
names separated by colons, each in wolfSSL's own spelling, such as
`ECDHE-RSA-AES128-GCM-SHA256` and `TLS13-AES128-GCM-SHA256`, or in the IANA
one. A list that names a suite for each version is the whole of what is
offered, on any build. A list that names one version only offers nothing at the
other, except on a wolfSSL built with `OPENSSL_EXTRA`, which updates the
version the list names and keeps the other version's suites as they were. Name
a suite for each version to bind both.
Leave the policy unset and every suite the build enables is offered; an empty
list and the keywords `ALL`, `DEFAULT` and `HIGH` ask for the same. wolfSSL
keeps the names it knows and drops the rest without reporting them, so it
rejects a list only where it knows none of its names. That fails the connection
at Open as `CIPHER_POLICY_REJECTED`, before any handshake, and nothing is
widened to compensate. A suite the build knows and the collector will not
negotiate is refused in the handshake as `HANDSHAKE_REJECTED`. Key-exchange
groups and signature algorithms are not selectable through the list, and the
key-size floors apply whatever it names.

Pinned peer fingerprints are read from the credentials source at each Open and
checked against the leaf the collector presents, by its SHA-256 or SHA-1 digest
as each pin names. A pin that will not parse fails the connection at Open as
`FINGERPRINT_MALFORMED`, and a `sha-1` pin is reported as `FINGERPRINT_SHA1` on
every connection that uses one.

With pins and no trust anchors, every chain fails wolfSSL's verification for
want of a signer, and that is the one objection a matching pin waives. A
collector with no PKI is authorised by its pin alone, whether it presents a
self-signed certificate or one with its issuer behind it. wolfSSL stops at the
missing signer and reports nothing it found before it, so the stream checks
what would otherwise go unchecked: each certificate whose missing signer it
waives is parsed again as a trust anchor would be, and one outside its validity
period is refused as `PEER_CERTIFICATE_EXPIRED` or
`PEER_CERTIFICATE_NOT_YET_VALID`, as the contract requires of a pin. A name that
does not match, and a key below the floor, are refused as they would be without
a pin.

With pins and trust anchors both installed, the peer must chain to an anchor
and match a pin. How the pin is checked on a chain that verifies depends on the
build, as the table above says: wolfSSL calls the verify callback on a
certificate that verifies only under `WOLFSSL_ALWAYS_VERIFY_CB`. A build may
choose a weaker check than its flags allow by defining
`SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK` to
`SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_AFTER_HANDSHAKE` or
`SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_REFUSED` - the refusal, say, rather than a
check made after the client credential has gone. A choice the flags cannot
honour stops the build.

Each connection is a fresh session with none supplied to resume, and the stream
never enables early data or renegotiation, so it meets the contract's
resumption and early-data obligations by construction.

A send first reads what the collector has sent, so that a session the collector
has closed refuses the record rather than taking it, as the
[Stream](../../api/structSolidSyslogStream.md) contract requires. A write alone
would not notice: wolfSSL takes the record into the session regardless of the
close_notify waiting unread, and the transport beneath may not see the
collector's close behind that unread alert. When the read meets the close, or
the transport has gone, the send fails, the stream closes itself, and your store
replays the record on the next connection. Syslog over TLS carries nothing from
collector to device, so the read normally finds nothing; application data it
does meet is discarded, not kept for a later read, and the probe makes at most
eight reads of up to 16 bytes each, so a close behind more than that goes
unnoticed.

A client credential supplied to the PEM-buffer source is loaded onto the
context and presented when the collector asks for one.

## Where it falls short of the contract

Certificate validity is checked only where the build carries a clock, below.

The refusal names the checks in the contract's order where wolfSSL raises more
than one, with one exception. A certificate that is both misnamed and outside
its validity period is named for its dates: wolfSSL treats the date failure as
fatal before it checks the name, so the name is never raised and the stream has
nothing to put first. The peer is refused either way. Whether that order, and
the contract's, should change is open under
[#1133](https://github.com/cososo-ltd/solid-syslog/issues/1133).

On a build with `KEEP_PEER_CERT` and without `WOLFSSL_ALWAYS_VERIFY_CB`, a pin
beside trust anchors is checked once the handshake has completed, so a collector
whose chain verifies and whose certificate matches no pin is refused after the
device has sent its client credential, where one is configured. No record is
sent to it. Build with `WOLFSSL_ALWAYS_VERIFY_CB` to refuse it first.

Of the cryptographic floor, what a certificate in the chain may be signed with
is not pinned. wolfSSL offers no per-connection way to refuse a chain
certificate signed with SHA-1; the only control is building the library without
SHA-1 at all, which refuses it everywhere. A deployment that must refuse such a
chain builds wolfSSL that way, or issues its collector certificates without one.

Every other obligation under [TLS obligations](../../tls.md) is met by this
pack as shipped.

## Certificate validity depends on your build carrying a clock

`NO_ASN_TIME` is what makes wolfSSL skip the dates on a certificate. With it set
the expired and not-yet-valid checks are compiled out, and a certificate outside
its validity period is accepted - silently, because nothing failed. On a board
with no real-time clock it is tempting to set it for exactly that reason, and
doing so gives up the last time-based control the contract has.

The adapter will not build with it set. A target that genuinely has no clock
defines `SOLIDSYSLOG_WOLFSSL_NO_VALIDITY_CHECK` to say so, which builds and
leaves the obligation knowingly unmet rather than quietly missing. Nothing
reports this at runtime, because by then there is nothing left to detect it
with - which is why the decision is forced at build time instead.

Without `NO_ASN_TIME`, wolfSSL reads the clock through `XTIME`, which is libc
`time()` unless your build says otherwise with `USER_TIME` and a definition of
its own. A newlib toolchain links `time()` against a stub that returns -1, and
wolfSSL then reads every certificate as outside its validity period - so a peer
that is plainly current is refused as not yet valid. The build will not tell
you; supply the clock. A coarse one is enough: X.509 asks only which side of a
window the device is on.
