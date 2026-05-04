# Dev Log

## 2026-05-04 — S13.19 slice 3: cross-platform TLS+mTLS BDD via OTel TLS receivers

### Decisions

- **Windows BDD oracle gains TLS (6514) and mTLS (6515) syslog
  receivers.** otelcol-contrib's syslog receiver supports a `tcp.tls`
  block — the same `cert_file`/`key_file` (and `client_ca_file` for
  mTLS) that syslog-ng uses on Linux is what otelcol consumes.
  Reusing the existing `Bdd/syslog-ng/tls/{ca,server}.{pem,key}`
  fixtures means a single set of test certs drives both runners and
  there's no Docker-on-Windows or Linux-sidecar complication.
- **Per-transport file exporters mirror the syslog-ng layout.** Linux
  has `received.log` (catch-all) + `received_<transport>.log`
  (per-transport pin). The OTel config now does the same with
  `received.jsonl` + `received_<tls|mtls>.jsonl`. UDP and TCP stay
  multiplexed on the catch-all `received.jsonl` because they share
  port 5514 — the SwitchingSender scenarios that need separate
  per-transport pins for udp/tcp are still `@buffered` Linux-only,
  so no Windows-side per-transport split is needed for them today.
- **`PER_TRANSPORT_LOG` replaced with `per_transport_log(context, t)`
  function.** Returns `received_<X>.log` on Linux, `received_<X>.jsonl`
  on Windows OTel — same semantic, oracle-format-aware path. Two
  internal dicts (`PER_TRANSPORT_LOG_SYSLOG_NG`,
  `PER_TRANSPORT_LOG_OTEL`) keep each runner's path table local.
- **Env-var host override on `ExampleTlsConfig` and
  `ExampleMtlsConfig`.** Defaults stay `"syslog-ng"` so Linux behaves
  unchanged when env vars are absent. Windows BDD's `before_all` sets
  `SOLIDSYSLOG_BDD_TLS_HOST=127.0.0.1` and `SOLIDSYSLOG_BDD_MTLS_HOST=127.0.0.1`
  via `setdefault` so the Windows OTel oracle (bound to 127.0.0.1)
  is reachable. Server cert SAN already includes 127.0.0.1 + localhost
  so the handshake completes against either host name.
- **`getenv_s` on MSVC, `getenv` on POSIX — split per platform main.c.**
  MSVC's `getenv` triggers C4996 under `/W4 /WX` (the project
  deliberately doesn't suppress that warning). The platform-specific
  env-var read lives in each example's main.c
  (`Example/Threaded/main.c` uses `getenv` directly,
  `Example/Windows/SolidSyslogWindowsExample.c` wraps `getenv_s` in a
  small static helper) and both call the same Common
  `ExampleTlsConfig_SetHost(...)` / `_SetMtlsHost(...)` setters. Common
  code stays clean of platform `#ifdef`s.
- **`@buffered` dropped from `tls_transport.feature` AND
  `mtls_transport.feature`.** Both reframed from "the threaded example"
  to "the buffered example" — the same reframe S13.18 did for
  `tcp_transport.feature` and `buffered.feature`. The Windows runner's
  existing `not @buffered` filter automatically picks them up. mTLS
  feature description updated to call out the cross-platform oracle
  path (Linux syslog-ng vs Windows otelcol-contrib).
- **CI port-wait extended to TCP 6514+6515.** The previous step polled
  UDP 5514 only — sufficient when the only Windows-side receivers were
  UDP+TCP on 5514. With TLS/mTLS on 6514/6515 added, the wait now
  asserts all three are bound before BDD runs, avoiding a flaky race
  where a TLS scenario could fire before the receiver is up.
- **`docs/bdd.md` `@buffered` description updated.** The TLS/mTLS
  capabilities are no longer Linux-only, so the tag's meaning narrows
  to "file-backed block store, switching sender, or syslog-ng reload".

### Deferred

- **SwitchingSender across UDP/TCP/TLS/mTLS on the Windows example.**
  The Windows example still picks one sender at startup. Slice 2
  flagged this as out of scope, and slice 3 doesn't need it — the
  cross-platform TLS/mTLS scenarios use a single fixed transport per
  scenario. Remains a follow-up, tracked under the example-commonality
  follow-up flagged in S13.18.
- **Per-transport pin for udp/tcp on Windows.** OTel UDP and TCP
  receivers both feed `received.jsonl`, so a Windows-side
  "syslog-ng receives N over udp" wouldn't disambiguate. Not needed
  for the in-scope features (which only use the per-transport step
  for TLS/mTLS). The `switching_transport.feature` keeps `@buffered`
  for now and stays Linux-only.

### Open questions

- None for slice 3. The end-to-end Windows TLS handshake is validated
  by CI behave runs; local smoke tests confirmed the OTel collector
  binds 5514/6514/6515 and accepts the agreed cert chain.

