# FreeRTOS BDD target

Cross-compiled FreeRTOS binary that runs under `qemu-system-arm` with the
`mps2-an385` (Cortex-M3) machine — the BDD harness drives it through
QEMU's stdio UART.

SolidSyslog runs here with the portable `SolidSyslogCircularBuffer` +
`SolidSyslogFreeRtosMutex` drained by a dedicated FreeRTOS Service task,
a `SolidSyslogSwitchingSender` wrapping a UDP sender (over
`SolidSyslogFreeRtosDatagram`) and a TCP sender (over
`SolidSyslogFreeRtosTcpStream` + `SolidSyslogStreamSender`), the
`SolidSyslogFreeRtosStaticResolver` pinned to the QEMU slirp gateway
(`10.0.2.2`), and the `Bdd/Targets/Common/BddTargetInteractive` runner.
Drive identity / endpoint / PRIVAL fields over the UART command channel
with `set NAME VALUE`, flip the active transport at runtime with
`switch udp` / `switch tcp` (routed through `BddTargetSwitchConfig`),
emit N RFC 5424 messages with `send N`, exit cleanly with `quit`. See
[`Bdd/README.md`](../../Bdd/README.md) for how Behave pipes these
commands through `qemu-system-arm`'s stdio UART.

**Persistent store-and-forward** is wired via the ChaN FatFs adapter
(`SolidSyslogFatFsFile`) over a semihosting-backed `diskio.c` —
QEMU's BKPT 0xAB traps route FatFs's block I/O to a host-resident
`solidsyslog-disk.img` (cleared by Behave's `before_scenario` and
`after_scenario`), so writes survive a QEMU restart for the
`power_cycle_replay` scenario. `set store file` flips the live store
from `SolidSyslogNullStore` to a `SolidSyslogBlockStore` over the
FatFs file backend; `set max-blocks` / `set max-block-size` /
`set discard-policy` / `set halt-exit` parameterise the BlockStore
config before that rebuild. `set shutdown 1` performs a graceful
teardown — destroys our objects (which close the FatFs files),
`f_unmount`s, then `SemihostingExit`s — and is what the BDD
`the client is killed` step uses on FreeRTOS so the next session's
`f_mount` finds the directory entries up-to-date.

Production integrators ship their own `diskio.c` (flash / SD / eMMC)
plus, if `FF_FS_REENTRANT=1`, their own `ffsystem.c`. The semihosting
shape used here is BDD-target glue, not a reference port.

`Common/` carries the shared infrastructure (CMSDK UART driver, newlib
syscalls, mps2-an385 linker script, startup) and `cmake/` the
`arm-none-eabi.cmake` toolchain file. `diskio.c` and `ffsystem.c` are
the FatFs ports specific to this BDD target. The `freertos-cross`
CMake preset and the `freertos-target` devcontainer service
([`docs/containers.md`](../../docs/containers.md)) carry everything
needed to build and run.

## Tuning for your MCU

[solidsyslog_user_tunables.h](solidsyslog_user_tunables.h) is the worked
example of the E21 port-time configurability mechanism — the
`freertos-cross` preset points `SOLIDSYSLOG_USER_TUNABLES_FILE` at it so
the library compiles with `SOLIDSYSLOG_MAX_MESSAGE_SIZE 512` instead of
the default 2048. That reclaims ~4.5KB of stack frame per `SolidSyslog_Log`
call on Cortex-M3, where 4KB task stacks are normal.

For your own port, copy the pattern: drop a `solidsyslog_user_tunables.h`
into your build tree, override whichever
[`SolidSyslogTunablesDefaults.h`](../../../Core/Interface/SolidSyslogTunablesDefaults.h)
macros suit the target, and set `-DSOLIDSYSLOG_USER_TUNABLES_FILE=<path>`
in your CMake config (preset, cache file, or `-D` on the command line —
all equivalent).

## In VS Code

The simplest path. Switch into the FreeRTOS target devcontainer and use
the standard Ctrl+Shift+B / F5 keys:

1. In [.devcontainer/devcontainer.json](../../.devcontainer/devcontainer.json),
   change `"service": "gcc"` to `"service": "freertos-target"`.
