# VxWorks 6.4 BDD target

The SolidSyslog BDD target for VxWorks 6.4: a kernel VxWorks Image Project (VIP)
that boots in QEMU's Malta machine. The scripts here create the VIP, build it
with SolidSyslog linked in, and boot it.

Nothing the Wind River tools generate is kept in the repository. The VIP is
created from scratch by script, under `build\`, which git ignores.

## Prerequisites

- A Windows host.
- A licensed Wind River VxWorks 6.4 installation with the Diab toolchain, by
  default at `C:\WindRiver`. The GNU toolchain is optional.
- The BSP `malta4kc_qemu_mips32sf`, installed beside the others in
  `<WindRiverRoot>\vxworks-6.4\target\config\`. It is a bug-fixed version of
  Wind River's `malta4kc_mips32sf` BSP.
- QEMU, installed separately, with `qemu-system-mips.exe` - by default at
  `C:\Program Files\qemu\`.
- This checkout at a path without whitespace, which the Wind River make rules
  cannot handle.

## Create, build and boot

From PowerShell in this directory:

```powershell
.\New-VxWorks64Vip.ps1
.\Build-VxWorks64Vip.ps1
.\Start-VxWorks64Qemu.ps1 -WaitFor 'SolidSyslog VxWorks 6.4 BDD target: Core ran'
```

`New-VxWorks64Vip.ps1` creates the VIP with `vxprj` and adds the BDD target's
component (`99SolidSyslogVxWorks64Bdd.cdf`). The project's one source is
`BddTargetVxWorks64Headers.c`, which the project compiles at C89 with its own
flags to prove the public headers compile in a C89 application. Pass `-Force`
to replace an existing project, and `-Tool sfgnu` for the GNU toolchain.

`Build-VxWorks64Vip.ps1` builds the SolidSyslog library, Core and the
`VxWorks64` platform pack, then the BDD target's archive, then the
`default_rom` image and the raw `vxWorks_rom.bin` that QEMU loads, and prints
the image's SHA-256. The BDD target (`BddTargetVxWorks64.c`) is C99, so
`bddtarget.makefile` builds it as `libsolidsyslogbdd.a` with the library's own
dialect and flags. The full build output is kept beside the project as
`build-default_rom.log`, and the last lines on the console count every compiler
diagnostic in it with its message, or report none. `-Clean` rebuilds both
archives from scratch and relinks the image; for a fully clean build, recreate
the project with `New-VxWorks64Vip.ps1 -Force`.

`Start-VxWorks64Qemu.ps1 -WaitFor` boots the image, waits for the text on the
console, then stops QEMU. It fails if the text does not appear within
`-TimeoutSeconds` (default 60). With `-SerialTcp host:port` instead, the console
connects out to that address and QEMU is left running.

Each script describes its parameters: `Get-Help .\<script>.ps1 -Detailed`.

## Target checklist

No CI lane can build or run this target, so every pull request that touches the
VxWorks 6.4 platform records a run of these steps against its commit: each one as
pass, fail or not run.

1. `New-VxWorks64Vip.ps1 -Force` completes.
2. `Build-VxWorks64Vip.ps1 -Clean` ends with `Diagnostics: none`.
3. `Start-VxWorks64Qemu.ps1 -WaitFor 'SolidSyslog VxWorks 6.4 BDD target: Core ran'`
   reports `PASS`.
4. `Run-VxWorks64Bdd.ps1` passes: every scenario its tag filter selects passes
   against the commit (see [Running the BDD scenarios](#running-the-bdd-scenarios)).

## Running the BDD scenarios

`Run-VxWorks64Bdd.ps1` runs the Behave scenarios against this target from the
development machine. Behave runs there, natively; the runner checks out and
builds the commit under test, which must therefore have been pushed, and then
boots the target once for each scenario, with its console connecting back. The
oracle is the OpenTelemetry Collector the Windows runner uses
(`Bdd/otel/Install-OtelCollector.ps1`), which the script starts for the run
with `Bdd/otel/config.vxworks64.yaml`, listening for the target's syslog on UDP
and TCP 5514. A scenario that stops the collector to make an outage starts it
again from the same config.

It needs the job service and the runner set up as in
[Driving it from another machine](#driving-it-from-another-machine), and
nothing else on the development machine holding UDP or TCP 5514. A container that
publishes the port - the devcontainer's syslog-ng does - has to be stopped for
the run, and the script names it and refuses if one is up. `-SkipBuild` reuses
the image already built; `-Paths` runs the features given, under the same tag
filter. The filter, in the script, is this target's list of what it cannot do
yet: [`docs/bdd.md`](../../../docs/bdd.md#feature-tags) says what each
`@vxworks64wip` scenario waits for.

## Driving it from another machine

The runner lets a development machine build and boot this target on the
machine that holds the Wind River installation, without anyone typing on it.
The runner connects outwards only, to a job service on the development
machine, and runs one job at a time from a fixed set. Both live in `Runner/`.

On the development machine, which needs Python 3 and the `openssl` that Git
for Windows provides:

1. `python Runner\job_service.py init`, once. It writes a token, a certificate
   and its key to `%USERPROFILE%\.solidsyslog-runner`, and prints the
   certificate's thumbprint. None of them goes into the repository.
2. Allow inbound connections from the local network: TCP 8765 for the job
   service, TCP 8766 for the target's console, and UDP and TCP 5514 for syslog
   sent by the target. `LocalSubnet` follows the network, so an address changing on
   either machine needs no new rule. In an administrator PowerShell:

   ```powershell
   New-NetFirewallRule -DisplayName 'SolidSyslog runner' -Direction Inbound -Protocol TCP -LocalPort 8765,8766 -RemoteAddress LocalSubnet -Action Allow
   New-NetFirewallRule -DisplayName 'SolidSyslog runner syslog' -Direction Inbound -Protocol UDP -LocalPort 5514 -RemoteAddress LocalSubnet -Action Allow
   New-NetFirewallRule -DisplayName 'SolidSyslog runner syslog TCP' -Direction Inbound -Protocol TCP -LocalPort 5514 -RemoteAddress LocalSubnet -Action Allow
   ```

3. `python Runner\job_service.py serve`, and leave it running.

On the build machine, in its clone:

1. Copy the token into `%USERPROFILE%\.solidsyslog-runner\token`.
2. `.\Runner\Start-VxWorks64Runner.ps1 -Service https://<development machine>:8765 -Thumbprint <thumbprint>`,
   and leave it running. Ctrl+C stops it.