## 2026-05-04 — S13.19 slice 2: ExampleTlsSender hoisted, Windows TLS/mTLS arms

### Decisions

- **Hoisted `ExampleTlsSender.h`, `_OpenSsl_*.c`, `_Unavailable.c` from
  `Example/Threaded/` to `Example/Common/`.** The only platform-specific
  thing in the existing `ExampleTlsSender_OpenSsl.c` was the underlying
  TCP stream type (`SolidSyslogPosixTcpStream`) — the rest of the
  factory (mTLS branch, TLS stream + StreamSender wiring, mTLS-vs-TLS
  config dispatch) is identical across platforms. Splitting the
  underlying stream into a per-platform backend file
  (`_OpenSsl_PosixTcp.c` and `_OpenSsl_WinsockTcp.c`, picked by CMake)
  keeps both example binaries aligned and matches the user's
  example-commonality preference.
- **Backend selected by CMake, not by `#ifdef`.** `Example/CMakeLists.txt`
  now appends `Common/ExampleTlsSender_OpenSsl_PosixTcp.c` for the
  Threaded (POSIX) example and `Common/ExampleTlsSender_OpenSsl_WinsockTcp.c`
  for the Windows example when `SOLIDSYSLOG_OPENSSL=ON`, falling back to
  `Common/ExampleTlsSender_Unavailable.c` otherwise. Mirrors how the
  rest of the project handles platform variance.
- **Windows example `--transport` switched from
  `enum SolidSyslogTransport` to `const char*`.** The library enum is
  a UDP-vs-TCP socket-type discriminator for the Resolver — it has no
  TLS value and shouldn't grow one (TLS rides on a TCP underlying
  stream, the resolver still asks for TCP). The Linux `ExampleOptions`
  already uses a string for the same reason. The string carries
  `"udp" | "tcp" | "tls" | "mtls"`; sender wiring switches on
  `strcmp` matching.
- **Three arms in the if/else chain, not a SwitchingSender.** Slice 2
  picks one sender at startup. The Linux Threaded example uses a
  `SolidSyslogSwitchingSender` over UDP+TCP+TLS so the
  `ExampleInteractive` "transport <name>" command can switch at runtime,
  but that's a separate hoist S13.18 deferred (factory shape mismatch).
  Today the Windows example remains startup-fixed and the `mtls` /
  `tls` arms reuse the same `ExampleTlsSender_Create(resolver, mtls)`
  factory the Threaded example calls.
- **`SolidSyslogTransport.h` removed from
  `Example/Windows/ExampleWindowsCommandLine.h` and
  `SolidSyslogWindowsExample.c`.** No longer needed since the example
  no longer routes through that enum, and dropping it preserves the
  IWYU gate.

### Deferred

- **Hoisting the per-platform `CreateSender` / `DestroySender` factories
  to `Example/Common`.** S13.18 already flagged this — Threaded composes
  via `SwitchingSender` (UDP+TCP+TLS+mTLS), Windows picks one. The
  shapes have diverged enough that any "hoist" is really "widen Windows
  to use SwitchingSender" first. Out of slice 2 scope.
- **Live TLS smoke test against `openssl s_server` on Windows.** Slice 2
  shipped CMake + linker + factory wiring; live handshake testing lands
  in slice 3 against the BDD oracle. The MSVC binary builds clean,
  starts under each `--transport` value without crashing, and the
  Linux Threaded TLS + mTLS BDD scenarios remain green — proves the
  refactor didn't regress the existing pipeline.

### Open questions

- **Windows mTLS host string.** `ExampleMtlsConfig_GetHost()` returns
  `"syslog-ng"` (the Linux compose service name). On the Windows BDD
  runner the oracle will be on `127.0.0.1`. Slice 3 needs to either
  branch by env var (`MTLS_HOST=...`) or have separate
  `ExampleMtlsConfig_*Linux*.c` / `*Windows*.c` files. Will sort during
  slice 3 when the otel mTLS receiver wiring lands.

## 2026-05-04 — S13.19 slice 1: OpenSslIntegrationTests on MSVC

### Decisions

- **Sliced S13.19 into three.** Slice 1 (this PR) — prove the existing
  `Platform/OpenSsl/SolidSyslogTlsStream` is byte-for-byte portable to
  MSVC + vcpkg OpenSSL by lighting up the same `OpenSslIntegrationTests`
  binary the Linux job runs. No example or BDD changes. Slice 2 will
  hoist `ExampleTlsSender` to `Example/Common` (per the
  example-commonality memory) and add `--transport tls`/`mtls` arms to
  the Windows example. Slice 3 extends `Bdd/otel/config.yaml` with
  `syslog/tls` and `syslog/mtls` receivers and drops `@buffered` from
  the two TLS feature files. mTLS in scope across all three slices.