2. `Ctrl+Shift+P` → "Dev Containers: Rebuild Container".
3. **Build** — `Ctrl+Shift+B` runs the `build and test` task, which under
   `freertos-cross` builds the BDD target ELF (skipping CppUTest, which
   isn't in the cross image).
4. **Run** — `Ctrl+Shift+P` → "Tasks: Run Task" → "run on QEMU (FreeRTOS)".
   The integrated terminal shows the `SolidSyslog>` prompt; type
   `send 1` / `quit` to exercise the target. `Ctrl+A` then `x` quits
   QEMU.
5. **Debug with breakpoints** — `F5` runs the
   `Debug FreeRTOS BDD Target (QEMU)` launch config (cortex-debug +
   qemu-system-arm + arm-none-eabi-gdb). The session pauses at `main`;
   set breakpoints in the editor as normal. `F10` steps over, `F11`
   steps into, `F5` continues.

When done, revert `"service": "gcc"` and rebuild.

## On the command line

Inside the `cpputest-freertos-cross` devcontainer (or any environment that
has `arm-none-eabi-gcc`, `arm-none-eabi-gdb`, and `qemu-system-arm` on
`PATH`):

```bash
cmake --preset freertos-cross
cmake --build --preset freertos-cross --target SolidSyslogBddTarget
```

The output ELF is at:

```text
build/freertos-cross/Bdd/Targets/FreeRtos/SolidSyslogBddTarget.elf
```

## Run under QEMU

```bash
qemu-system-arm \
    -M mps2-an385 -m 16M \
    -display none -serial stdio \
    -icount shift=auto,sleep=off,align=off \
    -netdev user,id=net0 \
    -net nic,netdev=net0,model=lan9118 \
    -kernel build/freertos-cross/Bdd/Targets/FreeRtos/SolidSyslogBddTarget.elf
```

You should see the `SolidSyslog>` prompt once the IP stack has come up
(`eNetworkUp`). Type `send 1` to emit one RFC 5424 datagram to
`10.0.2.2:514` (override with `set host` / `set port` first if you have
a different oracle reachable through the slirp gateway), then `quit` to
delete the interactive task. QEMU stays running because the FreeRTOS
scheduler keeps idling — quit with `Ctrl+A` then `x`.

## GDB attach (manual)

The VS Code F5 path above does this for you under the hood. Use the
manual flow when you need to reach behaviour the launch config doesn't
expose, or when working outside VS Code.

In one terminal — start QEMU paused, listening for GDB on `:1234`:

```bash
qemu-system-arm \
    -M mps2-an385 -m 16M \
    -display none -serial stdio \
    -netdev user,id=net0 \
    -net nic,netdev=net0,model=lan9118 \
    -kernel build/freertos-cross/Bdd/Targets/FreeRtos/SolidSyslogBddTarget.elf \
    -s -S
```

In a second terminal — attach GDB and run to `main`:

```bash
arm-none-eabi-gdb \
    build/freertos-cross/Bdd/Targets/FreeRtos/SolidSyslogBddTarget.elf
(gdb) target remote :1234
(gdb) break main
(gdb) continue
```

## How it works

mps2-an385 boots from `0x00000000`. The linker script
([mps2-an385.ld](Common/mps2-an385.ld)) places the Cortex-M3 vector table
at the start of the ELF, which QEMU loads at that address.
`Reset_Handler` (in [startup.c](Common/startup.c)) initialises `.data`
and `.bss`, calls `__libc_init_array`, then calls `main()`.

`main()` initialises the CMSDK UART (CMSDK_UART0 @ `0x40004000`) for
polled `_read` / `_write` via newlib retargeting
([Syscalls.c](Common/Syscalls.c)), brings up FreeRTOS-Plus-TCP with a
static IPv4 of `10.0.2.15` on the QEMU slirp network, and starts the
scheduler. On link-up the IP-task event hook spawns the interactive task
(running `BddTargetInteractive_Run`) and the Service task (draining the
`SolidSyslogCircularBuffer` and pushing through the
`SolidSyslogSwitchingSender`'s active inner — UDP by default, TCP after
`switch tcp`). Each `send N` line over the UART emits N RFC 5424
messages to `{10.0.2.2, port=g_port}` over whichever transport is
currently selected.

The FreeRTOS GCC ARM_CM3 port supplies `vPortSVCHandler`,
`xPortPendSVHandler`, and `xPortSysTickHandler`.
[FreeRTOSConfig.h](FreeRTOSConfig.h) aliases them to the
standard CMSIS names (`SVC_Handler`, `PendSV_Handler`, `SysTick_Handler`)
so the vector table in `startup.c` can reference the canonical names.
