# BDD scenarios

Gherkin features under `features/` are driven by [Behave](https://behave.readthedocs.io/)
against a `syslog-ng` oracle (Linux + FreeRTOS) or `otelcol-contrib` (Windows + VxWorks 6.4). The
same feature files run on every target — the active runner is selected by the
`BDD_TARGET` environment variable, which `features/environment.py` reads in
`before_all` and dispatches via `features/steps/target_driver.py`.

| `BDD_TARGET` | Built by | Driven as | Oracle |
|---|---|---|---|
| `linux` (default) | `tunable-override-debug` | native subprocess | `syslog-ng-linux` |
| `windows` | `msvc-tunable-override` | native subprocess | `otelcol-contrib` |
| `freertos` | `freertos-cross` | `qemu-system-arm` | `syslog-ng-freertos` |
| `vxworks64` | the VxWorks 6.4 runner, on a build machine | QEMU (Malta) on that machine, its console over TCP | `otelcol-contrib` on the development machine |

`environment.py` resolves each target's binary from these builds. Build the preset
for your target, or set `EXAMPLE_BINARY` to a path of your own.

Each BDD target pairs with its own oracle service so jobs (and developers
switching containers) never interfere — see [`docs/containers.md`](../docs/containers.md)
for the compose layout.

## Tags

Authoritative reference: [`docs/bdd.md`](../docs/bdd.md#feature-tags). Quick recap below.

| Tag | Meaning |
|---|---|
| `@udp` / `@tcp` / `@tls` / `@mtls` | Transport-specific scenario; CI jobs filter by these. |
| `@buffered` | Needs a buffered wiring (CircularBuffer / PosixMessageQueueBuffer / service thread) beyond single-task PassthroughBuffer. Carried by all three targets. |
| `@store` | Needs file-backed `SolidSyslogBlockStore` capability (write blocks, replay, threshold callbacks). Carried by all three targets. |
| `@requires_message_size_1500` | Needs `SOLIDSYSLOG_MAX_MESSAGE_SIZE >= 1500` — only the UDP path-MTU clipping feature. The authoritative entry names the runners that supply it. |
| `@windows_wip` | Skipped on the Windows runner (typically OS-specific behaviour the OTel oracle doesn't model). |
| `@freertoswip` | Skipped on the FreeRTOS-on-QEMU runner. Per-scenario follow-up tag for capability gaps; removed scenario-by-scenario as each gap closes. The early bring-up reasons (hardcoded `TEST_*` values, missing SD wiring, getopt-only args) have all been closed out by S08.03 + S08.04 slices. |
| `@rtc` | Scenario assumes the device has an RTC and synchronised wall-clock time. Run on Linux/Windows (which have both) and VxWorks 6.4 (whose harness sets the clock at startup). Skipped on the FreeRTOS and CMSIS-RTOS2 targets, which model a no-RTC product per RFC 5424 §6.2.3.1. |
| `@no_rtc` | Scenario asserts the no-RTC product behaviour over the wire (`tzKnown="0"`, `isSynced="0"`). Run on the FreeRTOS and CMSIS-RTOS2 targets. Skipped on Linux/Windows and VxWorks 6.4. The NILVALUE TIMESTAMP itself is not asserted via the oracle — syslog-ng silently substitutes receipt time for `${ISODATE}` / `${S_ISODATE}` when the wire timestamp is NILVALUE — so that case is covered by formatter unit tests. |
| `@vxworks64wip` | Skipped on the VxWorks 6.4 target until the capability the scenario needs lands; the authoritative entry says what each one waits for. |
| `@wip` | Globally skipped on every runner. Not currently in use on any scenario. |

## Running locally

### Linux (host development)

From inside the `gcc` devcontainer:

```bash
behave Bdd/features/syslog.feature              # one feature
behave --tags='not @wip and not @no_rtc' Bdd/features/   # full Linux suite
```

The target binary and oracle path default to the values
`environment.before_all` sets for `BDD_TARGET=linux`; no env vars needed.

### FreeRTOS-on-QEMU

Switch the devcontainer service to `freertos-target` (see
[`docs/containers.md`](../docs/containers.md) — switch the `service:` line in
`.devcontainer/devcontainer.json` and rebuild). After rebuild, build the ELF
once and run Behave from inside the container:

```bash
cmake --preset freertos-cross
cmake --build --preset freertos-cross --target SolidSyslogBddTarget
behave --tags='not @wip and not @freertoswip and not @rtc and not @windows_wip and (@udp or @tcp or @tls or @mtls)' Bdd/features/
```

`BDD_TARGET=freertos` and `EXAMPLE_BINARY=build/freertos-cross/...` are
preset on the service so Behave dispatches to the QEMU driver
automatically. The driver spawns `qemu-system-arm -M mps2-an385 -serial
stdio …` and pipes `set` / `send` / `quit` over the UART; the QEMU
guest's slirp gateway `10.0.2.2` reaches the paired `syslog-ng-freertos`
oracle on the shared loopback.

#### Fault-finding tips

- **No prompt within 30 seconds**: the `SolidSyslog>` prompt is printed by
  the FreeRTOS interactive task, which the IP-network event hook only
  spawns once `eNetworkUp` fires. Add `-d guest_errors` to the QEMU args
  (in `target_driver.py::_QEMU_BASE_ARGS`) to surface CPU exceptions, or
  detach the same QEMU command from Behave and drive it manually:

  ```bash
  qemu-system-arm -M mps2-an385 -m 16M -display none -serial stdio \
      -icount shift=auto,sleep=off,align=off \
      -netdev user,id=net0 -net nic,netdev=net0,model=lan9118 \
      -kernel build/freertos-cross/Bdd/Targets/FreeRtos/SolidSyslogBddTarget.elf
  ```

- **Oracle is silent**: confirm the syslog-ng-freertos service is
  healthy (`docker compose ps`), and that the host you're testing
  resolves under the shared netns. Slice 3b.1.5's ARP-priming work
  means the very first datagram is delivered, but a stale slirp NAT
  table on a long-running compose network has been observed to
  swallow packets — `docker compose down -v` then up.
- **Want a quick frame inspection**: `cat Bdd/output/received.log`
  inside the freertos pair gives the parsed frame in
  `key=value` form (the syslog-ng template in
  `Bdd/syslog-ng/syslog-ng.conf`).
- **`nc localhost 5514` from the host won't reach `syslog-ng-freertos`.**
  Unlike `syslog-ng-linux`, the freertos oracle exposes no host ports
  — `freertos-target` reaches it via the shared netns on the pair's
  loopback. To send a probe frame manually, run inside the container:
  `docker compose exec freertos-target nc -u 127.0.0.1 5514`.

### VxWorks 6.4 (QEMU on a build machine)

Run `Bdd\Targets\VxWorks64\Run-VxWorks64Bdd.ps1` on the development machine. What it
needs, and what it does, is in [its README](Targets/VxWorks64/README.md#running-the-bdd-scenarios).
No CI lane runs this target.

## Running in CI

| Job | Compose pair |
|---|---|
| `bdd-linux-syslog-ng` | `syslog-ng-linux` + `behave-linux` |
| `bdd-freertos-qemu-plustcp` | `syslog-ng-freertos` + `behave-freertos` (cross image w/ QEMU + Behave) |
| `bdd-freertos-qemu-lwip` | `syslog-ng-freertos-lwip` + `behave-freertos-lwip` |
| `bdd-cmsis-qemu-lwip` | `syslog-ng-cmsis-lwip` + `behave-cmsis-lwip` |
| `bdd-windows-otel` | runner-direct (no compose); spawns `otelcol-contrib.exe` directly |

Each QEMU job depends on its cross-build job (`build-freertos-target-plustcp`,
`build-freertos-target-lwip`, `build-cmsis-target-lwip`) to upload the BDD target
ELF as an artifact, and downloads it before `docker compose up`.