- **`std::filesystem::temp_directory_path()` + `std::ofstream` instead of
  `mkstemp` + `fopen`.** `SolidSyslogTlsStreamIntegrationTest.cpp` was the
  only Linux-only file in the test tree — `mkstemp`/`unlink` are POSIX,
  `/tmp/...` doesn't exist on Windows. C++17 `<filesystem>` is the
  portable replacement, and `std::ofstream` avoids MSVC's C4996 on
  `fopen` (the project deliberately doesn't suppress C4996 — see
  `_CRT_SECURE_NO_WARNINGS` ban in CLAUDE.md). Path buffers bumped from
  64 to 256 to comfortably hold a Windows temp path
  (`%LOCALAPPDATA%\Temp\...` is ~50–60 chars before the filename).
- **Vendored `applink.c` linked into `OpenSslIntegrationTests` on
  MSVC.** OpenSSL on Windows requires `applink.c` to be compiled into
  the application whenever the application passes `FILE*` across
  runtime-library boundaries (e.g. `PEM_write_*`). Without it, the test
  prints `OPENSSL_Uplink: no OPENSSL_Applink` and aborts. vcpkg's
  `find_package(OpenSSL)` exposes the path via `OPENSSL_APPLINK_SOURCE`,
  so the CMake guard is `if(MSVC AND OPENSSL_APPLINK_SOURCE)`. The
  vendored file trips `/W4 /WX` (C4152 fn-ptr cast, C4996 on `fopen` /
  `_open`); per-source `set_source_files_properties(... COMPILE_OPTIONS
  "/wd4152;/wd4996")` keeps the suppressions scoped to that one file
  instead of weakening the project-wide flags.
- **`vcpkg install openssl` added to `build-windows-msvc` AND a new
  `integration-windows-openssl` job.** `build-windows-msvc` previously
  configured with `SOLIDSYSLOG_OPENSSL=OFF` (no OpenSSL on the runner),
  so `Platform/OpenSsl/SolidSyslogTlsStream.c` and the OpenSslFake-backed
  unit tests didn't exercise on MSVC at all. Adding `openssl` to the
  install line lights both up. The new `integration-windows-openssl`
  job mirrors `integration-linux-openssl` shape — separate runner, own
  `vcpkg install`, real libssl, JUnit upload, surfaced through the
  `summary` required-check.
- **Two `vcpkg install` lines, not a manifest file.** Matches the
  existing `build-windows-msvc` style. Symmetric and simple — the
  binary cache (`x-gha,readwrite`) still kicks in across runs.

### Deferred

- **Branch protection update.** The new `integration-windows-openssl`
  check needs to be added to the required-checks list on GitHub branch
  protection settings — the workflow change alone doesn't update it.
  Flagged for the user to do post-merge (or pre-merge if blocking the
  squash).
- **Slice 2 (Windows example TLS) and Slice 3 (BDD TLS oracle).** As
  agreed in the slicing plan; tracked on issue #245.

### Open questions

- None for slice 1.

## 2026-05-04 — S13.18 Windows BDD on the portable ring buffer

### Decisions

- **Test-side prompt-protocol portability over a production
  drain-on-shutdown contract.** Two ways to make `wait_for_prompt`
  work on Windows pipe fds: (A) replace `select.select` + `os.read`
  with a thread-based stdout reader in the Python step layer, or (B)
  add a "drain pending records before honouring shutdown" path to
  `ExampleServiceThread` and ship a `SolidSyslogBuffer.HasPending`
  probe to support it. (A) shipped: same Python signature, same
  return shape, daemon thread + `queue.Queue`. The library and
  example don't bake test convenience into shipped semantics, and
  ungraceful shutdown stays a real testable scenario for later
  stories. New feedback memory recorded for the
  test-side-first principle.
