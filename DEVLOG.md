# Dev Log

## 2026-05-03 — S18.02 BlockDevice abstraction + file-backed driver

### Decisions

- **Sliced 5, not 6.** Plan called for S3 (internal repoint) and S4
  (public flip) as separate slices. The first hit on dual-handle
  FileFake test setups proved that S3 alone forced every FileStore
  TEST_GROUP to construct two FileFakes anyway — same churn as S4. So
  combining them avoided a transitional FileBlockDevice-built-inside-
  FileStore hack and the storage bloat that would have come with it.
  Single commit, cleaner end state.
- **Acquire-on-resume safety.** BlockDevice.Acquire's contract is
  "ensure the block is empty after the call" (file-backed creates-or-
  truncates). The resume path in BlockSequence.Open MUST NOT Acquire
  the existing newest block — that block has unsent records. Resume
  flow: scan Exists, find newest, Size it. Cold-start flow: Acquire(0)
  to create the first block. The implicit O\_CREAT-via-Open the old
  code relied on becomes an explicit Acquire for the cold case only.
- **EnsureHandleOpenOnBlock consults SolidSyslogFile\_IsOpen.** Not
  just the cached isOpen flag. Cost is one extra method call per
  hot-path op, benefit is the BlockDevice tolerates external close
  (test corruption helpers exploit this; a flash driver might also
  benefit if the integrator does anything weird with the underlying
  file). The cached flag stays as a fast-path "already on the right
  block" check; the IsOpen call defends against staleness.
- **Fast-fail-on-Open semantics dropped.** The pre-S18.02
  FileStoreErrors group asserted that if Open failed during Create,
  subsequent Writes also failed (the store knew it was degraded).
  With BlockDevice's lazy reopen on every op, a transient open
  failure heals. This is strictly better behaviour. The two tests that
  encoded the fast-fail (`WriteReturnsFalseWhenNotOpen`,
  `HasUnsentReturnsFalseWhenNotOpen`) are gone; one new test
  (`TransientOpenFailureRecoversOnNextWrite`) asserts the recovery.
- **FileFake\_FailNext\* now takes the target fake.** The old form
  (no args, targets `lastCreated`) was ambiguous in the new dual-fake
  test setup — FailNextRead always landed on the writeFile because it
  was created last, but reads happen via the readFile. Per-target
  args fix it cleanly. Single-fake callers (FileFakeTest itself)
  updated trivially.
- **One BDD scenario, deliberately small.** Issue #235 listed
  "Dispose only on capacity pressure" as a baseline regression scenario.
  S18.03 will overturn it (dispose-on-empty), so this is a short-life
  pin. Kept the scenario per the issue spec; held off on a more
  ambitious lifecycle suite that S18.03 will own.

### Deferred

- **Acquire vs Exists ordering in `BlockSequence_Open` cold start.**
  Today: scan via Exists, then Acquire(0) if nothing found. Acquire's
  contract assumes the block is empty post-call, so on a fresh
  filesystem this is fine. After a partial-write crash before this
  story shipped, block 00 might exist with bad content — current
  resume path Sizes it. S18.03's Acquire-disposes-stale-content
  behaviour will close that gap.
- **`SolidSyslogFileStore_Destroy` no longer closes file handles.**
  The integrator's responsibility now (via
  `SolidSyslogFileBlockDevice_Destroy`). Two FileStore tests that
  asserted the old behaviour are gone; the equivalent assertion lives
  in `SolidSyslogFileBlockDeviceTest.DestroyClosesOpenFileHandles`.

### Open questions

- **Should BlockDevice grow an explicit Open-existing verb?** Today,
  Size on an existing block opens the underlying file as a side
  effect (via EnsureHandleOpenOnBlock). It works, but reads weirdly:
  "Size opens the file" is not what the consumer API name suggests.
  An explicit `OpenExisting` verb would be cleaner. Deferred — wait
  to see what S18.04's example flash device wants.

## 2026-04-30 — S07.05 origin SD: enterpriseId and ip

### Decisions

- **`enterpriseId` static at Create.** Mirrors the existing
  software/swVersion lifecycle. SMI Private Enterprise OIDs do not
  change at runtime so a callback would have been needless ceremony.
- **`ip` via two callbacks (count + at) instead of an iterator
  object or stateful single-callback.** Three shapes were considered.
  (A) "caller writes the entire ip-token list" pushes SD-PARAM syntax
  knowledge into integrator code. (B) count + at lets the library own
  the `ip="…"`-with-leading-space framing while the integrator returns
  one escaped value per index. (C) a single index-returning-bool
  callback would need a new `SolidSyslogFormatter_TruncateTo` primitive
  to undo a speculative `ip="` prefix when the callback signals end-of-list,
  which is scope creep when no other feature wants the truncate. (B)
  is what shipped: two function pointers, no formatter primitive
  changes, library owns the SD framing so integrators can't get it
  wrong. Single-threaded Format dispatch removes the count/at race
  concern.
- **Hybrid pre-format + per-message dispatch.** OriginSd previously
  pre-formatted the whole SD-ELEMENT at Create and Format() just
  copied the bytes. With ip per-message-callback, that breaks. Two
  options: (a) full per-message formatting like MetaSd (rebuild
  software/swVersion/enterpriseId on every message), or (b) keep the
  static prefix pre-formatted and splice ip in per-message. Picked
  (b) — escaping software/swVersion/enterpriseId per-message is real
  cost on embedded targets, while ip splicing is small additional
  per-message work. Implementation: `PreFormatStaticPrefix` writes
  everything except the closing `]`; Format emits prefix bytes, calls
  EmitIps, then emits `]`.
- **One example IP, multi-IP unit-tested.** The library emits
  multiple `ip="…"` per RFC 5424 §7.2 and
  SolidSyslogOriginSdTest::FormatIncludesMultipleIpsFromCallback pins
  it. The example wires a single IP because syslog-ng's `${SDATA}`
  macro renders the parsed-and-deduplicated view, so multi-IP
  wouldn't roundtrip cleanly through the syslog-ng BDD oracle. The
  step definition uses re.findall + membership so the BDD is forward-
  compatible if a future scenario asserts a multi-IP wire output via
  the OTel oracle.
- **Bare `[origin]` is RFC-legal.** Initially second-guessed whether
  zero IPs should drop the SD-ELEMENT entirely — checked RFC 5424:
  §6.6 ABNF `*(SP SD-PARAM)` permits zero, §7.2 marks all params
  OPTIONAL, no SHOULD requiring "at least one" (unlike meta in §7.1).
  So `[origin]` is emitted when nothing is wired; integrator should
  remove origin from their SD list if they don't want it.
- **MISRA single-exit reaffirmed; `static inline` on extracted
  helpers.** Two style concerns user corrected mid-story. Both are
  CLAUDE.md / `feedback_misra_leanings.md` rules I had been applying
  loosely. The OriginSd helpers (PreFormatStaticPrefix, EmitSoftware,
  EmitSwVersion, EmitEnterpriseId, EmitIps, EmitIp) all have
  body-wrapped-in-`if` (no early-return guards) and `static inline`
  qualifier. Pre-existing TimeQualitySd / MetaSd violations recorded
  in `project_misra_early_return_audit.md` for a sweep after this
  story merges.
- **Tidy caught two real issues that incremental builds would have
  hidden.** (1) `bugprone-easily-swappable-parameters` on the test
  fixture's `recreate(software, swVersion)` — NOLINTed with
  justification mirroring `ClockFake_SetTime`. (2)
  `cppcoreguidelines-pro-bounds-constant-array-index` on the `ip`
  test fake's dynamic indexing — switched to
  `std::array<const char*, 8>` with `.at()`.
- **Test fake state stays file-scope.** ExampleIps_Count /
  ExampleIps_At could have been factored into a Tests/Support fake;
  instead they live as file-scope statics in
  SolidSyslogOriginSdTest.cpp because only one test file uses them.
  Same call-out as MetaSd's sysUpTime fake from S07.06 — extract to
  Tests/Support if a second test file ever needs them.

### Deferred

- **Platform IP enumeration helpers (`SolidSyslogPosixIp` /
  `SolidSyslogWindowsIp`).** Considered shipping `getifaddrs` and
  `GetAdaptersAddresses` reference integrations; deferred because
  enumeration policy is opinionated (which interfaces? IPv4 only?
  link-local? scope-id formatting?) and varies per deployment. The
  library ships the callback shape; integrators provide their own
  enumeration. Example wires a static demo IP.
- **Multi-IP BDD via OTel oracle.** The library emits multiple
  `ip="…"` correctly but syslog-ng's `${SDATA}` deduplicates.
  Cross-runner `bdd-windows-otel` should preserve repeated PARAM-
  NAMEs through the kvlist-based collector format; explicitly testing
  multi-IP through that oracle is a follow-up if the BDD wants
  stronger validation.
- **MISRA early-return + `static inline` sweep across
  TimeQualitySd / MetaSd.** Tracked under
  `project_misra_early_return_audit.md` — separate refactor PR after
  this story merges.

### Open questions

- **When does an SD callback shape grow into an iterator object?**
  count + at is fine for "small bounded list" cases; for thousands of
  items the count-then-N-callbacks pattern is wasteful. None of the
  RFC 5424 SD-IDs land in that regime, but if a future custom SD
  needs streaming we'll want a proper iterator API.

## 2026-04-29 — S07.06 meta SD: sysUpTime and language

### Decisions

- **Per-message callback for `language`, not static-at-Create.**
  Hostname and processId are wired the same way and the user's real
  deployments include cases that switch language at runtime — shipping
  vessels with multilingual crews, instrumentation used across English
  / Spanish shifts. Static-at-Create would have been one line shorter
  and is ruled out by exactly those two examples.
- **Reuse `SolidSyslogStringFunction` for `language`, not a new
  `SolidSyslogLanguageFunction` typedef.** Same shape (callback writes
  into a `SolidSyslogFormatter*`), same lifetime contract, same escape
  responsibility — minting a parallel typedef would be cosmetic.
  Extracted the typedef from `SolidSyslogConfig.h` into its own header
  `SolidSyslogStringFunction.h` so `SolidSyslogMetaSd.h` can pull just
  the typedef without dragging in the application-level config. Mirrors
  the precedent of `SolidSyslogClockFunction` living in
  `SolidSyslogTimestamp.h` rather than `SolidSyslogConfig.h`.
- **Real `CLOCK_BOOTTIME` / `GetTickCount64` in the example, shape
  assertion in BDD.** Considered the alternative (a fake returning a
  known value plus a literal-equality assertion) — that would have been
  tighter but the BDD wiring would no longer match what an integrator
  would write. Unit tests already pin exact-value behaviour with the
  fake; BDD's job is to prove "real source feeds through the pipeline
  intact", and shape assertion (positive decimal integer) does that
  honestly.
- **Counter joins sysUpTime and language as optional.** RFC §7.3 marks
  all three OPTIONAL; the production code now handles every subset
  uniformly via `if (handle != NULL) { emit_prefix; emit_value;
  emit_quote; }`. The previous `Increment(NULL)` UB hazard is gone.
  The "all three NULL" empty-meta case is an RFC SHOULD violation we
  don't enforce — integrator typo, not a library problem.
- **uint32 wrap is the spec, not a workaround.** RFC 3418 `TimeTicks`
  is a 32-bit counter wrapping at ~497 days. The cast `(uint32_t)
  hundredths` at the end of the math pipeline gives that wrap for
  free; no overflow handling needed. The boundary tests pin
  `UINT32_MAX = 4294967295` and the wrap-to-4 case for proof.
- **Mid-flow correction on test rigour.** First slice-2 commit had
  one `FakeSysUpTime → 12345` test, which the user (correctly) flagged
  as insufficient — a hardcoded `sysUpTime="12345"` in production
  would have passed it. Reworked into a stateful fake plus four tests
  (12345, 99999, 0, UINT32_MAX) before continuing. Second mid-flow
  correction extracted `useSysUpTime(N)` as a TEST_GROUP helper after
  the same setup repeated four times, then a third extracted
  `CHECK_SYSUPTIME(expected)` as a `#define` macro mirroring the
  `CHECK_PRIVAL` / `CHECK_TIMESTAMP_*` convention in
  `SolidSyslogTest.cpp`. Both patterns now live in memory under
  `feedback_test_helpers_for_repeated_setup.md`.

### Deferred

- **CLOCK_BOOTTIME portability beyond Linux.** `CLOCK_BOOTTIME` is a
  Linux extension, not portable to macOS / BSD. The library currently
  targets Linux + Windows and the devcontainer is Ubuntu. If we ever
  add a non-Linux POSIX target, the natural fallback is
  `CLOCK_MONOTONIC` (POSIX-portable, "since some unspecified epoch" —
  RFC 3418-acceptable since "since management portion init" is
  satisfiable by any monotonically-increasing reading).
- **OTel BDD validation.** Slice 8 ran `behave` against the syslog-ng
  oracle in the devcontainer (3 scenarios pass). The OTel oracle path
  is exercised cross-platform in CI (`bdd-windows-otel`) — locally
  the syslog-ng oracle is sufficient because
  `_render_otel_structured_data` collapses both representations to
  identical `[meta key="value"]` text before the regex steps run.

### Open questions

