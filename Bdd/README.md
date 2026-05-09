# BDD scenarios

Gherkin features under `features/` are driven by [Behave](https://behave.readthedocs.io/)
against a `syslog-ng` oracle (Linux + FreeRTOS) or `otelcol-contrib` (Windows). The
same feature files run on every target — the active runner is selected by the
`BDD_TARGET` environment variable, which `features/environment.py` reads in
`before_all` and dispatches via `features/steps/target_driver.py`.

| `BDD_TARGET` | Example-under-test | Oracle |
|---|---|---|
| `linux` (default) | `build/debug/Example/SolidSyslogExample` (native subprocess) | `syslog-ng-linux` |
| `windows` | `build/msvc-debug/Example/Debug/SolidSyslogExample.exe` (native subprocess) | `otelcol-contrib` |
| `freertos` | `build/freertos-cross/Example/FreeRtos/SingleTask/SolidSyslogFreeRtosSingleTask.elf` driven through `qemu-system-arm` | `syslog-ng-freertos` |

Each BDD target pairs with its own oracle service so jobs (and developers
switching containers) never interfere — see [`docs/containers.md`](../docs/containers.md)
for the compose layout.

## Tags

| Tag | Meaning |
|---|---|
| `@udp` / `@tcp` / `@tls` / `@mtls` | Transport-specific scenario; CI jobs filter by these. |
| `@windows_wip` | Skipped on the Windows runner (typically OS-specific behaviour the OTel oracle doesn't model). |
| `@freertoswip` | Skipped on the FreeRTOS-on-QEMU runner. The FreeRTOS SingleTask example bakes hard-coded `TEST_*` values for hostname, PROCID, and timestamp; consumes config via the interactive `set NAME VALUE` command rather than getopt; and does not yet wire `meta` / `origin` / `timeQuality` SD. Scenarios that depend on any of these are tagged today and the tag is removed slice-by-slice as each gap is closed. |
| `@wip` | Globally skipped on every runner. |

## Running locally

### Linux (host development)

From inside the `gcc` devcontainer:

```bash
behave Bdd/features/syslog.feature              # one feature
behave --tags='not @wip' Bdd/features/          # full Linux suite
```

The example binary and oracle path default to the values
`environment.before_all` sets for `BDD_TARGET=linux`; no env vars needed.

### FreeRTOS-on-QEMU

Switch the devcontainer service to `freertos-target` (see
[`docs/containers.md`](../docs/containers.md) — switch the `service:` line in
`.devcontainer/devcontainer.json` and rebuild). After rebuild, build the ELF
once and run Behave from inside the container:

```bash
cmake --preset freertos-cross
cmake --build --preset freertos-cross --target SolidSyslogFreeRtosSingleTask
behave --tags='not @wip and not @freertoswip and @udp' Bdd/features/
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
      -kernel build/freertos-cross/Example/FreeRtos/SingleTask/SolidSyslogFreeRtosSingleTask.elf
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

## Running in CI

| Job | Compose pair |
|---|---|
| `bdd-linux-syslog-ng` | `syslog-ng-linux` + `behave-linux` |
| `bdd-freertos-qemu` | `syslog-ng-freertos` + `behave-freertos` (cross image w/ QEMU + Behave) |
| `bdd-windows-otel` | runner-direct (no compose); spawns `otelcol-contrib.exe` directly |

`bdd-freertos-qemu` depends on `build-freertos-target` to upload the
SingleTask ELF as an artifact; the new job downloads it before
`docker compose up`.