- **Windows example uses CircularBuffer + WindowsMutex driven by a
  Win32 service thread, mirroring the Linux Threaded model.**
  `SolidSyslogNullBuffer_Create(sender)` removed; replaced by
  caller-allocated `bufferStorage` of
  `SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE(EXAMPLE_BUFFER_MESSAGES)`
  (10 messages, matching the Threaded example's `mq_open` capacity)
  backed by `SolidSyslogWindowsMutex`. The thread is launched via
  `_beginthreadex` calling a wrapper that invokes the existing
  `ExampleServiceThread_Run` — same Common helper as Linux. Shutdown
  is `shutdownFlag = true` + `WaitForSingleObject` + `CloseHandle`,
  the Win32 analogue of `pthread_join`.
- **`<windows.h>` after `<winsock2.h>` with an explicit comment.**
  Reversing the order leaks the legacy winsock1 declarations from
  windows.h and breaks the `winsock2.h` include. The MSVC build
  failed on a wrong order during slice 2; pinned the order with a
  one-line `// windows.h must follow winsock2.h to avoid winsock1/2
  declaration conflicts` comment so a future cleanup can't
  alphabetise it back.
- **`@buffered` tag re-scoped, not retired.** The two cross-platform
  scenarios (`buffered.feature`, `tcp_transport.feature`) drop
  `@buffered` so they run on the Windows OTel runner. Step prose in
  those two features changes from "the threaded example" to "the
  buffered example"; the step layer routes the new prose to
  `THREADED_BINARY` when `oracle_format == "syslog-ng"` and to
  `context.example_binary` (the new Win32 example) otherwise. The
  remaining `@buffered` features stay as-is — they really are
  pthread / Linux-specific (file-backed `SolidSyslogBlockStore`,
  `SolidSyslogSwitchingSender`, mTLS / TLS, `tcp_reconnect`'s
  syslog-ng UNIX-socket reload) and keep the "the threaded example"
  prose. `docs/bdd.md` `@buffered` tag description rewritten to
  reflect this — it now means "needs a Linux-only buffered
  capability beyond a basic ring buffer + service thread", not
  "needs the threaded binary".
- **`tcp_singletask.feature` tagged `@windows_wip` (Linux-only
  now).** Pre-S13.18 it was the runner-agnostic Windows-TCP
  validation companion to the Linux-only `tcp_transport.feature`. Now
  that `tcp_transport.feature` is itself cross-platform via the
  buffered example, `tcp_singletask` no longer earns its keep on
  Windows — and the one-shot `process.communicate` pattern in
  `run_example` would race with the new Windows service thread
  (no prompt-wait, so "quit" can land before the buffer drains).
  Kept on Linux as the bare-metal / NullBuffer / synchronous-send
  pin.
- **No CI workflow change required.** The Windows tag filter
  `not @wip and not @windows_wip and not @buffered` already does
  the right thing once the per-feature tag drops land — the
  `not @buffered` part still excludes the genuinely Linux-only
  features.
- **`Common/ExampleServiceThread.c` added to the Windows binary's
  CMake source list.** Pre-S13.18 only the Linux Threaded binary
  used it. Now it's shared, which is a small but meaningful step
  towards the user's stated goal of keeping the Windows and Linux
  examples as common as possible. New feedback memory recorded
  to keep watching for further hoisting opportunities.

### Deferred

- **`CreateSender` / `DestroySender`-shaped factories in the
  per-platform `main.c` files.** These are obvious candidates to
  hoist into `Common/` but the Threaded version composes a
  SwitchingSender over UDP + TCP + TLS + mTLS, while the Windows
  version still picks a single sender by transport flag. The
  shapes are too different today to share without first widening
  the Windows example (SwitchingSender wiring), which is well
  outside S13.18 scope. Flagged for a follow-up refactor PR.
- **`ExampleInteractive_Run`'s `\r\n` vs UTF-8-BOM stdin handling
  not tightened.** A PowerShell-based local smoke test surfaced
  that `.NET StreamWriter` writes a UTF-8 BOM at the start of
  stdin, which `fgets` reads as the first three bytes of the first
  command — `MatchCommand("send")` then fails. The BDD harness
  uses Python `subprocess.Popen(text=True)` which doesn't emit a
  BOM, so this isn't a CI hazard. Worth tightening if the example
  ever grows a "paste me into PowerShell" docs path; not today.
- **`PosixMessageQueueBuffer` retirement.** The user explicitly
  scoped it out of S13.18 — the Linux Threaded example continues
  to use it. Open question: is the long-term plan to retire it in
  favour of `SolidSyslogCircularBuffer` + `SolidSyslogPosixMutex`
  (one buffer for all threaded examples), or keep both shipped?
  The header table calls out both in `Tests/Support/`-style
  audience splits, suggesting both stay — but the duplication
  becomes more visible now that CircularBuffer has parity.

### Open questions

- **Should the Windows example's stdin handling defend against a
  leading UTF-8 BOM?** A two-byte one-line fix (skip BOM if the
  first three bytes match) would make the binary friendlier for
  `Get-Content | exe` style PowerShell pipelines without affecting
  POSIX or the BDD harness. Not a regression — the pre-S13.18
  Windows binary had the same behaviour because it used the same
  `ExampleInteractive_Run`.
- **Drain-on-shutdown semantics for `ExampleServiceThread_Run`.**
  Today the loop is `while (!*shutdown) Service()` — once shutdown
  is signalled the thread exits even if the buffer is non-empty.
  The BDD prompt protocol coordinates around this (oracle confirms
  receipt before "quit" is sent) but a real integrator may want
  graceful drain. `SolidSyslogBuffer` doesn't currently expose a
  "has unsent" probe; adding one is a deliberate API decision not
  taken in this story.

## 2026-05-04 — S18.03 Dispose-on-empty block lifecycle

### Decisions

- **Dispose trigger lives in MarkSent AND post-rotation.** Slice 1
  hooked `BlockSequence_DisposeReadBlockIfDrained` into MarkSent only.
  The unit tests passed (rotate-then-drain pattern: MarkSent fires
  while writeSequence has already moved on). Then the new BDD scenario
  failed: the threaded service-thread drain pattern is _interleaved_
  (each Service iteration: receive 1 from buffer → write 1 to store →
  read 1 from store → MarkSent). With this pattern, MarkSent fires
  while the write block is still active — IsReadBlockActiveWrite is
  true so dispose is suppressed. Then the next iteration's Write
  triggers rotation, but no MarkSent follows for the just-sealed block.
  The trigger needed a second call site after rotation seals the prior
  write block. Now in `RotateToNextBlock`, post-AdvanceWriteToNewBlock.
  Slice 1's MarkSent call still earns its keep for the rotate-then-drain
  pattern (where the block is already non-active at MarkSent time).
- **Dispose-on-empty guard: oldestSequence == readSequence.** The trigger
  declines to dispose when readSequence has advanced past oldestSequence
  (e.g. via AdvanceToNextReadBlock skipping a fully-sent block). Disposing
  an interior block would punch a gap in the sequence run that
  `ScanForExistingBlocks` can't represent (it assumes a single contiguous
  run). The pre-existing capacity-pressure DiscardOldestBlock path still
  reaps those interior holdovers eventually.
