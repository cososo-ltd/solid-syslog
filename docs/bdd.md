# BDD Testing

## Overview

BDD (Behaviour-Driven Development) tests verify SolidSyslog end-to-end: a C program sends an
RFC 5424 syslog message over UDP to a real syslog-ng instance, and Behave (Python) asserts that
the received message was parsed into the expected fields.

This catches conformance issues that unit tests cannot — syslog-ng's RFC 5424 parser is the test
oracle, so a malformed message that happens to match a byte-level expectation in a unit test will
fail here.

## Architecture

```
                                  ┌──────────────┐
                                  │    Behave     │
                                  │  (container)  │
                                  └──┬────────┬──┘
                                     │ runs   │ reads
                                     ▼        │
                              ┌──────────────┐│
                              │   Example    ││
                              │   binary     ││
                              └──────┬───────┘│
                                     │        │
                            UDP 5514 / TCP 5514│
                            TLS 6514 / mTLS 6515
                                     │        │
                                     ▼        │
                              ┌──────────────┐│
                              │   syslog-ng  ││
                              │  (container) ││
                              └──────┬───────┘│
                                     │ writes │
                                     ▼        ▼
                              Bdd/output/received*.log
```

BDD targets pair with their own oracle so jobs and developers
switching containers never interfere. Each pair lives on its own
bridge network so the `syslog-ng` DNS alias is scoped per-pair —
see [`Bdd/README.md`](../Bdd/README.md) for the per-target compose
layout, the `BDD_TARGET` env-var contract, and FreeRTOS local
fault-finding tips.

| Target | Behave runner | Oracle |
|---|---|---|
| Linux | `behave-linux` | `syslog-ng-linux` |
| FreeRTOS | inside `freertos-target` (cross image carries QEMU + Behave) | `syslog-ng-freertos` (shared netns; QEMU slirp `10.0.2.2` reaches it on loopback) |
| Windows | `behave` on the runner | `otelcol-contrib` (no compose; runner-direct) |

The Linux example binary is built in the `gcc` container but executed by Behave via
`subprocess.run`. Both services share the workspace mount, so `Bdd/output/received.log`
is visible to both syslog-ng (writer) and Behave (reader) without any network file
transfer. The FreeRTOS ELF is built in the cross image and run under
`qemu-system-arm` from inside the `freertos-target` service; Behave drives it
through the QEMU UART (`-serial stdio`).

## Key files