- **Should `language` ever sanitise its own input?** Today the library
  trusts the caller's callback to produce a valid BCP 47 tag. The
  formatter handles SD-PARAM-VALUE escaping (`"`, `\`, `]`) but not
  BCP 47 tag-syntax validation. If a deployment supplies garbage we
  emit it verbatim (escaped). The unit tests cover the escape side;
  no test today covers "library produces garbage when callback gives
  garbage" because that's defined-correct behaviour. Worth revisiting
  if a future story adds typed-tag support.

## 2026-04-28 — S12.12 PR #218 wrap-up: BOM follow-up, BDD diagnosis correction, CodeRabbit cleanup

### Decisions
- **Tagged `udp_mtu.feature` `@windows_wip` rather than chasing a Windows
  fix in this PR.** The OTel collector's syslog receiver interprets
  BOM-less UTF-8 as Latin-1 (RFC 5424 §6.4 requires a BOM the library
  doesn't yet emit), and Windows loopback's ~65535-byte MTU never
  triggers WSAEMSGSIZE for the message sizes we can produce inside
  `SOLIDSYSLOG_MAX_MESSAGE_SIZE`. So the full-delivery scenario waits on
  the BOM work; the oversize scenario is permanently POSIX-only by
  virtue of loopback MTU. Both are documented in the feature header
  with the BOM gap tracked as S12.13 (#219).
- **The `store_capacity` BDD failures were a stale diagnosis, not a
  fresh bug.** The first instinct (twice) was to edit the feature file
  — bump `max-files 2 → 7 → 8` and add a "fills one file" step — on
  the theory that PosixMessageQueueBuffer's `O_NONBLOCK` mq_send was
  silently dropping a message under the post-bump 1500-byte payloads.
  That theory walked the symptoms cleanly enough to be plausible, but
  the actual cause was the MAX bump 512 → 2048 silently quadrupling the
  production `MIN_MAX_FILE_SIZE` clamp. The feature file's
  `max-file-size 520` clamps up to 2055 at runtime, so the same default
  short messages now pack ~16 records per file instead of ~4 — total
  capacity 32, no overflow with 10 sent, the discard policy never
  engages.
- **Compensate in the step layer, not the feature file.** Feature files
  describe externally-visible behaviour and form the contract with the
  PO; they shouldn't change as a side-effect of an internal constant
  bump. Fix lives in `step_file_store_enabled_with_config`: size each
  MSG to `SOLIDSYSLOG_MAX_MESSAGE_SIZE/5 - 50` bytes so ~4 records pack
  per (clamped) file, restoring the original test design. The
  `OLDEST`/`NEWEST` retention asymmetry that the prior session saw
  (OLDEST keeps `maxFiles` records, NEWEST keeps `maxFiles − 1`) is
  real but only visible when files hold one record each — at 4 records
  per file the asymmetry collapses and both policies retain exactly 7
  of 10 sent. The `1500-byte body + max-files=8` workaround was
  swimming against that exact corner.
- **`SOLIDSYSLOG_MAX_MESSAGE_SIZE` mirrored as a Python constant in
  the steps module.** The store_capacity scenarios are now MAX-coupled
  by design; mirroring the C constant at module scope makes the
  dependency visible at the only place a future MAX bump needs to
  update.
- **README multi-instance limitation restored, narrowed to specific
  senders.** CodeRabbit flagged the original wording as too narrow
  (it had been worded as if only platform backends carried the
  constraint). Restored a precise list — `SolidSyslogUdpSender`,
  `SolidSyslogStreamSender`, `SolidSyslogSwitchingSender` — so
  integrators get an actionable signal.
- **`iec62443.md` SL1 row updated for connected-UDP error surfacing.**
  S12.12 shifted UDP from blanket fire-and-forget to a connected-socket
  model that surfaces local kernel failures (EMSGSIZE, ECONNREFUSED).
  The "UDP is unreliable — messages may be lost silently" SL1
  limitation is still accurate at the network-protocol level (mid-
  transit drops remain silent), so the row keeps that caveat alongside
  the new error-surface description.

### Deferred

- **`PosixMessageQueueBuffer` mq_send EAGAIN visibility** is still
  worth doing as a follow-up — not because it caused the store_capacity
  failures (it didn't), but because silent buffer drops are a real
  observability gap for any future high-volume scenario. Tracked under
  E12 #31.

### Open questions
- None.

## 2026-04-28 — S12.12 UDP path-MTU clipping (slices 6–7, story complete)

### Decisions
- **Winsock parallel as a mechanical mirror of slice 5.** WinsockDatagram
  gained the same lazy-connect, `IP_MTU_DISCOVER`, EMSGSIZE-detection,
  and `IP_MTU` lookup as PosixDatagram. The seam was already
  established as bare `Winsock_*` symbols — extending it with
  `Winsock_connect`, `Winsock_setsockopt`, `Winsock_getsockopt`
  matched the existing pattern. No `WinsockTcpStream_*` namespace
  needed because those names weren't already in use by another
  Windows module.
- **`<ws2tcpip.h>` for the IP_MTU_DISCOVER family on Windows.**
  Windows scatters `IP_MTU` / `IP_MTU_DISCOVER` / `IP_PMTUDISC_DO`
  across `<ws2ipdef.h>`, which `<ws2tcpip.h>` pulls in. Without it
  the constants are undeclared. Win10+ exposes the same numeric
  values as Linux for the PMTUD policy enum, so the production code
  reads identically across platforms.
- **WSAGetLastError() instead of errno.** Winsock's per-thread last-
  error API is the equivalent of POSIX `errno`. Test fake gained
  `WinsockFake_FailNextSendtoWithLastError(int)` paralleling
  `SocketFake_FailNextSendtoWithErrno(int)`.
- **Bumped `SOLIDSYSLOG_MAX_MESSAGE_SIZE` 512 → 2048 in slice 7.**
  RFC 5424 §6.1 says receivers SHOULD support 2048 over UDP, and
  real-world syslog deployments routinely use 4–8 KB. The 512
  default was conservative for the original embedded target. The
  bump also unblocks the BDD path-MTU trim scenarios — at the new
  default a max-size message produces ~2 KB of wire bytes, which
  comfortably exceeds the docker-bridge MTU's 1472-byte payload
  limit and triggers a real EMSGSIZE on the wire. A future CMake
  override (E21 #217) lets memory-constrained MCUs reduce it, with
  the BDD scenarios gated on the configured size.
- **Test fakes derived from `SOLIDSYSLOG_MAX_MESSAGE_SIZE` rather
  than hand-tuned constants.** SenderFake / SocketFake / WinsockFake
  buffer caps and FileFake's per-file storage all auto-adapt now,
  along with FileStoreTest's TEST_BUF_SIZE and TEST_MAX_FILE_SIZE.
  TEST_MAX_FILE_SIZE includes `SOLIDSYSLOG_MAX_INTEGRITY_SIZE` so
  the tests stay correct under any integrity policy. The bump
  surfaced exactly the hand-tuning that the user had warned about
  while writing the original FileStore tests.
- **BDD trim scenarios use prefix-equality as the boundary check.**
  The oversize scenario asserts (a) received bytes < sent bytes and
  (b) `sent.startswith(received)` on the Python string form. The
  prefix property fails iff the trim left orphan continuation
  bytes — so a single, simple assertion catches "trim happened" and
  "trim ended on a codepoint boundary" together. Robust regardless
  of exactly which codepoint the trim landed on.
- **Filed E21: Port-Time Configurability (#217).** Captures the
  tunables (`SOLIDSYSLOG_MAX_MESSAGE_SIZE`, `SEND_TIMEOUT_*`),
  documentation needs, and the gating dependency from S12.12's BDD
  scenarios. Decomposition deferred until prioritised.

### Deferred
- **Switch `sendto` to `send` after connect** in PosixDatagram /
  WinsockDatagram. Slightly more idiomatic on a connected socket;
  kept the diff minimal because both kernels treat connected
  `sendto` with addr arg correctly. Easy follow-up.
- **`SolidSyslogUtf8.h` to `uint8_t`.** Whole-codebase cleanup once
  S12.12 lands; would also unlock dropping the `(char)` casts at
  UdpPayload's call sites. MISRA Rule 10.1 motivation.
- **Address-mismatch detection in the Datagram impls.** Currently
  `connect`s once per Open lifetime regardless of subsequent
  addresses passed to SendTo. Safe in practice (UdpSender controls
  lifecycle) but could be enforced if the contract were ever
  broadened.
- **CMake configurability of `SOLIDSYSLOG_MAX_MESSAGE_SIZE`** —
  tracked in E21 (#217). When that lands, the BDD trim scenarios
  need gating on the configured size being ≥ ~1500 to actually
  trigger EMSGSIZE on the docker-bridge test path.

### Open questions
- None.

## 2026-04-27 — S12.12 UDP path-MTU clipping (slices 1–5)

### Decisions
- **Slice the story before writing tests.** Six conceptual pieces in
  the story body decompose cleanly into 7 slices (pure helpers ×2,
  vtable widening, UdpSender retry, Posix wiring, Winsock wiring,
  BDD). Slice ordering is bottom-up — each slice can be reviewed and
  committed in isolation, the algorithm consumes the helpers, and the
  retry loop is exercised against a `DatagramFake` before any real
  socket plumbing lands.
- **Datagram contract widened with a 3-state enum, not bool +
  out-param.** `SOLIDSYSLOG_DATAGRAM_SENT / _OVERSIZE / _FAILED`
  matches the project's vtable style (no out-params), is faketable
  per-call, and keeps the dispatcher one-line. The `OVERSIZE` value
  was added to the public enum in slice 3 even though no producer
  emits it until slice 5 — that lets the retry algorithm in slice 4
  read against the final contract rather than a stepping-stone.
- **Lazy `connect()` in `PosixDatagram`, not address-on-Open.**
  Original story wording suggested PMTUD setup "after `connect`",
  which would have meant either changing `Datagram_Open` to take an
  address (vtable churn, mirrors `Stream_Open`) or `connect`-ing
  inside `SendTo`. Lazy connect on the first SendTo keeps the vtable
  shape stable and matches `UdpSender`'s lifecycle — `UdpSender` does
  Disconnect/Open on endpoint-version changes, so the connected
  address never silently drifts. Documented assumption rather than
  enforced address-equality check.
- **Forward-scan rejected, backward-walk kept for `_TrimToCodepointBoundary`.**
  Considered a forward scan that advances by codepoint length until
  the next overshoots `length` — would have been single-return,
  no early guards, and structurally simpler. Backward walk is O(1)
  vs forward scan's O(n) over the buffer, and the EMSGSIZE retry
  is the slow path so performance is irrelevant. Backward walk
  decomposes into `FindLastCodepointStart` +
  `LastCodepointExtendsPastCut` which read in plain English; that
  beats the cleverer forward scan on clarity.
- **DRY pass: extracted `SolidSyslogUtf8.h`.** UdpPayload was about
  to duplicate the formatter's byte-level classifiers
  (`IsTwoByteLead`, `IsThreeByteLead`, `IsFourByteLead`,
  `IsUtf8Continuation`, `IsValidUtf8SingleByte`). Extracted as five
  `static inline bool` classifiers in `Core/Source/SolidSyslogUtf8.h`,
  formatter migrated to use them in the same slice. RFC 3629
  validity guards (overlong, surrogate, above-Unicode) stay in the
  formatter — they're rule-encoding, not byte-classification, and
  only one consumer needs them.
- **`char` parameter type kept for now.** Header uses `char` (signed
  on most ABIs), matching the formatter's existing types. UdpPayload
  uses `uint8_t` and casts at the boundary. MISRA Rule 10.1 on
  bitwise ops on signed types is a flagged follow-up — flipping the
  shared header to `uint8_t` is a separate cleanup.
- **MISRA-leaning style applied throughout.** Single returns, fully
  bracketed `if/else`, `if-else if` chains terminate with `else`,
  `cppcoreguidelines-init-variables` honoured, uppercase `U`
  literal suffixes. The trailing redundant-`else` (when result is
  already initialised to the default) is acceptable to drop —
  initialisation already proves "all paths considered".

### Deferred
- **Slice 6 — `WinsockDatagram` parallel.** Lazy `connect()`,
  `IP_MTU_DISCOVER`, `WSAEMSGSIZE` → OVERSIZE, `getsockopt(IP_MTU)`
  with fallback. Needs `WinsockFake` additions paralleling slice 5's
  `SocketFake` work.
- **Slice 7 — BDD scenarios.** Loopback-MTU constraint to actually
  exercise the retry path; two scenarios per the story body
  (full-delivery-at-safe-payload + clean-UTF-8-truncation).
- **`SolidSyslogUtf8.h` to `uint8_t`.** Whole-codebase cleanup once
  S12.12 lands; would also unlock dropping the `(char)` casts at
  UdpPayload's call sites.
- **`PosixDatagram` switch from `sendto` to `send` after connect.**
  Slightly more idiomatic on a connected socket. Kept `sendto` to
  minimise diff; Linux's connected-`sendto` semantics are
  well-defined.
- **Address-mismatch detection in `PosixDatagram`.** Currently
  `connect`s once per Open lifetime regardless of subsequent
  addresses. Safe in practice (UdpSender controls lifecycle) but
  could be enforced if the contract were ever broadened.

### Open questions
- None.

## 2026-04-26 — S13.16 WindowsFile — file abstraction using `<io.h>`

### Decisions
- **MSVC `<io.h>` underscore wrappers, not Win32 `CreateFile`/`ReadFile`.**
  `<io.h>` keeps the same `int fd` model and `INVALID_FD = -1` sentinel
  that `SolidSyslogPosixFile` uses, so the two impls read line-for-line
  alongside each other. Win32 native (HANDLE, `INVALID_HANDLE_VALUE`,
  overlapped I/O signature differences) would have produced a much
  larger diff with no behavioural gain — the file abstraction is
  blocking + sequential by design.
- **`_O_BINARY` is mandatory.** Without it the MSVC CRT translates
  `0x0A` → `0x0D 0x0A` on write and strips `0x0D` on read. That would
  silently corrupt `SolidSyslogFileStore` data the moment a frame
  contained either byte. Added a regression test
  (`BinaryRoundTripPreservesNewlineBytes`) that writes a payload
  containing `0x0A` and `0x0D` and asserts byte-for-byte round-trip.
- **`_sopen_s` over `_open`.** Plain `_open` triggers MSVC C4996
  (deprecation in favour of safe-CRT variants) and the project's
  banned-API policy forbids re-introducing `_CRT_SECURE_NO_WARNINGS`.
  `_sopen_s(&fd, path, oflag, _SH_DENYNO, pmode)` is the non-deprecated
  equivalent; `_SH_DENYNO` matches POSIX `open()`'s default of no share
  restriction. `_close`, `_read`, `_write`, `_lseeki64`, `_chsize_s`,
  `_access`, `_unlink` do not trigger C4996 and are used directly.
- **Tests hit the real filesystem.** Same approach as
  `SolidSyslogPosixFileTest`. No UT_PTR_SET seam, no fake — just open,
  write, read, seek, truncate, exists, delete against a path under
  `GetTempPathA`. The MSVC POSIX-compat layer is what production uses
  in anger, so exercising the real CRT is the truthful test.

### Deferred
- **`SolidSyslogFileStore` wiring on Windows** — needs a threaded
  buffer to be useful end-to-end (single-task example would not
  exercise S&F). Story for that lands once the Windows threaded buffer
  is in place.
- **`_chsize_s` / `_lseeki64` return-value checks.** `SolidSyslogPosixFile`
  ignores the return values of `ftruncate` / `lseek` likewise; matching
  the same risk profile keeps the impls symmetric. If a future story
  adds error reporting to the `SolidSyslogFile` vtable both impls get
  upgraded together.
- **Cross-platform unification under a shared `SolidSyslogFile` impl.**
  Considered briefly — both impls would need conditional includes and
  the underscore-prefix divergence would litter the source. The
  per-platform impls are cleaner.

### Open questions
- None.

## 2026-04-26 — S13.09 WinsockTcpStream — Windows TCP transport

### Decisions
- **Replicated `PosixTcpStream` → `WinsockTcpStream` rather than extracting more
  common code.** The `SolidSyslogStream` vtable extracted in S13.08 is already
  the platform seam; the deltas (POSIX `int fd` vs Winsock `SOCKET`, `ssize_t`
  vs `int`, `struct timeval` vs `DWORD` ms for `SO_SNDTIMEO`, `MSG_NOSIGNAL`
  vs `0`, EINTR retry vs none) are exactly the kind of platform-specific
  noise the Stream abstraction was designed to keep out of `StreamSender`.
  A 1:1 port keeps each impl small, readable, and free of `#ifdef`.