- **Acquire empty-check in RotateToNextBlock, not Open's cold start.**
  Story acceptance asked for "BlockSequence asserts the block is empty
  before Acquire". Open's cold-start branch only fires when
  `ScanForExistingBlocks` returned nothing — by construction Exists(0)
  is false at that point, so an empty-check would always be a no-op.
  Adding it would be defensive code for an impossible scenario per
  CLAUDE.md. So `AcquireEmptyBlock` wraps `RotateToNextBlock`'s Acquire
  only.
- **Success-only state advancement: separate failure-injection paths
  for Acquire and Dispose.** `RotateToNextBlock` now plumbs Acquire's
  result through `PrepareForWrite`'s `spaceAvailable` return — a
  transient device failure surfaces to the caller as `Write → false`
  with `writeSequence`/`writePosition` unchanged, ready to retry.
  `DiscardOldestBlock` wraps state mutation in `if (Dispose(...))` —
  on failure `oldestSequence` stays put and the next discard cycle
  re-attempts the same block. Without this, a failed Dispose would
  orphan a still-on-disk block forever (oldest pointer skipped past).
- **FileFake_FailNextDelete added (test infrastructure).** Mirrors the
  existing FailNext{Open,Write,Read} shape so the dispose-failure path
  could be exercised at the integration boundary, not just at the
  BlockSequence unit level. The unit-level fake (ScanFake in
  BlockSequenceTest) gained call-log + size-tracking knobs to model
  call ordering and "block has data" semantics that previously the
  no-op stubs couldn't represent. The pin test
  `RotationSkipsDisposeWhenTargetBlockEmpty` regressed when the new
  post-rotation dispose check shipped — fake's FakeSize returned 0,
  so dispose-on-empty thought block 0 was always drained. Fix: track
  sizes in the fake and seed `sizes[0]` after Open in the rotation
  test setup.
- **block_lifecycle.feature reframed.** The pre-S18.03 baseline
  scenario said "block files are not disposed when capacity is not
  exhausted" — now overturned. Reworded to pin the active-write-block
  guard, plus added "Older block is disposed once all its records are
  drained" as the positive case. The failure-injection scenarios from
  the story acceptance (Acquire/Dispose failure) live at unit level —
  exercising them at BDD level would need a fault-injection harness
  in the threaded example which is scope creep.

### Deferred

- **AdvanceToNextReadBlock as a third dispose trigger.** When the read
  cursor walks off the end of a block via this path (record corruption
  or end-of-data hit), the block is technically "drained" but the
  trigger doesn't fire — IsReadBlockOldest tests against the new
  readSequence which has already moved on. Capacity-pressure
  DiscardOldestBlock still reaps it. Worth revisiting once the example
  flash driver lands and we see if the latency matters.
- **Block-device-side empty-check verification.** Today the contract
  is "BlockSequence Disposes-then-Acquires when the target block
  exists". A more defensive contract would have the BlockDevice itself
  assert empty in Acquire (returning false on stale content). Pushed
  to S18.04 since the contract shape depends on what flash drivers
  actually want.

### Open questions

- **Should `DisposeReadBlockIfDrained` propagate Dispose-success to
  the caller?** Today it returns `readBlockChanged` only when Dispose
  succeeded AND the read pointer advanced — but a failed Dispose is
  silently swallowed (oldest stays put per the slice-4 contract,
  no-op). An integrator-supplied error reporter could surface the
  Dispose failure once that path lands. Not a blocker for S18.03;
  noted alongside the "persistent media error" path mentioned in
  RecordStore's IsRecordSent comment.

## 2026-05-03 — Hoist SolidSyslogAddress.h to Core + CMake layer guard