| File | Purpose |
|---|---|
| `Bdd/syslog-ng/syslog-ng.conf` | syslog-ng configuration — UDP source, key=value template output |
| `ghcr.io/davidcozens/behave` | GHCR image — Debian trixie + Python + Behave ([source](https://github.com/DavidCozens/BehaveDocker)) |
| `Bdd/output/` | Shared directory — syslog-ng writes here, Behave reads |
| `Bdd/features/` | Gherkin feature files and step definitions |
| `Example/SolidSyslogExample.c` | Minimal C program that creates a logger and sends one message. Accepts `--facility` and `--severity` CLI flags (defaults to local0/info) |
| `Example/CMakeLists.txt` | Builds the example binary, linked against the SolidSyslog library |

## syslog-ng configuration

The syslog-ng config (`Bdd/syslog-ng/syslog-ng.conf`) declares four `syslog()` sources —
UDP and TCP on 5514, server-auth TLS on 6514, and mutual TLS (`peer-verify(required-trusted)`)
on 6515 — all parsing RFC 5424 natively. Every message is tee'd to both `received.log`
(catch-all) and a per-transport log file (`received_udp.log`, `received_tcp.log`,
`received_tls.log`, `received_mtls.log`) using a key=value template:

```
PRIORITY=14 TIMESTAMP=2009-03-23T00:00:00+00:00 HOSTNAME=TestHost APP_NAME=TestApp PROCID=1234 MSGID=TestMsgId STRUCTURED_DATA= MSG=Test message
```

### Important syslog-ng behaviours

These are deliberate syslog-ng design choices, not bugs:

| Behaviour | Detail |
|---|---|
| **`keep-hostname(yes)`** | Required in the source config. Without it, syslog-ng replaces the RFC 5424 HOSTNAME field with the sender's transport-level IP address. `$HOST` then reflects the parsed payload hostname; `$HOST_FROM` always contains the sender IP regardless. |
| **Timestamp reformat** | syslog-ng normalises timestamps via `$ISODATE`. A sent value of `...T00:00:00.000Z` will appear as `...T00:00:00+00:00`. |
| **Nil structured data** | The RFC 5424 nil value `-` for SDATA produces an empty string in `$SDATA`, not `-`. |
| **MSG leading space** | BSD syslog convention may prepend a space to the message body. Step definitions should use `.strip()`. |

### flush_lines(1)

The destination uses `flush_lines(1)` so that each message is written to disk immediately.
This avoids flaky BDD tests that poll for output before the buffer is flushed.

## Reconfiguring syslog-ng from Behave

The `syslog-ng` and `behave` containers share a named Docker volume (`syslog-ng-ctl`) mounted
at `/var/lib/syslog-ng`. This exposes syslog-ng's Unix control socket to the behave container,
allowing Behave to trigger a config reload without SSH or `docker compose exec`.

Since the syslog-ng config file is bind-mounted from the workspace (`Bdd/syslog-ng/syslog-ng.conf`),
Behave can write a new config to that path via the shared workspace mount and then reload:

```python
import socket

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect("/var/lib/syslog-ng/syslog-ng.ctl")
sock.sendall(b"RELOAD\n")
response = sock.recv(1024)  # b'OK Config reload successful\n.\n'
sock.close()
```

This is exercised by scenarios that need a different syslog-ng source configuration — for
example, the TLS and mTLS scenarios swap in tighter `peer-verify` settings and reload without
restarting the container.

## Feature tags

Features are tagged by **capability** — what the system under test must provide for the
scenario to be meaningful. Runners declare which capabilities they have by excluding the
tags they lack. This lets us add new runners (Windows, bare-metal RTOS, TLS-only client,
etc.) without rewriting feature tags.

| Tag | Meaning |
| --- | --- |
| `@udp` | Needs UDP transport |
| `@tcp` | Needs TCP transport (RFC 6587 framing) |
| `@tls` | Needs TLS transport (RFC 5425, server-auth) |
| `@mtls` | Needs mutual TLS (client cert + key) |
| `@buffered` | Needs a Linux-only buffered example capability beyond a basic ring buffer + service thread — file-backed block store, switching sender between transports, or syslog-ng reload via the UNIX control socket. The cross-platform "single message via UDP/TCP through a real buffer" path is *not* `@buffered` post-S13.18; TLS and mTLS dropped `@buffered` in S13.19 once the OTel oracle gained TLS receivers (Windows otelcol-contrib listens on 6514 / 6515 with `client_ca_file` for mTLS). |

Three rollout markers are also used (temporary; remove once the scenario passes):

| Tag | Meaning |
| --- | --- |
| `@wip` | Skip everywhere — work in progress |
| `@windows_wip` | Skip on Windows only — should work but not yet verified |
| `@freertoswip` | Skip on FreeRTOS only — scenario currently fails or errors on the FreeRTOS-on-QEMU target and is gated until the relevant capability lands. Each tagged scenario is a follow-up tied to a specific gap; the tag is removed scenario-by-scenario as the gap closes. The early bring-up reasons (hardcoded `TEST_*` values, missing SD elements, cmdline-only args) have all been closed out by S08.03 + S08.04 slices. |

Runner tag filters:

| Runner | Filter |
| --- | --- |
| Linux (syslog-ng) | `not @wip` |
| FreeRTOS (syslog-ng-freertos via QEMU) | `not @wip and not @freertoswip and @udp` |
| Windows (OTel Collector) | `not @wip and not @windows_wip and not @buffered` |

## Two oracles, one step file

Step definitions read the active oracle from `ORACLE_FORMAT`:

| Oracle | `ORACLE_FORMAT` | `RECEIVED_LOG` default | Runs on |
| --- | --- | --- | --- |
| syslog-ng (key=value text) | `syslog-ng` | `Bdd/output/received.log` | Linux container |
| OTel Collector Contrib (JSON Lines) | `otel-jsonl` | `Bdd/output/received.jsonl` | Windows native |

`parse_oracle_line` dispatches to the right parser; both produce the same flat field dict
(`PRIORITY`, `TIMESTAMP`, `HOSTNAME`, `APP_NAME`, `PROCID`, ...) so the `Then` steps don't
need to know which oracle is in use. Walking-skeleton scope only — `STRUCTURED_DATA`
re-rendering for the OTel parser is added when the structured-data scenarios are
promoted out of `@windows_wip`.

## Local Windows BDD setup

Prerequisites: Python 3.13+ on `PATH` (`winget install Python.Python.3.13`), MSVC + vcpkg
for the `msvc-debug` build.

> **Shell:** the recipe below uses bash syntax (background `&`, inline `VAR=value command`).
> Run it from **Git Bash for Windows**, which ships with the Git installer that's already
> required for this repo. PowerShell users either translate (`Start-Process` for backgrounding,
> `$env:VAR = "..."` for env vars) or invoke `bash -c "..."`.

```pwsh
# 1. Install behave (pinned version matches the Linux container image)
pip install -r Bdd/requirements.txt

# 2. Download otelcol-contrib (pinned version, SHA-256 verified)
powershell -ExecutionPolicy Bypass -File Bdd/otel/Install-OtelCollector.ps1

# 3. Build the Windows example
cmake --preset msvc-debug
cmake --build --preset msvc-debug --target SolidSyslogWindowsExample

# 4. Start the OTel oracle (binds 127.0.0.1:5514 udp+tcp, 6514 tls, 6515 mtls)
./Bdd/otel/bin/otelcol-contrib.exe --config=Bdd/otel/config.yaml &

# 5. Run the Windows-eligible scenarios
EXAMPLE_BINARY=build/msvc-debug/Example/Debug/SolidSyslogExample.exe \
RECEIVED_LOG=Bdd/output/received.jsonl \
ORACLE_FORMAT=otel-jsonl \
behave --tags='not @wip and not @windows_wip and not @buffered' Bdd/features/
```

If port 5514 is busy, stop the dev-container's syslog-ng:
`docker compose -f .devcontainer/docker-compose.yml stop syslog-ng`

## Test isolation

Behave uses line-count tracking for test isolation. The `Given` step records the current line
count of `received.log`, and the `When` step waits for a new line to appear before parsing it.
This avoids restarting the syslog-ng container between scenarios.

## Running BDD tests locally

From WSL or a host terminal (not inside the devcontainer):

```bash
# Start the Linux oracle
docker compose -f .devcontainer/docker-compose.yml up -d syslog-ng-linux

# Build the example binary (inside the gcc container)
docker compose -f .devcontainer/docker-compose.yml exec gcc \
    cmake --preset debug
docker compose -f .devcontainer/docker-compose.yml exec gcc \
    cmake --build --preset debug

# Run Behave
docker compose -f .devcontainer/docker-compose.yml run --rm behave-linux \
    behave Bdd/features/
```

For the FreeRTOS pair (cross-build the ELF, then run Behave via QEMU
inside `freertos-target`), see [`Bdd/README.md`](../Bdd/README.md).

## Verifying syslog-ng manually

To send a test message and check that syslog-ng is receiving and parsing correctly:

```bash
echo '<14>1 2009-03-23T00:00:00.000Z TestHost TestApp 1234 TestMsgId - Test message' \
    | nc -u -w1 localhost 5514

cat Bdd/output/received.log
```

If the file is not created, check that:
1. The syslog-ng container is running: `docker compose -f .devcontainer/docker-compose.yml ps`
2. The config is mounted correctly: `docker compose -f .devcontainer/docker-compose.yml exec syslog-ng cat /etc/syslog-ng/syslog-ng.conf`
3. After editing `syslog-ng.conf`, reload the config — either via the control socket (see above) or by recreating the container: `docker compose rm -sf syslog-ng && docker compose up -d syslog-ng`