Then, on the development machine, `python Runner\job_service.py submit <job>`
queues a job, prints its log as it runs, and exits 0 only if it succeeded. It
stops waiting after 120 seconds, which suits every job but `build`; give that
longer with `--timeout`, for example `submit --timeout 1200 build`. Stopping
waiting does not stop the job: the runner carries on with it.

| Job | What the runner does |
|---|---|
| `checkout ref=<branch or commit>` | Fetches, then forces the clone to that commit |
| `build` | Checklist steps 1 and 2 |
| `boot-check "marker=<text>"` | Checklist step 3, waiting for that text |
| `qemu-start` | Boots the image, its console connecting to port 8766 here |
| `qemu-stop` | Stops the QEMU that `qemu-start` began |
| `status` | Reports the clone's commit, and whether QEMU is running |

`python Runner\job_service.py console`, started before `qemu-start`, shows
the target's console.

What protects the build machine: it accepts no connections. It trusts the job
service only by the pinned certificate, and a job only with the token. It runs
nothing outside the fixed set, refuses an argument it does not expect or one
that could be read as an option or quoting, and works only in its own clone.
The console and syslog are plaintext, and reach the development machine only
through the firewall rules above.

## How SolidSyslog gets into the image

The build script runs `Platform/VxWorks64/solidsyslog.makefile` against the
project's own Makefile, so the library is compiled with the build
specification's compiler and flags into `solidsyslog\` in the project. The
creation script sets the project's build macros with
`vxprj buildmacro set`:

- `CFLAGS` gains `Core/Interface`, `Platform/VxWorks64/Interface` and
  `Platform/VxWorks64/Compat`.
- `LIBS` names the BDD target's archive, then the library, ahead of
  `$(VX_OS_LIBS)`.
- With Diab, `PROJECT_BSP_FLAGS_EXTRA` gains `-ei1606`. Wind River's own
  `pciIntLib.c`, which the BSP's `sysLib.c` includes, raises `dcc:1606`. The
  macro reaches Wind River's sources and the ones `vxprj` generates - the BSP,
  `romStart.c`, `prjConfig.c` and `linkSyms.c` - so the project's own sources
  keep the warning.

The creation script also adds the network the target sends over: the IPv4 stack
with UDP, TCP, sockets and select, the host table, routing, and the END driver
for QEMU's PCnet adapter. QEMU loads the ROM image directly, so the boot line's addresses
are never used; `INCLUDE_ADDIF` puts the adapter on QEMU's user network instead,
as `10.0.2.15/24`. The target adds a default route through QEMU's gateway,
`10.0.2.2`, at start-up, and the harness names the collector with `set host`
and `set port` over the console.

The kernel header tree has no `<stdint.h>` or `<stdbool.h>`, which the
SolidSyslog headers include. `Platform/VxWorks64/Compat` supplies both, for the
library and for the project's own sources alike.