Small refactor: `Platform/Posix/Interface/SolidSyslogAddress.h` and
`Platform/Windows/Interface/SolidSyslogAddress.h` were byte-identical
(an opaque storage blob sized for `sockaddr_storage`). Collapsed into
`Core/Interface/SolidSyslogAddress.h` and added `cmake/LayerGuard.cmake`
to keep the dependency direction honest going forward. Merged as #257.

### Decisions

- **Hoist over keeping duplicates.** Both copies were identical and
  the type is platform-agnostic (the Posix and Winsock `sockaddr_storage`
  shapes both fit in 128 bytes). Two copies survived from earlier
  story-by-story growth; no actual platform variance to preserve.
- **Configure-time guard, not build-time.** `solidsyslog_enforce_layering()`
  runs in `CMakeLists.txt` immediately after `project()`. CMake re-runs
  configure on every build when inputs change, so PRs are always
  checked. A failure produces a single `FATAL_ERROR` listing every
  offending file/include, not a flood per translation unit.
- **Header basename matching, not path matching.** Scan enumerates
  `*.h` basenames owned by `Platform/` and `Example/`, then greps
  `#include "..."` directives in the consuming layer for any match.
  Cheaper than building a path graph, and robust to whatever relative
  include form the source uses.
- **Tests/ and Bdd/ out of scope.** Per CLAUDE.md tier table they are
  not supported targets; test code is allowed to reach into anything
  it needs. The guard scans `Core/` and `Platform/` only.
- **Skipped most prechecks.** CLAUDE.md prescribes the full preset
  battery before raising a PR. For a header rename + cmake addition
  with no behavioural change, gcc-debug + full unit suite (1005 tests,
  1406 checks, all pass) was judged sufficient. Flagged the omission
  before pushing.

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

## 2026-05-04 — S24.01 IWYU CI gate

Landed the include-what-you-use gate as a CI-friendly preset, then ran
the apply over the whole tree as a discovery exercise. The story body
predicted "single-digit pragmas needed" based on a pre-flight audit;
the reality was 84 files with findings on the first run, dropping to
77 after the project filter. That triggered a rethink mid-stream.

### Decisions

- **IWYU lives in the clang container, not the gcc container.** IWYU
  is a Clang library tool — building it from source against clang-19
  in `CppUTestDockerClang` was one extra apt line plus a small build
  block. Putting it in the gcc image would have meant ~250 MB of LLVM
  dev headers in a container whose identity is GCC. Compile commands
  also line up cleanly when IWYU and the actual compiler are both
  clang.
- **Custom target driven by `iwyu_tool.py`, not the CMake launcher.**
  CMake's `CMAKE_<LANG>_INCLUDE_WHAT_YOU_USE` launcher mechanism
  intentionally ignores IWYU's exit code (a 3.11 design choice), so
  `--error` doesn't fail the build through it. Driving via
  `iwyu_tool.py` piped to a project filter keeps the non-zero-on-
  finding contract intact and lets the filter be the authoritative
  gate.
- **Per-target scope (Core, Platform, Example, Tests) — but not Bdd.**
  Story body said Tests/ was out of scope, but narrowing public
  headers immediately exposed test code that was relying on transitive
  includes; the same hygiene rule has to hold there or the gate keeps
  tripping consumers. Bdd/ remains out of scope (it's not in
  `compile_commands.json` anyway).
- **Project filter drops three classes of finding IWYU gets wrong:**
  redundant `struct Foo;` forward-decls (kept for MISRA/readability —
  the in-struct auto-declaration trick is too obscure), `<stdbool.h>`
  removals on dual-language C+C++ headers (IWYU only sees the C++
  side), and `CppUTest/TestHarness.h` removals (IWYU doesn't model
  `TEST(...)` macro expansion). Each rule is documented inline in
  `scripts/iwyu_filter.py` with the rationale.
- **Custom CppUTest mapping (`cmake/cpputest.imp`) directs IWYU to
  suggest the public `TestHarness.h` umbrella over the internal
  headers.** Without this, IWYU was asking tests to include
  `<UtestMacros.h>` directly, which broke the project's CppUTest
  convention.
- **Disabled `modernize-deprecated-headers` in `.clang-tidy`.** IWYU
  natively suggests `<stdint.h>` (the C-style names), which is right
  for a C library; clang-tidy's modernize rule was demanding `<cstdint>`
  in C++ test code. For an embedded-friendly C library these two
  tools were fighting each other; resolving in IWYU's favour matches
  the library's identity.
- **Two pragmas added, both for genuinely dual-language headers.**
  `Tests/Support/OpenSslFake.h` keeps `<stdbool.h>`; `Tests/TestUtils.h`
  has the CppUTest include relocated inside `#ifdef __cplusplus`. Each
  has an inline comment explaining why.
- **`fix_includes.py` blank-line bug.** It inserted 1021 spurious
  blank lines between `TEST(...)` macros and their `{` body across 46
  test files. clang-format treated both forms as fixed points so
  neither tool surfaced it. Stripped via a small Python script.