- **Pruned the EINTR retry loop and `MSG_NOSIGNAL` flag.** Winsock `send()`
  has no signal-interruption semantics — there is no equivalent of POSIX's
  EINTR for blocking socket I/O — so `SendRetryingOnSignal` and the
  `WasInterruptedBySignal` helper would have been dead code on Windows.
  Likewise Winsock does not generate SIGPIPE, so `MSG_NOSIGNAL` becomes a
  plain `0`. The shape of the contract remains identical to Posix: any
  failure or short return surfaces as `Send` → false, the caller closes
  and reconnects, and store-and-forward replays the message on the fresh
  socket. `WSAETIMEDOUT` from `SO_SNDTIMEO` propagates via the same path —
  no `WSAGetLastError()` is needed.
- **Test seam namespaced as `WinsockTcpStream_*` instead of bare `Winsock_*`.**
  `SolidSyslogWinsockDatagram` already exports `Winsock_socket` and
  `Winsock_closesocket` as un-namespaced globals; defining them again in
  `WinsockTcpStream.c` would have triggered MSVC LNK2005. Considered
  renaming Datagram's seams to `WinsockDatagram_*` for symmetry — chose
  not to drag that into this story; per-impl prefixed seams are
  self-contained and the inconsistency is a small follow-up.
- **`SO_SNDTIMEO` value lives in the impl as a `DWORD` ms enum
  (`SEND_TIMEOUT_MILLISECONDS = 5000`)** matching the 5-second cap on
  Posix. Recording as the same future port-time CMake override candidate
  flagged in S12.08; no separate decision here.
- **Consolidated `SOLIDSYSLOG_TCP_DEFAULT_PORT` and
  `SOLIDSYSLOG_UDP_DEFAULT_PORT` into `Core/Interface/SolidSyslogTransport.h`
  in a prep commit on the same branch.** Both are platform-agnostic
  IANA/RFC facts (RFC 5426 §3.2 and RFC 6587 §3.2) and naturally pair
  with the `SolidSyslogTransport` enum already in that header.
  Previously TCP's lived on `Platform/Posix/Interface/SolidSyslogPosixTcpStream.h`,
  which would have meant duplicating the constant when adding the Winsock
  TCP stream. `SolidSyslogUdpSender.h` and `SolidSyslogPosixTcpStream.h`
  now `#include "SolidSyslogTransport.h"` so existing callers continue
  to see the constants transitively. `CLAUDE.md` public-headers table
  and `docs/rfc-compliance.md` updated to point at the new home.

### Deferred
- **Example wiring (TCP variant of `Example/Windows/SolidSyslogWindowsExample.c`)
  and BDD scenario** for Windows TCP delivery to syslog-ng. Both deferred to
  S13.10 (#136), which is already scoped for end-to-end exercise via the
  `windows-build-and-test` job. Today's S13.09 work delivers the unit-tested
  building block.
- **Renaming `WinsockDatagram`'s `Winsock_*` seam symbols to `WinsockDatagram_*`**
  for cross-impl consistency with the new `WinsockTcpStream_*` prefix. Flagged
  as a future tidy — no functional impact.
- **Adding `SolidSyslogPosixTcpStream.h` / `SolidSyslogWinsockTcpStream.h`
  (and the platform Datagram/Resolver headers) as their own rows** in the
  CLAUDE.md public-headers table. Today's CLAUDE.md update only widened the
  `SolidSyslogStreamSender.h` row to mention the new Winsock TCP option,
  matching the existing pattern. A broader docs cleanup adding rows for all
  platform impls is a separate housekeeping task.

### Open questions
- None.

## 2026-04-26 — S12.08 TcpSender error guards (socket(), partial write, SO_SNDTIMEO)

### Decisions
- **Fail-fast on any short return from `send()`, no internal retry loop.**
  Considered the SendAll-with-offset pattern but went simpler: a short return,
  `EAGAIN`/`EWOULDBLOCK`, or any non-`EINTR` error all surface as `Send` →
  false. The caller closes and reconnects on the next attempt; store-and-forward
  replays the whole message on the fresh socket. Trade-off: under a peer that
  always short-writes (rare on a blocking socket without `O_NONBLOCK`), the
  message can churn through replay. In return: clean per-call latency upper
  bound (≤ `SO_SNDTIMEO`), much smaller code surface, easier to port to a new
  Stream backend.
- **`SO_SNDTIMEO` set at `Open` (5 s, hard-coded).** Bounds the blocking
  window of any single `send()`. A wedged peer can no longer make a
  `SolidSyslog_Service` call hang indefinitely — the timeout fires as
  `EAGAIN`, propagates as `Send` → false, and S&F handles the rest. The
  5 s value is recorded as a candidate for a future port-time CMake
  override (the same shape we'd use for `SOLIDSYSLOG_MAX_MESSAGE_SIZE` on
  memory-constrained MCUs); for now it lives as an `enum` in the
  implementation file.
- **`EINTR` is the only retried errno.** Strictly a portability shim
  against systems without `SA_RESTART`. Documented inline so it doesn't
  get deleted as dead code in a future cleanup.
- **`MSG_NOSIGNAL` already in place.** No SIGPIPE work needed for
  POSIX/Linux. BSD/macOS would use `SO_NOSIGPIPE`, VxWorks 7 a per-thread
  mask — out of scope until a concrete porting target appears.
- **`getaddrinfo` NULL guard (item 1 in the original issue) was already
  fixed** by an earlier refactor that removed the `BuildAddress` helper.
  Existing tests `ReturnsFalseWhenGetAddrInfoFails` and
  `DoesNotFreeAddrInfoWhenGetAddrInfoFails` document the safe path. No
  code change for this item.

### Deferred
- **Port-time CMake override mechanism** for `SEND_TIMEOUT_SECONDS` and
  `SOLIDSYSLOG_MAX_MESSAGE_SIZE`. Both are good candidates; neither has a
  concrete user need yet. A future story can add the CMake plumbing
  (`-D` cache variables → preprocessor defines → `enum` defaults guarded
  by `#ifndef`).
- **Symmetric tests for the BSD/macOS/VxWorks SIGPIPE matrix.** Not
  testable without a VxWorks build; would slot under E13 cross-platform
  CI if/when a VxWorks target is added.

### Open questions for the blog source
- **What's the right "fast fail vs internal retry" line for embedded
  transports?** SendAll is correct for streams over reliable lossless
  links; fast-fail-and-replay is correct when there's a backstop (S&F)
  and you value latency predictability. The conventional advice is
  "always SendAll on TCP" — but that advice doesn't account for systems
  where the transport sits below an event loop with its own retry
  semantics. SolidSyslog's S&F is exactly that. Worth a paragraph.

## 2026-04-25 — S07.07 sequenceId rollover and zero-avoidance per RFC 5424 §7.3.1

### Decisions
- **Wrap policy lives in the library, not in the user's atomic seam.**
  `AtomicOps` exposes only `Load` and `CompareAndSwap`. The wrap-at-2³¹-1
  and zero-avoidance rules live in `SolidSyslogAtomicCounter_Increment`'s
  CAS loop. A user porting to a toolchain without `<stdatomic.h>`
  (e.g. older VxWorks 7) only has to implement two primitives — they
  cannot accidentally violate RFC by getting wrap wrong.
- **`Platform/Atomics/` for cross-platform C11 implementations,
  `Platform/Windows/` for host-specific.** `<stdatomic.h>` isn't POSIX
  — it's a C11 language feature available on GCC, clang, and modern MSVC
  — so it earns a sibling folder. Anything that requires Windows-specific
  primitives (e.g. `InterlockedCompareExchange` for legacy MSVC) lives
  in `Platform/Windows/` and CMake selects it in preference where
  available.
- **`uint32_t` end-to-end.** The seam, the storage, the RFC range, and
  `Increment`'s return type are all 32-bit. The original `uint_fast32_t`
  on `Increment` was incidental and is now narrowed.
- **sequenceId assigned at raise, not at transport.** Consciously
  interpreting RFC §7.3.1's "originator" as the application layer rather
  than the transport socket. SIEMs see one end-to-end loss-detection
  signal across the internal buffer, store-and-forward, and transport.
  Cost: a small reorder window under concurrent raise (adjacent IDs
  may invert). Documented in the RFC compliance row and the IEC 62443
  cross-reference.
- **Phased commits.** Branch carries four commits — seam (additive),
  StdAtomicOps default, consumer cutover (+ Windows path migration),
  legacy removal. Each is independently revertable. POSIX/clang/gcc
  validated locally; Windows path is plumbed but only validated under
  `msvc-debug` in a separate Windows clone session.

### Deferred
- **Pre-atomics / mutex-based AtomicOps.** Architecture supports it via
  the seam; no in-tree implementation ships with this story. Add when
  a concrete toolchain need arises.
- **Branch-coverage instrumentation.** lcov reports "no data found" for
  branches across the project — the CMake coverage preset enables
  branch coverage at the lcov level but no compiler-side `-fprofile-arcs`
  branch flags. Worth a separate housekeeping story.

### Open questions for the blog source
- **What's the right shape for an "extension point" header?** This
  story added `SolidSyslogAtomicOpsDefinition.h` (vtable-only, for
  implementors) without a sibling `SolidSyslogAtomicOps.h` (forward
  declaration only, for consumers). Existing extension points in the
  repo are split — `SolidSyslogBufferDefinition.h` vs `SolidSyslogBuffer.h`.
  Here the consumer (`AtomicCounter`) needs the full vtable to call
  through it, so it includes Definition directly, and there's no
  consumer for whom a forward-declaration-only header would be useful.
  Open question: should every seam ship both headers symmetrically for
  consistency, or only when there's a consumer that benefits?

## 2026-04-23 — S12.09 PRINTUSASCII validation for RFC 5424 header fields

### Decisions
- **New formatter primitive `SolidSyslogFormatter_PrintUsAsciiString`.**
  Sits alongside `BoundedString` and `EscapedString` in the same family
  — same call shape, same null-terminate discipline, same bounded-loop
  semantics. Validation lives in the primitive, not in
  `FormatStringField` or `FormatMsgId`, because MSGID goes through a
  different call path than the three callback-driven fields;
  primitive-level keeps the rule in one place and the adoption a
  one-line swap per call site.
- **Substitute non-PRINTUSASCII bytes with `?`.** Matches syslog-ng /
  rsyslog convention. Silent — no PRIVAL flag, no counter SD-ELEMENT.
  PRIVAL is `facility × 8 + severity`, not a flag carrier; flipping a
  severity bit would lose semantic information. SIEMs that care detect
  `?` themselves.
- **Signed / unsigned `char` both handled.** The RFC's PRINTUSASCII
  range `[33, 126]` implemented as
  `(value >= LOWEST_PRINTABLE_US_ASCII) && (value <= HIGHEST_PRINTABLE_US_ASCII)`
  — `'~'` is 126, so a `signed char` high-bit byte (negative) and an
  `unsigned char` high-bit byte (> 126) both fail the upper bound.
  No explicit `unsigned char` cast needed.
- **Same truncation semantic as `BoundedString`.** Substitution is
  1-byte-in / 1-byte-out, so `maxLength` bounds input consumed and
  output bytes identically. No 2× storage considerations like
  `EscapedString`.
- **Name registers split deliberately.** Public API
  `SolidSyslogFormatter_PrintUsAsciiString` echoes the RFC token
  `PRINTUSASCII`. Internal helpers and constants use the English
  descriptor "Printable" (`IsPrintableUsAscii`, `LOWEST_PRINTABLE_US_ASCII`,
  `NON_PRINTABLE_SUBSTITUTE`) because they read as prose in context.

### Deferred
- **Oracle round-trip BDD.** No parameterised fixture today that drives
  non-printable bytes through hostname/app-name/procid/msgid
  end-to-end. Same argument as S07.04 — cost of scaffolding exceeds
  benefit given how well unit tests cover the primitive. If a
  parameterised BDD fixture arrives later (E14 is one candidate host),
  PRINTUSASCII scenarios plug into it.

### Open questions for the blog source
- **What makes a primitive promotion-worthy?** `EscapedString` and
  `PrintUsAsciiString` both started as "do one thing at each char".
  They earned their place in the formatter family because (a) every
  call site that needed the rule could get it with a one-line swap,
  and (b) the rule was uniform across call sites. If either failed,
  you'd want the rule expressed inline or in a higher-level helper.
  That's a clean heuristic for future policy primitives (UTF-8 safe
  truncation next).
- **Duplication of idiom vs duplication of logic.** Three of the
  formatter primitives now share
  `while ((len < maxLength) && (source[len] != '\0'))`. Considered
  extracting — function pointer, macro, `MoreToFormat()` helper — all
  rejected: the repeated thing is a C idiom for bounded-nul iteration,
  the distinctive content is the one-line per-char dispatch, and
  future divergence (UTF-8 boundary-walk in S12.10) would fight a
  shared abstraction. Rule of three presumes repeated *logic*, not
  repeated *idiom*.

## 2026-04-23 — S07.04 escape RFC 5424 PARAM-VALUE specials

### Decisions
- **Escape is a formatter primitive, not an SD helper.** New
  `SolidSyslogFormatter_EscapedString(formatter, source, maxRawLength)`
  lives alongside `BoundedString` and the digit formatters. Avoids
  mixing dispatch and format-time plumbing in `StructuredData.c`, and
  gives E14 custom-SD writers the same primitive to compose into their
  pre-formatted storage.
- **`maxRawLength` bounds raw input, not output bytes.** Follows from
  the RFC's "the documented field limits apply to source content".
  Each of the escape tests uses `maxRawLength = strlen(input)` so the
  bound is only satisfied under raw-input semantics; an additional
  test (`EscapedStringMaxRawLengthBoundsInputNotOutput`) sets the
  bound below the input length and relies on the resulting output
  being 2×-not-1× the bound to prove the semantic.
- **SD pre-format storage sized for worst-case 2× expansion.**
  `OriginSd`'s storage moved from 115 to 194 bytes (193 content + null).
  Expressed as a sum of named parts (`ORIGIN_LITERAL_BYTES`,
  `ORIGIN_CONTENT_MAX`, null terminator) using the new
  `SOLIDSYSLOG_ESCAPED_MAX_SIZE(rawMax)` macro. Drift-protected by a
  worst-case test (48 × `]` + 32 × `"`).