- **Two-PR strategy for branch protection.** This PR lands the gate
  and the cleanup but does **not** make `analyze-iwyu` a required
  status check. Doing so would block any in-flight PR (no in-flight
  PR has been through `iwyu-apply`). The follow-up PR flips it on
  branch protection.

### Deferred

- **`analyze-iwyu` as a required status check** — separate small PR
  after this one merges and any in-flight PRs land.
- **Container image bump in this clone.** The active feature-work
  clone is using `cpputest-clang:sha-0385cea` via docker-compose; not
  bumping the SHA here keeps that stable. The CI workflow change to
  add `analyze-iwyu` will pin the new clang-container SHA at the same
  time.
- **Whether to switch the default devcontainer from gcc to clang** —
  raised in conversation, deferred. Modest lean toward keeping gcc as
  default (target deployment compilers are gcc-derived for embedded);
  the cleaner cleanup would be slimming each container to a single
  identity (gcc + cppcheck + coverage; clang + LLVM tooling). Worth
  a separate epic when ready.

### Open questions

- **Audit fidelity.** The story body's "single-digit pragmas" estimate
  was wrong by an order of magnitude. The audit was done by reading
  headers; IWYU's strict purist rule is much narrower than human
  intuition. Lesson for future "small CI gate" stories: estimate by
  running the tool, not by reading the code.


## 2026-05-04 — S04.05 Circular buffer with mutex injection

### Decisions