- **Truncation in the main message buffer is the SIEM's concern.**
  Considered atomic-escape-pair behaviour to avoid dangling `\` on
  overflow; rejected because no rewind produces a valid message once
  we're inside an unterminated SD — all strategies (drop char / drop
  SD / drop message) produce RFC-invalid fragments, same as any
  truncated datagram. Don't smooth it over at the primitive level.

### Deferred
- **SD-NAME syntax validation** (RFC 5424 §6.3.2) — the three
  standard SDs have compile-time-constant names, so validation
  protects nothing until callers supply names. Moved to E14 (#64)
  with an explicit scope bullet; original story (#68) updated to
  reflect the deferral.
- **Oracle round-trip BDD for escape rules** — proving syslog-ng
  decodes our escaped output back to the intended value needs a
  parameterised example fixture. Deferred to E14 where caller-supplied
  PARAM-VALUE is the natural API surface to drive such scenarios.

### Open questions for the blog source
- **When does a sizing constant earn a macro?** Between "the magic 194"
  and "an enum computed from parts via `SOLIDSYSLOG_ESCAPED_MAX_SIZE`
  plus a null-terminator addend" there's a taste call about how much
  structure pays for itself. The DRY argument — E14 custom SDs will
  size the same way — is load-bearing here; for a one-off it would be
  over-engineered.
- **Test-double vs. oracle in test helpers.** `escapeEach` in the
  worst-case test is functionally the escape function, hand-written
  to match RFC 5424. Not circular (it's the specification oracle)
  but worth noting the smell: if the oracle drifts from the RFC, both
  the helper and the production code could be wrong together.

## 2026-04-23 — S19.02 widen SBOM product scope to include build contract and licence

### Decisions
- SBOM product scope widened from `Core/` + `Platform/` to
  `Core/` + `Platform/` + root `CMakeLists.txt` + `CMakePresets.json`
  + `LICENSE.md`. Rationale: the earlier scope matched the CLAUDE.md
  support tiers (Tier 1 Core, Tier 2 Platform) but missed two
  integrator-facing realities — the top-level build contract
  (CMakeLists.txt, CMakePresets.json) and the licence text
  (LICENSE.md). Tampering with either is a genuine integrity concern;
  not covering them left a gap a careful client would notice.
- Deliberately still excluded: docs, tests, examples, CI, dev-env,
  VSCode config, scripts, the SBOM template itself, and the root-level
  meta files (README, CHANGELOG, CLAUDE, SKILL, DEVLOG, .clang-format,
  .clang-tidy, .gitattributes, .gitignore, .release-please-manifest).
  Informational or agent-facing, not library source. Including them
  would inflate the scope without reducing integrity risk.
- Content-tree hash algorithm unchanged — just the pathspec list
  widens. Primary (git-based) and fallback (find-based) recipes both
  updated. Local double-check: both recipes produce the same hash for
  the new 110-file scope.

### Deferred
- None for this scope decision.

### Open questions
- None. Next rehearsal throwaway release will demonstrate the new
  hash value flows through cleanly, as before.

## 2026-04-23 — S19.02 swap source-tarball hash for content-tree hash

### Decisions
- Rehearsal of the signing pipeline against `v0.0.2-sbom-test` surfaced
  a reproducibility caveat: the original `source-sha256.txt` hashed the
  output of `git archive --format=tar.gz HEAD -- Core/ Platform/`. Git
  archive's byte output is not guaranteed stable across git versions —
  the runner (recent git) and a local WSL Ubuntu (git 2.25.1) produced
  different hashes for the same commit. Commit-SHA and signatures all
  matched, but the convenience hash didn't, which is exactly the kind
  of "why doesn't this match" conversation a client-facing asset
  shouldn't invite.
- **Swap to a content-tree hash.** Deterministic across git versions,
  archive formats, locales, and tooling. Algorithm:
  ```shell
  git ls-tree -r --name-only HEAD -- Core/ Platform/ \
    | LC_ALL=C sort \
    | while IFS= read -r path; do
        printf "%s  %s\n" "$(git show "HEAD:$path" | sha256sum | cut -d' ' -f1)" "$path"
      done \
    | sha256sum
  ```
  A consumer can reproduce with `git ls-tree` + `git show` + `sha256sum`
  + `sort` (LC_ALL=C) and have no sensitivity to `git archive` output
  formatting, gzip implementation, or locale collation.
- **Portable non-git fallback.** If a consumer has an extracted source
  tree but no git clone, `find Core Platform -type f | sort | ... |
  sha256sum` produces the same hash. Documented as the secondary path
  in `docs/security/release-verification.md`.
- **File renamed** `source-sha256.txt` → `source-tree-sha256.txt` so
  the filename signals the algorithm change. SBOM template property
  correspondingly renamed from `solidsyslog:source-hash-sha256` to
  `solidsyslog:source-tree-sha256`. No prior releases shipped the old
  name (both throwaway `v0.0.*-sbom-test` releases were deleted), so
  this is a clean replace — no backwards-compatibility concern.
- **Self-documenting file content.** The new `source-tree-sha256.txt`
  opens with comment-prefixed header lines (scope, commit, algorithm,
  pointer to the verification guide) before the hash itself. `#`-lines
  are ignored by `sha256sum -c` so the file remains machine-friendly.

### Deferred
- **Mode-aware content-tree hash.** Current form hashes contents only,
  not file modes. Source files are all `100644`; a mode flip on a
  source file would be unusual. If regulated use ever needs it, extend
  the format to `<mode>\t<content-sha>\t<path>` and document. Not
  worth the complexity cost today.

### Open questions
- None. Next rehearsal (throwaway pre-release) will re-verify the new
  path end-to-end; I'll drive that after this PR merges.

## 2026-04-22 — S19.02 SBOM signing and release-asset attachment

### Decisions
- Extended `sbom.yml` with a second trigger (`release: published`) and a
  second job (`publish`) that keyless-signs the SBOM and a source-hash
  file with cosign, then attaches four assets to the GitHub Release.
  `workflow_dispatch` remains as the rehearsal path — generates the SBOM
  and source hash, uploads as a workflow artifact, does no signing or
  release interaction.
- **Keyless signing via GitHub OIDC** (cosign + Fulcio + Rekor). Zero
  long-lived keys or secrets in the repo; every signature is a
  short-lived certificate issued to the workflow's OIDC identity and
  recorded in Sigstore's public transparency log. The `id-token: write`
  permission is on the `publish` job only, not the `sbom` job — keeps
  the generation path read-only.
- **Two-job split with narrow permissions.** The `sbom` job runs with
  `contents: read` only (no write permissions, no OIDC token). The
  `publish` job depends on it via `needs:`, only runs on `release`, and
  gets `contents: write` (for `gh release upload`) plus `id-token:
  write` (for cosign). Minimises permission surface on the workflow
  that runs on every rehearsal.
- **cosign signature bundles**, not raw `.sig` files. A `.bundle` is a
  single JSON blob containing signature + certificate + Rekor inclusion
  proof, which is what a downstream verifier needs for offline checks.
  Two files per signed artefact (original + `.bundle`) instead of three
  (original + `.sig` + `.pem`).
- **Source-hash file committed to the signed set.** A one-line
  `source-sha256.txt` captures the SHA-256 of
  `git archive --format=tar.gz HEAD -- Core/ Platform/` — the same hash
  value as the SBOM's `solidsyslog:source-hash-sha256` property. A
  verifier can reproduce from `git archive` at the release tag and
  cross-check. Caveat: `git archive` byte-output can vary slightly
  across git versions — documented as a gotcha in
  `docs/security/release-verification.md`.
- **Cert-identity convention:** signatures pin to
  `https://github.com/DavidCozens/solid-syslog/.github/workflows/sbom.yml@refs/tags/v<version>`.
  That binds the signing event to "this workflow, this repo, this tag"
  — the mechanism a verifier uses to tell apart a real SolidSyslog SBOM
  from any other CycloneDX document claiming the same name. Fork
  signatures, random-branch signatures, and impersonator workflows all
  fail verification.
- **Advisory-only rollout.** All signing + upload steps carry
  `continue-on-error: true` per E19 guidance — a signing-infrastructure
  outage at release time should not block the release itself. Tighten
  after the first real release (`v0.1.0`) demonstrates the pipeline
  works. Follow-on `chore:` PR.
- **New doc: `docs/security/release-verification.md`** — step-by-step
  guide for a downstream integrator, ordered the way they'd actually
  run verification (source-hash → SBOM signature → source-hash
  signature → optional SBOM re-validation). Includes "what verification
  does not tell you" so a reader doesn't over-interpret a green
  result.

### Deferred
- **`cosign attest`** (signed SLSA provenance statement on top of raw
  signatures). Natural next step; separate E19 follow-on.
- **Flipping `continue-on-error` off.** Deliberate. First real release
  gets to prove the pipeline works; the tightening is a `chore:` PR
  after that.
- **Backfilling signatures for old releases.** No prior releases exist
  (manifest was `0.0.0` before `initial-version: 0.1.0` pinned the
  first release target).

### Open questions
- None blocking. Live test will be the first `release: published` event
  — covered by the rehearsal plan in the S19.02 issue (#196). Plan is
  to cut a throwaway `v0.0.1-sbom-test` pre-release on `main`, verify
  all four assets land, verify `cosign verify-blob` passes, then delete
  the throwaway. After that the first real release (`v0.1.0`) will
  exercise it for real.

## 2026-04-22 — S19.01 SBOM rehearsal workflow (CycloneDX template + on-demand generate)

### Decisions
- First story decomposed out of E19 (#155). Two-story split for SBOM work —
  this one (`S19.01`) does template + on-demand generation + validation; the
  follow-on (`S19.02`) adds cosign signing and release-time attachment.
  Primary driver: a maintainer rehearsal workflow, runnable via
  `workflow_dispatch`, before next week's client conversation about SBOMs.
- **Format: CycloneDX 1.5 JSON** (primary; E19 locks this in, no SPDX).
- **Scope:** `Core/` + `Platform/` subdirectories. Initially drafted as
  `Core/` only, widened on review — `Platform/` code ships with the
  repo and the integrator consumes it as source. `Example/`, `Tests/`,
  `Bdd/`, `ci/`, `docs/`, `.devcontainer/`, `.github/` stay out of scope
  (reference / harness / infrastructure, not product). POSIX / Windows
  runtime facts stay as `metadata.properties`, not components (they are
  *deployment requirements*, not *shipped software*).
- **SBOM kind disclosed.** Added `solidsyslog:sbom-kind: product` as a
  top-level property so a consumer can tell this apart from the future
  build-env SBOM and source SBOM. Framing captured in
  `docs/security/sbom.md` as "three flavours, three questions".
- **OpenSSL declared as an optional dependency.** When
  `SOLIDSYSLOG_OPENSSL=ON`, the integrator links against OpenSSL for
  the reference TLS Stream. We don't bundle it, but transparency
  matters for CRA readiness, so OpenSSL appears in the product SBOM as
  `components[0]` with `scope: optional`, supplier = OpenSSL Project,
  and no version or licence pinned. Licence terms depend on which
  OpenSSL version the integrator chooses (Apache-2.0 for ≥ 3.0,
  OpenSSL + SSLeay for ≤ 1.1.1); the integrator's own product SBOM
  records the specific version and licence. `dependencies[]` wires the
  relationship so tooling can walk it.
- **Template-based generation, not tool-scanning.** The project is a
  pure-C library with no runtime dependencies — a static template with
  env-substituted placeholders for version / commit SHA / source hash /
  timestamp / serial UUID gives a more precise and more honest SBOM than
  `syft` or similar would (those over-report test fakes, build tooling,
  etc.). `envsubst` is given an explicit variable allowlist so no
  unrelated environment variables get expanded.
- **Validation: `cyclonedx-cli` (official OWASP tool).** Does JSON-Schema
  checks plus semantic validation (PURL syntax, SPDX licence identifier,
  external-reference type enum). `--input-version v1_5 --fail-on-errors`
  locks the validation to the spec version the template claims and fails
  the workflow if anything drifts. Binary pinned by release tag
  (`v0.30.0`) and SHA-256 checksum.
- **Licence identifier.** `PolyForm-Noncommercial-1.0.0` is on the SPDX
  list, so the library's licence can be expressed via the preferred
  `license.id` field rather than a free-form name. Good signal for
  regulated-industry consumers who parse SBOM licences.
- **Versions via release-please manifest.** The rendered SBOM's
  `metadata.component.version` is read from
  `.release-please-manifest.json`. Today that's `0.0.0` pre-first-release;
  when we cut 0.1.0 it flows through automatically.

### Deferred
- **Signing + release attachment → S19.02.** Out of scope here; this
  workflow is manual-only and produces a workflow artifact, not a Release
  asset.
- **Dependency scanning** — revisit only if the single-component
  assumption stops holding (e.g. if `Core/` ever gains a build-time
  dependency bundled into shipped headers).
- **SPDX output** — E19 says CycloneDX primary, no SPDX.

### Open questions
- None blocking. Follow-on (S19.02) will need a judgement call on whether
  cosign signing applies to the SBOM only or also to a source-tarball
  hash file; defer until we draft that story.

## 2026-04-22 — S03.14 BDD exercises TLS 1.3

### Decisions
- Tightened syslog-ng's TLS and mTLS receivers
  (`syslog-ng-full.conf`) to require TLS 1.3 only by adding
  `ssl-options("no-sslv2", "no-sslv3", "no-tlsv1", "no-tlsv11", "no-tlsv12")`
  to both `tls(...)` blocks. Existing `@tls` / `@mtls` / switching-TCP↔TLS
  scenarios remain unmodified and now prove 1.3 negotiation implicitly —
  they all pass because our TLS stream's minimum-version floor is 1.2
  (allows 1.3) and OpenSSL prefers the highest mutually-supported version.
- Added one new explicit scenario to `tls_transport.feature` tagged
  `@tls13`, with comment noting that message delivery here proves TLS 1.3
  negotiation because the receiver refuses anything else. The
  `syslog-ng-full.conf` is the contract; any future loosening of the
  receiver would turn the scenario back into a generic "TLS works" test,
  so the contract-via-config is self-documenting.
- No library code changes. Current code already negotiates 1.3
  transparently. 1.3-specific cipher pinning via
  `SSL_CTX_set_ciphersuites` is a separate concern; deferred to a future
  story if a deployment needs it. The existing 1.2 cipher-list pin is
  ignored under 1.3 (different OpenSSL API path) — no breakage.

### Deferred
- 1.3 cipher-suite pinning API exposure.
- Explicit assertion in BDD that reads the negotiated version from
  syslog-ng logs — not worth the template + oracle plumbing while the
  syslog-ng receiver config is the binding contract.
- Running the library against a 1.3-only server on a different OpenSSL
  version (e.g. boringssl) — separate TLS-backend story.

### Open questions
- None. E03 is now story-complete; closing the epic on merge.

## 2026-04-22 — S03.13 TLS backend composition via CMake

### Decisions
- Factored TLS sender construction out of `Example/Threaded/main.c` into
  per-backend files selected at CMake link time, matching the existing
  repo idiom of link-time composition (platform subdirectories,
  `POSIX_FAKES_SOURCES`, etc.). `main.c` now has zero TLS `#ifdef`s, no
  `SOLIDSYSLOG_HAVE_OPENSSL` compile define, and no knowledge of whether
  the TLS backend is real or a stub — it just calls
  `ExampleTlsSender_Create(resolver, mtls)` / `_Destroy()`.
- Two backends today: `ExampleTlsSender_OpenSsl.c` (the real wiring
  previously inside `main.c::CreateSender`) and
  `ExampleTlsSender_Unavailable.c` (nil-object stub). A third
  `ExampleTlsSender_WolfSsl.c` is deliberately not created here — the
  `_<Backend>.c` filename convention is the extension point for future
  backends; adding wolfSSL is its own story.
- Stub is a nil-object, not an exit-on-create. Under S03.12 all three
  switching-slot senders are created on startup regardless of the
  launch-time transport, so exiting from the stub's `_Create` would kill
  UDP/TCP launches too. The nil sender's `Send` prints a one-shot stderr
  message ("TLS support not compiled in — messages routed to the TLS slot
  will be dropped") and returns false; `Disconnect` is a no-op. Not a
  silent fallback (user sees the message), not an abrupt exit (UDP/TCP
  keep working), consistent with existing null-object conventions
  (NullBuffer, NullStore, NullSecurityPolicy). `warned` latch is reset in
  `_Create` so repeated Create/Destroy cycles stay quiet on subsequent
  Sends in the normal case but re-warn after any genuine re-setup.
- Dropped the draft `ExampleTlsSender_Available()` function — the
  nil-object approach makes it unnecessary. `main.c` doesn't branch on
  availability; it just wires whatever the linker gave it.
- `ExampleTlsConfig.c` and `ExampleMtlsConfig.c` link alongside the
  OpenSSL backend, dropped with the Unavailable one. The Unavailable
  build no longer compiles the TLS/mTLS config modules at all.

### Deferred
- Actual wolfSSL / mbedTLS backends — separate future story per-backend.
- Extending the same composition idiom to other transports (UDP / plain
  TCP) — only TLS has the optional-dependency problem today.
- Runtime backend registration / dynamic plugin loading — not needed in
  the foreseeable roadmap.

### Open questions
- None.

## 2026-04-22 — S03.12 multi-instance senders and streams

### Decisions
- Converted `SolidSyslogStreamSender`, `SolidSyslogPosixTcpStream`,
  `SolidSyslogTlsStream`, and `SolidSyslogPosixFile` from static-singleton
  `_Create(config)` / `_Destroy()` pairs to multi-instance with
  caller-supplied storage. `_Create` now takes a `SolidSyslog<X>Storage*` and
  the config; `_Destroy` takes the returned handle. Heap stays out —
  caller owns the memory, static / stack / pool allocation all work.
- Settled on one canonical "opaque storage" pattern for fixed-size classes
  and codified it in memory (`feedback_storage_pattern.md`): `SOLIDSYSLOG_<X>_SIZE`
  as its own enum with `(sizeof(intptr_t) * N) + sizeof(uint32_t) + ...`;
  storage typedef using ceiling-divided `intptr_t slots[...]`; static_assert
  `sizeof(struct) <= sizeof(storage)`; `DEFAULT_INSTANCE` / `DESTROYED_INSTANCE`
  + struct assignment in Create/Destroy. One spelling across the library,
  Formatter's variable-size macro remains the documented exception. Picked
  the `intptr_t slots` flavour over PosixFile's original `uint8_t opaque[...]`
  for pointer-alignment; realigned PosixFile in the same pass.
- Size formula authoring rule: each `sizeof(intptr_t) * N` slot covers
  pointer-scaling fields (function pointers, object pointers, `int`, `bool`,
  enums) so 16/32/64-bit targets pay only for what they need, and each
  explicit `sizeof(uint32_t)` / `sizeof(uint64_t)` term covers a fixed-width
  stdint field. Ceiling division in the slot count stops a stdint-tail size
  (e.g. StreamSender's `(sizeof(intptr_t)*7) + sizeof(uint32_t)` = 60 on
  64-bit) from silently under-allocating when a future field is added.
- Added exactly one new regression-net test per class:
  `CreateReturnsHandleInsideCallerSuppliedStorage` with
  `POINTERS_EQUAL(&storage, handle)`. That single check proves the storage
  contract and, by extension, multi-instance independence — didn't write a
  separate "create two, they don't collide" test.
- Threaded example rewired to always wire UDP + plain TCP + TLS
  simultaneously at launch. `--transport` now only sets the initial selector;
  runtime `switch` works across all three. New BDD scenario
  (`switching_transport.feature`) covers the TCP↔TLS path.

### Deferred
- Audit of the remaining static-singleton `_Create` pairs (UdpSender,
  PosixDatagram, MetaSd, OriginSd, TimeQualitySd, Crc16Policy,
  NullBuffer / NullStore / NullSecurityPolicy, GetAddrInfoResolver,
  AtomicCounter, FileStore, PosixMessageQueueBuffer, SwitchingSender)
  to the future allocation-strategy epic. S03.12 was the tactical enabler
  for TLS + TCP coexistence; the broader dual-API (heap vs caller-storage)
  redesign is out of scope.
- Stored-length vs null-termination tightening on Origin SD (project
  memory already records this).
- Error-reporting integration — still on E12.

### Open questions
- None — pattern is locked in via `feedback_storage_pattern.md`, ready to
  be applied consistently as further classes get converted under the future
  allocation-strategy epic.

## 2026-04-22 — S03.11 TLS promoted to Available + docs sanity pass

### Decisions
- Promoted TLS from Planned to Available in `docs/iec62443.md`: new
  `SolidSyslogTlsStream` row in the SL4 components table, present-tense
  bullets for what SL4 now adds, and a "Still to come" block carrying the
  remaining E17 (integrity + encryption at rest) and S12.x (string hygiene)
  items. Dropped the "Remaining E03 work" paragraph — there's no
  compliance-relevant E03 work left; S03.12 / S03.13 are architectural.
- Added CR 1.5 (Authenticator management) and CR 1.8 (PKI certificates) to
  the Relevant Requirements table at SL4 and wired both into the
  traceability matrix. Both are honestly claimed: library covers the bits
  it can (load / refresh / authenticity / integrity), and the notes call
  out the integrator-owned pieces (at-rest key protection is filesystem /
  HSM; revocation is deferred to the OS trust store per the S03.08 ADR;
  enrolment is the caller's PKI process). Better to state the gaps than
  to overclaim.
- README Status section rewritten: "Approaching feature-complete for POSIX
  and Windows"; not production-ready; explicit known-gaps list covering
  API churn, at-rest cryptography, CRL/OCSP, PRINTUSASCII / UTF-8 hygiene,
  and error-guard rollout. Keeps the "not production-ready" signal but
  stops undersellling where the library actually is.
- Sanity-checked the other repo markdown and fixed the drift that was
  visible: `docs/rfc-compliance.md` RFC 5425 section (all rows were
  Planned — now 6 Supported + 1 Partial for the absent
  `SOLIDSYSLOG_TLS_DEFAULT_PORT` constant); `docs/bdd.md` architecture
  diagram + syslog-ng source list + `@tls` / `@mtls` feature tags;
  `Bdd/syslog-ng/tls/README.md` now lists `client.key` / `client.pem` and
  mentions `mtls_transport.feature`; `docs/ci.md` gains
  `openssl-integration` and `bdd-windows` rows; `SKILL.md` lists RFC 6587
  and notes the epic-number range has grown beyond the original #2–#12.
- Epic #5 story table synced: S03.06 / S03.07 / S03.08 / S03.09 marked Done
  with PR links.

### Deferred
- `SOLIDSYSLOG_TLS_DEFAULT_PORT = 6514` constant — would promote the RFC
  5425 §4.2 row from Partial to Supported. Small, mechanical, not urgent.
- `docs/template-updates.md` claims `docs/` is template-owned. That
  conflicts with the reality that `docs/iec62443.md` /
  `docs/rfc-compliance.md` are clearly project-specific. Worth tightening
  the template-ownership list on a later pass.

### Open questions
- None.

## 2026-04-22 — S03.10 retired (cert rotation covered by reconnect)

### Decisions
- Closed S03.10 (cert rotation via version-fingerprint callback) as not
  planned. `SolidSyslogTlsStream` rebuilds `SSL_CTX` on every `Open` and
  re-reads cert material from disk, so replacing `client.pem` / `client.key`
  (or `caBundlePath`) and waiting for reconnect — or calling
  `SolidSyslogSender_Disconnect` — picks up new material without any
  callback plumbing.
- Maps cleanly onto IEC 62443-4-2 **CR 1.5 c** (authenticator
  change/refresh) and **CR 1.8 c** (PKI update) — both are *capability*
  requirements, satisfied by file-replacement-on-reconnect. A
  version-fingerprint callback would only add value for forced
  mid-connection rotation; no deployment has surfaced that need.
- `docs/iec62443.md` updated: rotation is now a first-class bullet in the
  TLS hardening section (with explicit CR 1.5 / CR 1.8 attribution),
  and the "Remaining E03 work" paragraph no longer lists S03.10 — only
  S03.11 (Planned → Available promotion) remains.
- Epic #5 story table updated: S03.10 row marked Retired with the same
  rationale. Removed stale "cert-version callback" mention from the
  Example architecture-boundary bullet.

### Deferred
- None. If a concrete deployment later needs forced mid-connection
  rotation, reopen a fresh story designed to that requirement.

### Open questions
- None.

## 2026-04-22 — S03.09 mutual TLS (SL4 CR 2.12)

### Decisions
- Added opt-in `clientCertChainPath` + `clientKeyPath` to
  `SolidSyslogTlsStreamConfig`. Both NULL = server-auth TLS (existing
  behaviour). Both set = mTLS. Exactly one set = Open fails — no silent
  downgrade. Implementation uses `SSL_CTX_use_certificate_chain_file`,
  `SSL_CTX_use_PrivateKey_file`, and `SSL_CTX_check_private_key` with
  each return value checked and ctx freed on any failure.
- Rejected a callback-based cert-rotation API for this story. The
  `SSL_CTX` is rebuilt on every Open, so on-disk cert rotation is
  picked up automatically on the next reconnect. S03.10 can revisit.
- `TlsTestServer` pins TLS 1.2 when client-cert verification is
  enabled. In TLS 1.3 the client's `SSL_connect` can return before the
  server has verified the client cert, which would hide a server-side
  rejection behind the next read. TLS 1.2 keeps the handshake
  synchronous so integration tests observe the rejection within Open.
- `TlsTestCert` now emits `basicConstraints=CA:TRUE`. OpenSSL 3's chain
  validation requires it on issuer certs; previously-working self-signed
  scenarios are unaffected.
- BDD adds a second TLS source to `syslog-ng.conf` on port 6515 with
  `peer-verify(required-trusted)`, leaving the existing 6514 source on
  `optional-untrusted`. Both paths stay under test.
- `ExampleSwitchConfig` maps `mtls` onto the TLS slot rather than
  introducing a fourth slot — the TlsStream is a singleton, so mtls is
  a flavour of tls rather than a parallel transport. New slot would
  wait on the multi-instance allocation-strategy epic.

### Deferred
- Password-protected client keys — OpenSSL's default pw callback
  prompts stdin, not what embedded callers want. Follow-on if asked.
- Cert-rotation callback — reconnect-driven rotation was judged
  sufficient. S03.10 may still exist as a doc+test story.

### Open questions
- None.

## 2026-03-28 — GitHub project setup

- Created `epic` label on DavidCozens/solid-syslog
- Created epic issues E0–E10 as GitHub Issues (#2–#12):
  - E0 (#2): Epic: Walking Skeleton
  - E1 (#3): Epic: Core Syslog Formatting
  - E2 (#4): Epic: UDP Transport
  - E3 (#5): Epic: TLS Transport
  - E4 (#6): Epic: Buffering
  - E5 (#7): Epic: Store and Forward
  - E6 (#8): Epic: Optional Header Fields
  - E7 (#9): Epic: Structured Data
  - E8 (#10): Epic: RTOS Examples
  - E9 (#11): Epic: C++ Wrapper
  - E10 (#12): Epic: Static Analysis and MISRA
- Created GitHub Project board "SolidSyslog" (project #1); all epics added
- No open questions

## 2026-03-28 — E0 walking skeleton completion

### Decisions
- E0 scope revised: BDD harness moved to E2, where there is something real to drive end-to-end
- E0 contains one story only: S0.1 repository setup
- README.md written with architecture summary and status notice
- SKILL.md added as standing brief for Claude Code sessions
- misra_suppressions.txt added — empty, version-controlled, ready for first suppression

### Deferred
- BDD harness (Behave/Python) — deferred to E2
- GitHub Issue story template — deferred, revisit before E1 decomposition

### Open questions
- None

## 2026-03-29 — E1 story decomposition

### Decisions
- E1 decomposed into 5 stories: E1.1–E1.5 (GitHub Issues #16–#20), created as sub-issues of Epic #3
- E1.1: Walking skeleton — single Log call produces a valid RFC 5424 message
- E1.2: PRIVAL encoding — facility and severity on the Log call
- E1.3: Timestamp — raise-time capture via injected clock function
- E1.4: Hostname, AppName, ProcessId — injected via config function pointers
- E1.5: MessageId and Message — driven onto the Log call
- `story` label created (#0075ca) to distinguish stories from epics on the project board
- All stories added to SolidSyslog project board under Epic #3; Epic #3 added to board
- Structured data fixed as `-` throughout E1 — real structured data deferred to E7

### Deferred
- GitHub Issue story template — still deferred, not needed yet

### Open questions
- ~~Boundary truncation policy~~ — resolved 2026-04-01: always send, truncate strings,
  consider adjusting PRIVAL to indicate the problem. See S1.2 entry.
- ~~Out-of-range PRIVAL handling~~ — resolved 2026-04-01: override to facility 5 /
  severity 3 (PRIVAL 43), send message with remaining fields as-is. See S1.2 entry.

## 2026-03-30 — Devcontainer SSH agent bind mount fix

### Decisions
- Added `initializeCommand` to `.devcontainer/devcontainer.json` to remove stale stopped
  containers before each devcontainer open
- Command: `docker compose --project-name "$(basename $(pwd))_devcontainer" -f .devcontainer/docker-compose.yml rm -f 2>/dev/null || true`

### Root cause
Docker Desktop on Windows/WSL assigns a UUID to each WSL filesystem bind mount internally.
When a devcontainer starts, that UUID is baked into the container's stored config. After a
Windows restart, Docker Desktop resets its bind mount registry and issues new UUIDs — but
the stopped container still references the old UUID, causing:
`error mounting "/run/desktop/mnt/host/wsl/docker-desktop-bind-mounts/Ubuntu/<UUID>" ... no such file or directory`

VS Code's "Reopen in Container" tries to restart the existing stopped container (`--no-recreate`),
hitting the stale UUID. "Rebuild Container" avoids it because it creates a fresh container.
The `initializeCommand` automates the equivalent of Rebuild by dropping stopped containers
before each open — running containers are unaffected so normal reopens have no friction (~80ms overhead).

### Cross-platform note
The `initializeCommand` is safe for all contributors. `docker compose rm -f` is standard
and `$(basename $(pwd))` is POSIX — on Mac and Linux it runs harmlessly as a no-op.
The UUID issue is Windows/WSL-specific but there is no cost to running the command elsewhere.

### Deferred
- `${SSH_AUTH_SOCK}` in `docker-compose.yml` has no fallback — if a contributor has no SSH
  agent running, `docker compose` will fail with an interpolation error. A `${SSH_AUTH_SOCK:-/dev/null}`
  guard would make the container start cleanly for those contributors (SSH just won't work).
  Low priority until a second contributor joins.
- This fix should be backported to the CppUTest devcontainer template so future clones
  don't encounter the same issue.

### Open questions
- None

## 2026-03-29 — E2 story decomposition

### Decisions
- E2 renamed from "UDP Transport" to "UDP Sender" — scope is the POSIX sender implementation only
- E2 decomposed into 4 stories: E2.1–E2.4 (GitHub Issues #22–#25), created as sub-issues of Epic #4
- E2.1: Walking Skeleton — PosixUdpSender transmits a buffer
- E2.2: UdpSender configuration — host and port injection
- E2.3: CMake platform detection — PosixUdpSender included conditionally
- E2.4: BDD walking skeleton — end-to-end UDP message
- PosixUdpSender.c included in build via CMake platform detection only — no #ifdef in source
- POSIX socket calls tested with hand-rolled SocketFake (strong-symbol fakes for socket/sendto/close) — no real network in unit tests; fff considered but rejected as an unnecessary dependency for three functions
- BDD harness deferred from E0 now lands in E2.4 as planned
- RTOS-specific sender implementations and non-POSIX build testing explicitly deferred to E8

### Deferred
- RTOS-specific sender implementations — deferred to E8
- Non-POSIX build testing — deferred to E8

### Open questions
- Port 0 behaviour: reject or default to 514? Decide before implementing E2.2
- Unresolvable hostname: error at Create time (fail fast) or at Send time (deferred failure)?
  Preferred: fail fast at Create time — matches alloc-failure pattern from S1.1. Confirm before E2.2.

## 2026-03-30 — E2 BDD infrastructure design

### Decisions
- syslog-ng chosen as BDD test oracle — RFC 5424 parser decomposes received messages into named
  fields; Behave asserts field by field, not on raw bytes. Catches conformance failures a string
  comparison would miss.
- syslog-ng runs as a third Docker Compose service in `.devcontainer/docker-compose.yml` alongside
  the existing gcc/clang devcontainer services. No Docker-in-Docker required.
- Hostname resolution in PosixUdpSender uses `getaddrinfo` — accepts both IP strings and DNS
  hostnames (e.g. `syslog-ng` service name from Docker Compose network).
- syslog-ng UDP source on port 5514 (unprivileged; no NET_BIND_SERVICE needed).
- syslog-ng output: key=value template, one line per message, written to
  `Bdd/output/received.log` via shared workspace mount. Behave reads directly.
- `flush_lines(1)` in syslog-ng destination config to minimise write latency.
- Behave test isolation via file-offset tracking: `before_scenario` records byte offset,
  steps read only new lines from that offset. No container restart between scenarios.
- Example binary (`Example/SolidSyslogExample.c`): minimal C program that creates a logger,
  sends one message via SolidSyslog+PosixUdpSender, exits. Behave invokes as subprocess.
  Lives under `Example/` as a user-facing reference, doubling as the BDD sender.
- CI: `docker compose up -d syslog-ng` before BDD step; same docker-compose.yml as devcontainer.
- SenderFake renamed from SpySender throughout — subject-first naming more idiomatic.
- E2, S2.2, S2.3, S2.4 GitHub issues updated with infrastructure design notes.

### Deferred
- TLS cert management for RFC 5425 BDD scenarios — self-signed CA, generation script
  to be version-controlled alongside Compose config. Deferred to E3.

### Open questions
- None

## 2026-03-31 — S2.4 BDD walking skeleton implementation

### Decisions
- syslog-ng validated as BDD test oracle — receives UDP syslog on 5514, writes parsed fields
  via key=value template to `Bdd/output/received.log` with `flush_lines(1)`
- syslog-ng normalises `Z` timestamps to `+00:00` via `$ISODATE` — BDD assertions match the
  normalised form, not the sent form
- Behave container published to GHCR (`ghcr.io/davidcozens/behave`) from new repo
  `DavidCozens/BehaveDocker` — Debian trixie-slim base matches cpputest image for glibc
  compatibility, includes git and openssh-client for devcontainer use
- Devcontainer switching extended to three services: gcc, clang, behave — single `"service"`
  change in `devcontainer.json`, `postStartCommand` guards cmake behind `BUILD_PRESET` check
- Ctrl+Shift+B runs `behave Bdd/features/` when in the behave container (no `BUILD_PRESET` set)
- VS Code extensions added: `ms-python.python` and `cucumber.cucumber-official`
- CI bdd job uses artifact handoff from `build-and-test` (upload/download-artifact v4) rather
  than rebuilding — tests the same binary that passed unit tests
- CI bdd job is advisory (`continue-on-error: true`) — not a required status check
- CI bdd job uses dedicated `ci/docker-compose.bdd.yml` without dev-specific volume mounts
- syslog-ng healthcheck (`test -S /var/lib/syslog-ng/syslog-ng.ctl`) in CI compose ensures
  readiness before Behave starts
- JUnit XML output from Behave displayed via `dorny/test-reporter` in PR check runs
- Compose logs captured on failure for CI debugging
- `Example/` added to clang-format CI check
- Test isolation via line-count tracking — `Given` records line count, `When` waits for a new
  line — avoids root-owned file permission issues with truncation

### Deferred
- syslog-ng image not pinned to SHA — using `balabit/syslog-ng:latest`. Pin when stability matters.
- MISRA C:2012 addon for cppcheck — future addition, not part of this story

### Open questions
- None

## 2026-04-01 — S1.2 story rewrite and design decisions

### Decisions
- S1.2 rewritten with BDD acceptance criteria — Gherkin scenarios replace the ZOMBIES
  unit test list as the story's acceptance criteria. TDD still drives the implementation
  underneath; BDD covers the observable end-to-end behaviour.
- Out-of-range PRIVAL handling resolved: invalid facility or severity → override both to
  facility 5 (syslog) / severity 3 (err), producing PRIVAL 43. The message is still sent
  with all other fields as-is. This makes the error observable via BDD and avoids silent
  data loss.
- Boundary truncation policy resolved: the library always sends a message regardless of
  input. Strings exceeding RFC 5424 field limits (HOSTNAME 255, APP-NAME 48, MSGID 32)
  will be truncated. In each case, adjusting PRIVAL to indicate the problem will be
  considered. Error reporting deferred to a later story.
- Example program CLI: `--facility` and `--severity` flags via `getopt_long`. All fields
  default to the S1.1 test defaults when omitted. Future stories add flags for their own
  fields (e.g. `--hostname`, `--message`). This approach scales to multi-message scenarios
  needed for buffering (E4) without premature design — the example can evolve as needed.
- Existing walking skeleton BDD scenario remains unchanged — the defaults match S1.1
  values so existing tests continue to pass without modification.

### Deferred
- Error reporting API — deferred to a later story
- Additional CLI flags for other message fields — deferred to E1.3–E1.5

### Open questions
- Example program unit tests — as the example grows (more CLI flags per story), should it
  get its own unit tests? Current decision is to rely on BDD coverage, but revisit if the
  example starts containing non-trivial logic or if BDD proves too coarse to catch bugs.

## 2026-04-03 — E4 Buffering epic

### Decisions
- E4 decomposed into 6 stories (S4.1–S4.6), GitHub Issues #50–#55.
- Buffer abstraction inserted between formatting and sending. `SolidSyslog_Log` writes to
  a `SolidSyslogBuffer` vtable; `SolidSyslog_Service` reads from it and sends via
  `SolidSyslogSender`. One message per Service call — caller controls the loop.
- NullBuffer: Write sends immediately via injected sender. Service returns false. Current
  single-task behaviour preserved as a special case.
- PosixMessageQueueBuffer: `mq_send`/`mq_receive` with O_NONBLOCK. Thread-safe with zero
  application-level synchronization. Kernel manages the queue.
- `SolidSyslogConfig` holds both `buffer` and `sender`. NullBuffer owns its own sender
  internally; real buffers use the sender on SolidSyslog for the Service path.
- `SolidSyslogBuffer_Read` signature: `bool Read(buffer, void* data, size_t maxSize,
  size_t* bytesRead)` — out-params for data, single-exit with bytesRead always initialised.
- Read return type is `bool` for now. May evolve to an enum (MESSAGE_SENT, NOTHING_TO_SEND,
  MESSAGES_LOST) when overflow handling lands in S4.5/S4.6.
- `volatile bool` for service thread shutdown (not `atomic_bool`) to avoid C/C++ atomics
  incompatibility in the shared header. Pragmatically correct for the single-writer pattern.

### Architecture — example restructure
- Example split into `SingleTask/` (NullBuffer, bare-metal model) and `Threaded/`
  (PosixMessageQueueBuffer, two pthreads). Shared code in `Example/Common/`: command line parsing,
  app name, UDP config, service thread loop.
- Example code tested via separate `ExampleTests` executable using PosixFakes link-seam:
  real SolidSyslog library, real UdpSender, real PosixMessageQueueBuffer — only POSIX system calls
  (socket, sendto, clock_gettime etc.) intercepted by strong-symbol fakes.
- Test fakes split: PosixFakes static lib (SocketFake, ClockFake) shared across test
  executables; SolidSyslog-level fakes (SenderFake, BufferFake, StringFake) compiled
  directly into library tests only.

### Test counts
- 182 library unit tests (SolidSyslogTests)
- 17 example unit tests (ExampleTests)
- 16 BDD scenarios (14 existing + 2 buffered delivery)

### Deferred
- Circular buffer (S4.5) — bare-metal targets, mutex injection, overflow handling
- Overflow notification (S4.6) — RFC-compliant messages-lost reporting
- Run common BDD scenarios against both executables — future parameterisation
- CI test result aggregation and trend tracking — Issue #60
- ExampleTests not yet run in CI — to be added

### Open questions
- None

## 2026-04-06 — E12.1 Remove dynamic allocation from SolidSyslog core

### Decisions
- SolidSyslog adopts single-instance static model (Grenning pattern 2: module
  with file-scope static state). `struct SolidSyslog` is `static` inside
  `SolidSyslog.c` — still opaque, still encapsulated.
- `SolidSyslog_Create(config)` returns `void`, copies config into static instance.
  `SolidSyslog_Destroy()` takes no arguments, restores nil functions.
- Null Object pattern for function pointers: `NilClock` and `NilStringFunction`
  are static functions inside `SolidSyslog.c`. The instance is initialised with
  nil functions at file scope. Create only overwrites non-NULL config values.
  Destroy restores nil functions. This eliminates all NULL checks from the
  formatting hot path — function pointers are always callable.
- `SolidSyslog_Log(message)` and `SolidSyslog_Service()` lose the handle parameter.
- `alloc` and `free` removed from `SolidSyslogConfig` — the logger itself has
  zero dynamic allocation.
- `ExampleServiceThread_Run` simplified — no longer takes logger handle.
  `ServiceThreadArgs` struct eliminated from threaded example.

### Test counts
- 224 library unit tests (SolidSyslogTests) — 4 removed (multi-instance/allocation), 1 added (nil function safety)
- 17 example unit tests (ExampleTests)
- 23 BDD scenarios (unchanged — behaviour identical)

### Deferred
- Allocation patterns for SD objects, buffers, senders — separate E12 stories
- Error handling for invalid config — subsequent E12 story
- Static allocation for SD objects (caller-provided storage) — E11 (#29)

### Open questions
- None

## 2026-04-06 — S7.3 origin structured data

### Decisions
- OriginSd pre-formats `[origin software="X" swVersion="Y"]` once at Create time.
  Unlike timeQuality (dynamic callback) or meta (incrementing counter), origin
  parameters are static for the lifetime of the logger.
- Fixed buffer (`char formatted[115]`) sized for maximum RFC 5424 §7.2 parameter
  lengths: software max 48, swVersion max 32, plus framing characters.
- Per-parameter truncation enforced at Create — strings exceeding RFC limits are
  silently truncated. NULL parameters rejected (Create returns NULL).
- `formattedLength` stored for future robustness (preferred over null termination),
  though not yet used by Format() — deferred to E12.
- enterpriseId and ip parameters deferred to new story S7.5 (#75).

### Test counts
- 227 library unit tests (SolidSyslogTests)
- 17 example unit tests (ExampleTests)
- 23 BDD scenarios (20 existing + 3 origin)

### Deferred
- enterpriseId and ip origin parameters — S7.5 (#75)
- Use formattedLength in Format() instead of null termination — E12 (#31)

### Open questions
- None

## 2026-04-06 — S7.2 timeQuality structured data

### Decisions
- SD config generalised from single `SolidSyslogStructuredData*` to array pointer
  (`SolidSyslogStructuredData**`) + count (`sdCount`). FormatStructuredData iterates
  the array, skips zero-length results, falls back to NILVALUE when no SD-ELEMENTs
  succeed — matching RFC 5424 ABNF.
- TimeQualitySd uses a dynamic callback (`void (*)(SolidSyslogTimeQuality*)`) rather
  than pre-formatting at Create time. Rationale: NTP failure can change `isSynced`
  mid-flight; timezone can change on mobile platforms (e.g. oil tanker setting local
  TZ at each port).
- Callback takes pointer parameter, not return-by-value — clearer ownership, embedded
  C idiom, opens the door for error returns. Existing `SolidSyslogClockFunction`
  (return-by-value) flagged for refactoring to match.
- `syncAccuracyMicroseconds` uses `SOLIDSYSLOG_SYNC_ACCURACY_OMIT` (enum, value 0)
  as sentinel for "omit from output". Unit in variable name, not in a comment.
- Interface Segregation: `SolidSyslogTimeQuality.h` (struct + callback typedef)
  separate from `SolidSyslogTimeQualitySd.h` (Create/Destroy API).
- BDD `@wip` tag mechanism added to CI: `--tags='not @wip'` in behave command
  (`ci/docker-compose.bdd.yml`). Allows landing BDD scenarios before implementation.

### Test counts
- 212 library unit tests (SolidSyslogTests)
- 17 example unit tests (ExampleTests)
- 18 BDD scenarios (16 existing + 2 timeQuality)

### Deferred
- SD array ownership: defensive copy at Create boundary for MISRA robustness — E12 (#31)
- Clock callback refactor: `SolidSyslogClockFunction` return-by-value → pointer
  parameter — cleanup after S7.2, before next story
- CodeRabbit suggestion to extract MetaSd test fixture — premature with Phase 2 changes

### Open questions
- None

## 2026-04-08 — Formatter extraction design

### Context

SolidSyslog.c is 482 lines. ~320 lines (66%) are RFC 5424 formatting functions —
a separate responsibility that should be extracted. Additionally, 16 of 24 format
functions receive no buffer size information, writing through raw `char*` pointers
with no overflow protection (MISRA C:2012 concern).

### Design decisions

**Formatter struct** — stack-allocated by `SolidSyslog_Log`, passed to all format
functions:
```c
struct SolidSyslogFormatter
{
    char*  buffer;
    size_t size;
    size_t position;
};
```

**Single write chokepoint** — all writes funnel through `FormatCharacter`, giving one
consolidation point for future overflow protection. Today it writes blindly (behaviour-
preserving); the subsequent story adds the bounds check.

**File structure:**
- `Source/SolidSyslogFormatter.h` — internal header (not in Interface/)
- `Source/SolidSyslogFormatter.c` — all 24 format functions extracted from SolidSyslog.c
- `Source/SolidSyslogFormat.h/.c` — slimmed to shared utilities (MinSize, DigitToChar)
- BoundedString and Uint32 move to Formatter or are updated to use Formatter*

**SD vtable signature change** — `Format(sd, char*, size_t)` becomes
`Format(sd, struct SolidSyslogFormatter*)`. Breaking change for SD implementers, but
no external implementations exist yet. Right time to do it.

**What stays in SolidSyslog.c:**
- Create, Destroy, Service, Log, instance, nil functions (~60 lines)
- Log creates the Formatter and calls `SolidSyslogFormatter_FormatMessage()`

**What moves to SolidSyslogFormatter.c:**
- FormatMessage and all 23 subsidiary format functions
- MakePrival, CaptureTimestamp, TimestampIsValid, and other helpers

### Open questions

1. **String callbacks** (getHostname, getAppName, getProcessId) — currently write
   directly into buffer. Options: (a) change callback signature to receive Formatter*,
   (b) keep temp buffer and copy, (c) callbacks write at formatter->buffer + position
   using raw pointer. Option (c) is simplest but leaks the Formatter's internals.

2. **BoundedString via FormatCharacter** — should BoundedString funnel each char
   through FormatCharacter (consistent, one overflow check point) or use a bounded
   loop with a single check (more efficient for embedded)? Leaning toward bounded loop
   with single remaining-size check — FormatCharacter is for structural characters,
   BoundedString is for bulk data.

3. **Story sizing** — the extraction is large (~320 lines moving) but mechanical. One
   story for the pure refactor, one for TDD-driven overflow protection. The refactor
   could potentially split further (extract timestamp functions first, then field
   formatters, then wire up Formatter struct) but this risks intermediate states where
   the code is half-extracted.

---

## 2026-04-09 / 2026-04-10 — E15 TCP transport and BDD infrastructure hardening

_Reconstructed 2026-04-21 from PR bodies, memory, and git log._

### What shipped

- **S15.1 TCP sender** (#91) — RFC 6587 octet-counting framing sender.
- **S15.2 connection failure and reconnection** (#93) — first explicit sender
  reliability work, reconnect-on-failure semantics.
- **BDD infrastructure hardening** (#94) — teardown reload, wait-for-prompt,
  syslog-ng startup fail-fast.
- **S5.3 file-based store** (#98) — first file-backed store implementation and
  sender-outage BDD.
- **Prove string callbacks are invoked per Log** (#92) — tightens the contract
  so tests can't silently pass if the library caches values it shouldn't.

### Collaboration note

The BDD hardening PR (#94) accumulated items deferred from S15.2 — a pattern
that recurs: reviews flag real issues, the author keeps the story tightly
scoped, and the deferred items surface either as a follow-up infrastructure
PR or as entries in the robustness backlog memory. Worth noticing: the
"defer to follow-up" default holds only as long as the robustness-backlog
memory doesn't rot.

---

## 2026-04-10 — MISRA and function-ordering conventions formalised

_Reconstructed._

Two code-style feedback memories landed this day
(`feedback_misra_style.md`, `feedback_function_order.md`) formalising rules
that had been implicit:

- **Single exit point** per function, **positive logic** in conditions,
  **no magic numbers** (named constants with documented derivation).
- **Top-down function ordering**: Create/Destroy first, then public fns, then
  static helpers _directly below the function that first calls them_.
  Optimised for human reading depth-first rather than alphabetic.

### Collaboration note

This is a pattern worth naming: _repeated correction → once it is a rule,
capture it as feedback memory so future-me doesn't need the correction
again_. The cost of writing the memory is lower than the cost of the next
correction. Later sessions show the rules sticking — new C modules arrive
in roughly the right shape without prompting.

---

## 2026-04-11 / 2026-04-12 — Store & Forward epic (E5) completes

_Reconstructed._

### What shipped

- **S5.4** Power-cycle replay BDD (#100)
- **S5.5** File rotation with discard policies (#103) — plus fix for filename
  buffer overflow on long path prefixes (#104)
- **S5.6** File-store corruption detection and recovery (#106)
- **S5.7** Halt callback on storage full (#107)
- **S5.8** Halt-policy BDD scenarios and example CLI plumbing (#109)
- **Release versioning reset to 0.x** (#99) — admits pre-1.0 status explicitly

### Collaboration note — TDD discipline reset

S5.5 Phase 1 and Phase 4 triggered two documented TDD violations (captured
in `feedback_tdd_no_untested_code.md`). I wrote whole sub-systems (FileFake
rewrite, new FileApi operations, new state fields) _to unblock later tests_
rather than one failing test at a time. Much of that code had no failing
test driving it individually, even though the aggregate suite passed at the
end.

The reset after the second incident was specific: _"When facing a test that
requires infrastructure, write the ONE test that fails; the MINIMUM code to
pass, even if ugly; the NEXT test; more infrastructure only as it demands."_
The instinct to batch infrastructure is strong exactly when the rule most
matters — "quick refactor" mode is where discipline slips. A prior incident
in S05.01 (2026-04-06, `sendto >= 0` returned from UdpSender without a test
for the failure path) had already flagged the same class of error.

If there's a blog thread in this, it is: _agent-paired TDD is effective in
proportion to how strictly you hold the loop_. Violations don't look like
violations at the time — they feel like momentum. The green bar covers them.

---

## 2026-04-13 — Formatter extraction and buffer-overflow protection

_Reconstructed._

- **S12.2** Formatter extraction (#113) — the design discussed in the
  2026-04-08 entry above; every write now funnels through `WriteChar`.
- **S12.3** TDD-driven buffer overflow protection at `FormatCharacter` (#114) —
  bounds-checking added test-first at the new chokepoint.
- **CLI numeric validation fix** (#110) — CodeRabbit flag from #109 addressed
  with `ParsePositiveNumber` helper; six negative/non-numeric/trailing tests.

### Collaboration note

S12.3 is a clean example of "design-before-code, then strict TDD". The
extraction (S12.2) created the overflow-protection seam _on purpose_ — we
didn't need the protection yet, but naming it as a subsequent story made
the extraction scope sane. The reward was a tight S12.3: one chokepoint,
one test-driven guard, nothing else. Shape worth repeating.

---

## 2026-04-14 — IEC 62443 and RFC compliance docs + C99 baseline

_Reconstructed._

### What shipped

- **docs/iec62443.md** (#122) — client-facing mapping of library components
  to IEC 62443-3-3/4-2 requirements across SL1–SL4, with traceability
  matrix and links to open stories for SL4 gaps.
- **docs/rfc-compliance.md** (#122) — sender-side RFC 5424/5426/6587/5425
  compliance matrix.
- **`feedback_c99_target.md`** captured — C99 baseline, C11 only via
  injection (atomics), in anticipation of older embedded toolchain reach.

### Why

Client-visible capability documents needed for the "Crafted with AI" blog
series audience and for demonstrating suitability to industrial buyers. The
C99 commitment ties to the same audience — legacy embedded compilers tend
not to track the latest standard cleanly.

---

## 2026-04-15 — Windows MSVC CI begins (E13 kick-off)

_Reconstructed._

- **S13.1** Windows MSVC CI build (#124)
- **S13.2** C99 portability — portable static assert, MSVC suppressions
  documented (#130)
- **S13.3** Replace Microsoft SDL banned APIs with portable bounded
  alternatives (#131) — `SafeString` abstraction, banned-API policy in CLAUDE.md

### Collaboration note

First portability PR in a new direction surfaces a batch of related issues;
subsequent PRs in the same series get cleaner because the patterns are
established. `SafeString` formalised the "test code uses a `SafeString_Copy`,
production code uses the `Formatter`" split and that stuck.

---

## 2026-04-17 — Platform reorganisation and abstraction extractions

_Reconstructed. Structural refinement day._

### What shipped

- **Platform/<OS>/ reorganisation** (#143) — introduced top-level `Platform/`
  tree so Windows transport work had a stable structural home rather than
  mixing with POSIX and being reorganised afterward.
- **S13.6** Resolver abstraction extracted from UdpSender and TcpSender (#137)
- **S13.7** Datagram abstraction extracted from UdpSender (#139)
- **S13.8** Stream abstraction extracted from TcpSender (#140)
- **S13.11** Opaque `SolidSyslogAddress` to hide `sockaddr_in` (#145)
- **S12.11** Honest error reporting for Resolver and Datagram vtables (#144)
- **S13.4** Winsock UDP transport — Winsock Resolver + Datagram (#149)
- **S13.13–S13.15** Windows clock, hostname, and process-id helpers (#150)

### Collaboration note — "land the structure before the feature"

The platform reorg landing _before_ the Windows port proper is the key
refinement this day. Discussed in the conversation leading to #141 and
PR `#142`. The lesson applies more broadly: when a structural change and a
feature change both want to happen, land the structure first so the
feature drops in without carrying reorg noise. Same rationale surfaced
again at the Core/ rename (PR #180, 2026-04-21).

### Why

Three `Platform/*/` abstractions (Resolver, Datagram, Stream) extracted
the platform-agnostic contract from what had been a POSIX-locked
implementation. Windows could then come in as a peer rather than a graft.
Each abstraction was its own TDD story; the graft would have concealed
those seams.

---

## 2026-04-18 — Windows end-to-end: BDD on Windows

_Reconstructed._

- **S13.5** (five PRs across the day, #152–#159) — BDD features tagged by
  capability, `bdd-windows` GitHub Actions job, Windows example executable,
  scenarios promoted to the Windows runner progressively.

### Collaboration note — small-PR vs one-PR call

The cost of 5 PRs vs 1 is meaningful (context switches, review overhead),
but so is the benefit — bisectable if something breaks a month from now.
For infrastructure where a regression could mean "Windows CI is red for
days", small PRs pay for themselves. For a pure refactor that either
builds or doesn't, one PR is usually fine. The heuristic: _could this
specific slice break something subtly and only be noticed later?_ If yes,
slice it.

---

## 2026-04-19 — Phase-gated TDD convention + S3.4 lazy init

_Reconstructed. Biggest working-practice refinement of the project so far._

### What shipped

- **S3.4** Lazy initialisation and reconnection on UdpSender and TcpSender
  (#161) — introduced `SolidSyslogEndpoint` (host/port supplier +
  version-fingerprint callback), `Disconnect` vtable op, lazy open on first
  Send. Foundation for E20 and a future reconfiguration epic.

### Refinement — phase-gated workflow and BDD-first

`feedback_tdd_and_review_style.md` grew substantially this day with new
working conventions:

- **Phase-gated workflow for larger stories** — break work into phases,
  stop at each for code review, no commit until reviewed, no next phase
  until David says so. PR #161 (S3.4) was scoped across multiple phases
  with this review rhythm.
- **BDD first** — write Gherkin scenarios and pending step definitions at
  the start of a story, before implementation. Steps go green progressively
  as phases land. Working solo with Claude means front-loading what a
  tester would otherwise write in parallel.
- **Autonomy semantics** — when told "let me see what you can do by
  yourself", run the full TDD cycle without pausing at each step, but
  still stop _before_ committing for review.

### Collaboration note

All three conventions push in the same direction: _stop more often_.
Phase-gated stops protect design decisions. BDD-first stops the
temptation to implement before acceptance criteria exist. Even under
autonomy, the pre-commit stop preserves veto. The bar for "paused
collaboration" has moved steadily toward more pauses, not fewer. This is
probably the single most important meta-refinement of the project so far.

---

## 2026-04-20 — S3.6 StreamSender rename, S20.1 SwitchingSender, PR "Closes" keyword

_Reconstructed._

### What shipped

- **S20.1** SwitchingSender (#163) — selector-driven multi-transport
  switching; first new pattern under E20.
- **S3.6 rename** `SolidSyslogTcpSender` → `SolidSyslogStreamSender` (#166,
  breaking change) — reflects that the module frames bytes over any
  Stream, not just TCP. Sets up S3.7 TLS as a Stream drop-in.

### Refinement — `Closes #<issue>` keyword in PR bodies

After PR #163 (S20.1) squash-merged, issue #162 (the story) and #160 (the
parent epic) were left open because the PR body said _"Implements story
S20.01 (#162)"_ — descriptive but not a GitHub close-keyword. Had to
close manually.

Captured as an addendum in `feedback_pr_template.md`: **each PR body's
Purpose section must use `Closes #<issue>` (or `Fixes` / `Resolves`)**.
If the PR also completes a single-story parent epic (like E20), close
both.

### Collaboration note

Small process rule born from a small embarrassment. Telling because the
workflow had operated for weeks without it — story-closing mostly worked
because Claude kept noticing and closing manually. Once the PR template
got the keyword discipline, the "manual close" step disappeared. Another
case of _the cost of writing it down is lower than the cost of the next
occurrence_.

---

## 2026-04-21 (morning) — S3.7 TLS walking skeleton

_Reconstructed._

### What shipped

- **S3.7** (#170) — `SolidSyslogTlsStream` (OpenSSL), TLS BDD scenario,
  first end-to-end syslog-over-TLS into `syslog-ng`. TLS lands as a Stream
  per the architecture set up in S3.6.

### Scope note

The walking-skeleton is deliberately minimal: OpenSSL defaults for
hostname verification and cipher selection, knowingly leaves mid-`Open`
cleanup imperfect. Explicit validation, cipher hardening, and cleanup
move to S3.8 (created as #172). mTLS, cert rotation, docs promotion
queued as S3.9 / S3.10 / S3.11.

### Collaboration note

The walking-skeleton discipline is holding: _make one scenario green,
capture every known imperfection as a distinct follow-up story, defer all
of them_. The PR review surfaced several deferrable items which became
the S3.8 scope rather than creeping into S3.7.

---

## 2026-04-21 (afternoon) — Chore catchup session + SKILL.md reading gap

Session scope: a deliberate "low-effort tidy" phase while E3 TLS
hardening isn't ready to start. Landed five PRs in one afternoon.

### What shipped

- **#176** — Codified three GitHub workflow conventions in CLAUDE.md:
  - **Issue / epic linking** via GraphQL `addSubIssue` (plain-text
    `Parent epic: #N` lines don't create GitHub's native sub-issue edge,
    and the board roll-up and swimlanes depend on that edge).
  - **Project board membership**: epics are never items (they'd duplicate
    as orphan rows and as swimlanes); stories always are. Stable project
    and field IDs baked into the recipe.
  - **Zero-padded issue numbers** (`E03`, `S03.07`) so GitHub's
    alphabetic title sort also sorts numerically.
- **#177** — Closed the last coverage gap in `SolidSyslogFileStore`
  (null-object fallback when `securityPolicy->integritySize >
  SOLIDSYSLOG_MAX_INTEGRITY_SIZE`). Behavioural test asserts on-disk
  record = `[magic][length][body][sent_flag]`, no integrity gap.
  Coverage back to 100%.
- **#178** — `SOLIDSYSLOG_TCP_DEFAULT_PORT` corrected from `514` to `601`
  per RFC 6587 / IANA, with a pinning test. CodeRabbit flag from #166.
- **#179** — Four `FailNext*OnlyAffects*` tests across FileFake and
  SenderFake strengthened with `CHECK_FALSE` on the injected call, so
  they prove what their names claim.
- **#180** — Top-level `Interface/` and `Source/` relocated under `Core/`
  in a pure two-commit rename (`git mv` at 100% similarity, then
  reference updates). CLAUDE.md got an explicit **support tiers** table
  (Tier 1 `Core/`, Tier 2 `Platform/`, Tier 3 `Example/`, out-of-scope
  `Tests/` / `Bdd/` / docs / CI).

Also restructured the GitHub project board: 37 orphan story items added
with correct Status, 11 epics removed-as-items (they render as swimlanes
via Parent-issue grouping), stale In-Progress on #65 flipped to Done.
Renamed 76 issue titles and cross-refs to the padded format. Created
S3.9 / S3.10 / S3.11 as placeholder issues under E3.

### What went well

- **TDD red-step discipline held even for pure backfill coverage.**
  Coverage gap in FileStore: commented out the production line,
  confirmed SIGSEGV exit 139, uncommented, green. Similar pattern for
  the TCP port pin test (red before the enum flip). Makes "the test was
  green from the start" genuine rather than performative.
- **Review-before-execute caught a bad test design.** First proposal
  for the FileStore coverage test was spy-observation (assert the
  over-size policy's `Compute` function was not invoked). User rejected
  in favour of a behavioural assertion (on-disk record layout shows no
  integrity gap). The rejected test would have been brittle to
  implementation changes; the behavioural test survives them.
- **GraphQL automation for GitHub state** made the project-board audit
  and repair feasible as a single chore rather than many UI clicks.
  Adding it to CLAUDE.md as a standard recipe means the next session
  won't rebuild the workflow.

### What didn't — three real misses in one session

- **SKILL.md not read at session start.** The DEVLOG cadence is defined
  in SKILL.md (_"append an entry … after every meaningful session"_).
  I didn't read SKILL.md. The user asked _"why did you stop updating the
  devlog?"_ while waiting for CodeRabbit on #180. Honest answer: I
  never started in this session; the memory-based workflow had quietly
  replaced it. Fix: CLAUDE.md now points to SKILL.md so any future
  session that auto-loads CLAUDE.md gets the pointer, plus a memory
  entry for belt-and-braces.
- **Audit regex for the Core/ rename too narrow.** Grepped for
  `Interface/` / `Source/` with trailing slashes; missed the
  `find Interface Source Tests Example` invocation in the `format` CI
  job that uses them as bare directory names. CodeRabbit caught it on
  PR #180 — would have broken the `format` required status check on
  merge. A third commit on the rename branch fixed it pre-merge.
  Lesson: for rename audits, search for the name with multiple
  delimiters (`/`, end-of-word, end-of-line).
- **Story renumbering first pass missed 7 issues.** Filtered by label
  `epic,story` — the oldest E4 stories (`S0.1`, `S4.1`–`S4.6`) predated
  the labelling convention and weren't tagged. Only noticed when the
  user pointed at remaining unpadded titles on the board. Second pass
  used a title-pattern search across all labels, caught them, and
  added the missing labels so this can't recur.

### Decisions captured in CLAUDE.md today

- Sub-issue linking uses the GraphQL `addSubIssue` mutation (textual
  `Parent epic: #N` is human-readable noise, not machine-authoritative).
- Epics never appear as board items; Parent-issue grouping renders the
  swimlane automatically when any child story is on the board.
- Zero-padded issue numbers (`E03`, `S03.07`) in titles and bodies —
  commit messages and merged PR titles stay unpadded (no history rewrite).
- Support tiers: Tier 1 `Core/`, Tier 2 `Platform/`, Tier 3 `Example/`,
  out-of-scope `Tests/`, `Bdd/`, `docs/`, CI, build tooling.

### Deferred

- E0 swimlane appears even though its only story (#15) is archived on
  the project — archived items still participate in Parent-issue
  grouping. User prefers leaving archived items on the project for
  insight-graph value; the swimlane persistence is an accepted cost.
  May revisit.
- Template-repo backport of the `Core/` layout and the
  `initializeCommand` devcontainer fix — tracked in
  `project_template_sync.md` memory.
- S3.9 / S3.10 / S3.11 have minimal placeholder bodies; full refinement
  before each is picked up.

### Open questions for the blog source

- **Granularity of DEVLOG entries in chore-heavy sessions.** Today
  landed five PRs; one entry feels right, per-PR would be noisy.
  SKILL.md says "per meaningful session" — a chore-heavy session is
  one meaningful thing even if it contains multiple small PRs.
- **Two-way blog material.** DEVLOG is already doing double duty as
  design record and blog raw material. If the blog process later wants
  structured tags (`[collab]` / `[project]` / `[incident]`), easy to
  add retroactively. Not doing it yet — premature structure.
- **Why SKILL.md didn't get auto-read.** VSCode's Claude Code extension
  auto-loads `CLAUDE.md` but not other root-level Markdown. The fix
  here (CLAUDE.md pointing to SKILL.md) is the cheapest cross-context
  solution — works on Windows host, WSL, and in-container sessions
  because it rides on the already-reliable auto-load. Worth recording:
  separating "conventions that must be read" from "conventions the tool
  auto-loads" is a real gap, and signposting from CLAUDE.md bridges it.

## 2026-04-21/22 — S03.08 TLS hardening: cert validation, cipher pinning, lifecycle

### What went well

- **Planning turn before coding paid off.** Before any C, I laid out a
  phase plan and six API-shape surprises — integration harness must
  live in its own binary (fake interposes libssl), cert generation
  programmatic not fixtures, cipher policy at caller not library, BIO
  lifecycle per-Open, the `OpenSslFake` per-BIO aliasing issue. The
  user corrected one call (BDD scenarios for negative cases →
  integration harness instead, much faster feedback), parked three
  (cipher shape, BIO lifecycle, `SSL_VERIFY_FAIL_IF_NO_PEER_CERT`),
  and greenlit the rest. Every surprise came up once, got a decision,
  then didn't recur.

- **Composable test-support trio.** `TlsTestCert` + `TlsTestServer` +
  `BioPairStream` under `Tests/OpenSslIntegration/` each do one thing;
  new scenarios are a single test body with a cert config and an
  assertion. The fourth scenario (unsupported cipher) was a
  seven-line addition. This is the scaffolding paying for itself
  instantly, not in six months.

- **Pump-on-read pattern.** `BioPairStream::Read` drives the server
  side cooperatively when the BIO pair is empty, so
  `SolidSyslogTlsStream`'s synchronous `SSL_connect` works unchanged
  over non-blocking in-memory BIOs — no threads in the harness, no
  production changes to accommodate testing.

### What didn't go well / process refinements

- **BDD-first plan got reversed.** My Phase 1 was "BDD pending
  scenarios for SL4 failure modes." User pushed back: integration
  harness covers it faster with no PO-visibility loss. Should have
  surfaced this trade-off *myself* in the opening surprises list
  rather than defaulting to BDD. Memory entry
  `feedback_integration_over_bdd.md` captures the pattern: for
  crypto/TLS failure modes, integration harness beats BDD.

- **Phase 2 / Phase 4 coupling I didn't spot up front.** Plan
  separated "return-value checks" (Phase 2) from "resource lifecycle"
  (Phase 4). In practice, every new early-return path made a
  pre-existing leak worse until `Destroy` covered partial state.
  Ended up interleaving — not a real problem but the phase plan was
  cleaner than reality.

- **Commit count.** 17 commits on the branch is a lot of tight red/green
  pairs. Each is a clear atomic behaviour change and reads well in
  `git log --oneline`, but the PR-review surface is larger than a
  single "TLS hardening" story arguably needed. Next time, consider
  batching several return-value checks into one commit when they
  share a test shape.

- **Characterisation vs red/green mismatch.** Three of the integration
  tests went green on first write — OpenSSL defaults already rejected
  expired / wrong-hostname / wrong-CA. Under strict "only write
  production code driven by a failing test" this feels off, but the
  tests still earn their keep as regression protection. I flagged
  this explicitly rather than pretending it was red/green, and the
  user accepted the framing. Calling out "this is characterisation,
  not TDD" is probably the right move whenever it happens.

- **One stray production branch.** The `if (ctx == NULL) return NULL;`
  inside `CreateSslContext` wasn't strictly driven by a failing test —
  it was the natural implementation alongside the `OpenReturnsFalseWhenCtxNewFails`
  fix. User reminded me mid-session to "only write a new branch of code
  when you have a failing test." Tightened up for the rest of the session.

### Decisions captured this session

- **Integration test executable is its own CI job** (option C of A/B/C)
  — runs in parallel with `build-and-test`, self-contained configure
  + build of only `OpenSslIntegrationTests`. Clang and sanitize
  presets don't run it; unit tests remain the coverage engine.
- **Cipher list is caller policy.** `SolidSyslogTlsStreamConfig.cipherList`
  is forwarded to `SSL_CTX_set_cipher_list` if non-NULL; library ships
  no SL4 default. `docs/iec62443.md` names
  `ECDHE+AESGCM:ECDHE+CHACHA20` as a reasonable starting point, not a
  baked-in list.
- **BIO_METHOD lifecycle is per-Open.** Symmetric with SSL/SSL_CTX —
  each Open allocates a fresh set, each Close releases. Destroy
  covers partial-init state idempotently.
- **Coverage stays unit-test-only.** Integration tests are supplementary
  evidence of real-libssl acceptance, not the coverage engine. Memory
  entry `project_coverage_integration.md` records the trigger for
  revisiting (multi-platform, or a real-libssl-only code path).

### Deferred

- **`OpenSslFake` per-BIO data refactor.** Listed as S03.08 acceptance
  but no production code path or test requires distinct BIO data yet
  (the library creates exactly one BIO per Open). Forward-looking
  for mTLS (S03.09) and future cases. Suggest a short follow-up
  story rather than doing it here without a test that fails without it.
- **TLS 1.3 cipher suite control.** `SSL_CTX_set_cipher_list` governs
  TLS 1.2 only; TLS 1.3 uses `SSL_CTX_set_ciphersuites`. Out of the
  ticket's acceptance. Follow-up.
- **`max_proto_version` control.** Library pins min but not max. No
  current need; might emerge with an SL4 policy that mandates TLS 1.3.
- **Formal Planned → Available promotion** for TLS in
  `docs/iec62443.md` — S03.11 scope. This session added the SL4
  substrate narrative without touching the headline status.

### Open questions for the blog source

- **When is characterisation a legitimate TDD outcome?** Three tests
  went green on first write because OpenSSL already did the right
  thing. Keeping them is evidence and regression protection; rejecting
  them would be dogmatic. The useful lens: if the test protects against
  a realistic future regression, it's load-bearing — TDD discipline is
  about "production code driven by tests", not "every test must have
  been red at some point."

## 2026-05-02 — S05.09 capacity threshold alert

Built the early-warning capacity threshold callback on top of the
S18.01 prerequisites (capacity getters, sticky-100% bit, void* context
on `SolidSyslogStoreFullCallback`). Sliced 11 ways, strict TDD with
slice-by-slice review.

### Decisions

- **Threshold lives on `BlockSequence`, not `FileStore`.** The capacity
  state and sticky bit already lived there; adding a single
  `thresholdCrossed` bool plus the three new config fields kept the
  layering clean and FileStore stays a thin pass-through.
- **Threshold returned in bytes, not percentage.** Per-`Write` cost is
  one indirect call (`getCapacityThreshold`), one already-cheap
  `_UsedBytes`, and a `>=` compare. No division on the hot path. If
  integrators want a percentage UI they compute it themselves.
- **NullBuffer recursion: document, don't guard.** Header doc-comment
  on the typedef explains the `SolidSyslog_Log` recursion footgun
  under `SolidSyslogNullBuffer`. A re-entrancy guard would have added
  state and a hot-path branch for a trivially avoidable mistake on the
  integrator side. Pre-1.0, simpler is better.
- **At 100% with HALT, threshold fires before `onStoreFull`.** Spec
  ordering. Refactored `BlockSequence_PrepareForWrite`'s file-full +
  store-full branch to set the sticky bit first, then call
  `NotifyThresholdCrossed`, then `NotifyStoreFull`. Sticky-bit
  assignment moved out of `NotifyStoreFull` into the call site to
  make the sequencing explicit.
- **Three "comes-free" tests.** Slices 5 (context plumbing), 7 (sticky
  doesn't refire), and 9 (threshold drop below current usage) all
  passed without production change. Kept as regression guards rather
  than rejected — they protect realistic future regressions and pin
  down spec contracts that the rest of the design implies.
- **BDD marker file.** Threshold callback in the threaded example
  appends to `/tmp/solidsyslog_threshold_marker.log`; environment.py
  cleans the marker between scenarios. Simpler than stderr-polling and
  scales to multiple scenarios. CI behave runs as root so chmod works;
  locally needed `--user 0` to repro.

### Deferred

- **Repeated `onStoreFull` firing** under sustained HALT — every
  failed `Write` re-fires the callback. Pre-existing behaviour, not in
  S05.09's scope. Worth a separate story if integrators report it as
  noisy.
- **Per-file or per-block thresholds** — explicitly out of scope per
  the AC; single store-level threshold only.

### Open questions

- **Does threshold = 0 belong in the API at all, or should `NULL`
  function be the only "disabled" signal?** Spec mandates both work,
  so I implemented both. Worth a future review pass — the dual-disable
  surface is a small foot-gun.