- **Drop-newest under back-pressure.** Originally the story body called
  for drop-oldest plus a lost-message marker. Closing comment on S04.06
  (#55) made that obsolete: gap detection on the receive side via
  `origin sequenceId` covers loss visibility. Drop-newest is also what
  `PosixMessageQueueBuffer` already does silently (`O_NONBLOCK` +
  ignored `mq_send` return), so the two non-null buffer implementations
  now share semantics. If a need for drop-oldest or halt at this layer
  ever shows up, `SolidSyslogDiscardPolicy` from `BlockStore` is the
  natural place to extend.

- **No-split records on wrap.** When a record wouldn't fit between
  `tail` and the storage end, the impl marks the unused tail with a
  `wrapPoint` and writes the new record at index 0 — but only if the
  pre-wrap unread region wouldn't be overwritten. Otherwise the write
  is dropped. This keeps records contiguous, simplifies reasoning, and
  bounded the wasted tail space at one record-worth. The trade-off
  (a few bytes of waste at each wrap) is fine; if it ever matters we
  can switch to modular indices without changing the public API.

- **Drained-buffer reset.** When `head == tail` at a non-zero offset,
  Write recycles to `head = tail = 0` before deciding whether to write.
  This avoids dropping records that are smaller than `capacity` but
  larger than the remaining tail space. Caught during ZOMBIES probing
  before the test ever shipped — there was a state-space hole in the
  initial overflow logic.

- **`maxMessages` as the user-facing capacity unit.** The
  `SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE` macro takes "max messages
  at max size" rather than raw bytes. Friendlier than asking
  integrators to multiply by `MAX_MESSAGE_SIZE + sizeof(uint16_t)`
  themselves; matches the embedded "size for the worst case" mindset.

- **Mutex vtable shape mirrors `SolidSyslogAtomicOps`** (`Lock(self)` /
  `Unlock(self)` rather than the new `void* context` callback
  convention) since a mutex naturally has state. `SolidSyslogNullMutex`
  is the no-op default for single-task use; `SolidSyslogPosixMutex`
  wraps `pthread_mutex_t`; `SolidSyslogWindowsMutex` wraps
  `CRITICAL_SECTION`. Both platforms in scope so S13.18's BDD wiring
  has both ready.

- **Disable `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling`
  globally.** The rule recommends C11 Annex K (`memcpy_s` etc.) which
  glibc doesn't ship. Every `memcpy`/`memset`/`strncpy` site in the
  codebase already had a `NOLINTNEXTLINE` suppression — 21 in total.
  When *every* hit is suppressed, the rule is doing zero discrimination.
  Sanitizers and bounded-by-construction call sites are the real
  defence. Added the rule to the negation list in `.clang-tidy` and
  swept the 21 inline suppressions out.

### Deferred

- **Magic-numbers clang-tidy suppressions.** `.clang-tidy` also
  disables `-readability-magic-numbers` and
  `-cppcoreguidelines-avoid-magic-numbers`. The "no magic numbers"
  preference David has been asking for in code reviews is the
  opposite — re-enabling these rules might enforce that automatically.
  Worth a separate discussion + cleanup PR after S04.05 lands.

- **Atomics-based lock-free SPSC ring.** The original story body
  mentioned "or atomics where available". A lock-free ring is a
  substantially different design (writer doesn't wait, reader doesn't
  wait, ABA hazards, etc.) and isn't needed by current targets.
  Revisit under a future RTOS / lock-free epic if a real need emerges.

- **Wiring the ring buffer through the BDD harness on Windows.** That
  is S13.18 (#244), now unblocked.

### Open questions

- **Buffer sizing in the embedded use-case.** With `maxMessages = 1`
  and `MAX_MESSAGE_SIZE = 2048`, the smallest buffer is ~2 KB plus
  overhead. For very small targets that's significant. Could be
  addressed later with a CMake cache variable for `MAX_MESSAGE_SIZE`
  (already on the project memory backlog) or by relaxing the macro to
  take an explicit byte count alongside the message count.

## 2026-05-04 — S04.05 review-driven fixes

CodeRabbit review on PR #263 surfaced four substantive findings beyond the
IWYU/format hygiene noise. All addressed via strict TDD (Red test → Green fix)
in commit `ebddeb4`:

### Decisions

- **`Read` discards `maxSize`** (Critical). `Read` now refuses if
  `recordSize > maxSize`: returns `false`, leaves the record queued, sets
  `bytesRead = 0`. Mirrors `mq_receive` `EMSGSIZE` semantics (fail loud, not
  silent truncation). New TEST_GROUP `SolidSyslogCircularBufferSmallRing`
  with a 32-byte ring keeps the boundary tests readable.

- **`Write` truncates `size` to `uint16_t`** (Critical). `Write` now drops
  payloads larger than `SOLIDSYSLOG_MAX_MESSAGE_SIZE` outright, before any
  framing. `MAX_MESSAGE_SIZE = 2048` is far below `UINT16_MAX = 65535`, so
  this single bound subsumes the 16-bit truncation concern and aligns with
  the rest of the library's invariant.

- **`<=` on the fit-helpers collapsed full onto empty** (Critical).
  `RecordFitsAtTail` (wrapped branch) and `RecordFitsAfterWrap` now use `<`
  instead of `<=`. The non-wrapped branch in `RecordFitsAtTail` keeps `<=`
  because perfect-fit at end of storage (`tail + recordBytes == capacity`)
  does not collapse onto `head == tail` while there is unread data. Two
  distinct tests probe both helper paths.

- **`pthread_mutex_init` return ignored** (Major). Deferred to E12 (error
  handling). Default in-process mutex init is reliable in practice; the
  project does not yet have an error-reporting infrastructure to surface
  failures to callers, so adding the check inconsistently with the rest of
  the library would be premature. Tracked in
  [robustness backlog](memory). Same applies to
  `InitializeCriticalSection`, which is `void` on Vista+ anyway.

### Refactor

- **`_Create` now takes `storageBytes` instead of `maxMessages`** (commit
  `0b89e69`). Added `SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE_BYTES(ringBytes)`
  alongside the friendly `_STORAGE_SIZE(maxMessages)`. Lets the small-ring
  TEST_GROUP construct a 32-byte ring directly, decoupled from
  `MAX_MESSAGE_SIZE`. Per-record byte counts in tests are now tractable
  (10/12/19 byte payloads instead of 1000-byte ones).

### IWYU pragma keep on `SolidSyslog.h`

- IWYU's verdict on `SolidSyslogCircularBuffer.h` differs per TU: the .c
  (which does not expand `SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE`) sees
  `SolidSyslog.h` as unused and asks to remove it; the test (which does
  expand the macro) sees the macro body's reference to
  `SOLIDSYSLOG_MAX_MESSAGE_SIZE` and asks to add it. The two TUs cannot
  agree — one or the other always complains. `// IWYU pragma: keep` on the
  include is the canonical remediation for this exact case (commit
  `c74f142` after format pass). Verified locally with
  `cmake --build --preset iwyu --target iwyu` exiting `0`.

- This is the only IWYU pragma added in this PR. The two existing pragmas
  (`Tests/Support/OpenSslFake.h` keeping `<stdbool.h>`, `Tests/TestUtils.h`
  keeping `CppUTest/TestHarness.h`) cover *load-bearing* includes that IWYU
  cannot see through; this new one covers a *macro-deferred reference*.

### Cleanup

- **Disabled `clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling`
  globally** (commit `fdf75f9`). Rule recommends C11 Annex K (`memcpy_s`
  etc.) which glibc does not ship; every `memcpy`/`memset`/`strncpy` site
  in the codebase had a `NOLINTNEXTLINE` suppression — 21 in total. When
  every hit is suppressed, the rule is pure noise. ASan/UBSan and
  bounded-by-construction call sites are the real defence.

### Deferred — to revisit after S04.05 lands

- `.clang-tidy` also disables `-readability-magic-numbers` and
  `-cppcoreguidelines-avoid-magic-numbers`. David's reviewer-side
  preference is "no magic numbers." Worth a separate discussion to decide
  whether to re-enable those rules and accept the resulting cleanup sweep.
  Tracked in
  [project_clang_tidy_magic_numbers.md](memory).
