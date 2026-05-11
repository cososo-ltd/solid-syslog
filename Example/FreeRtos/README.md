# FreeRTOS example

Cross-compiled FreeRTOS demo that runs under `qemu-system-arm` with the
`mps2-an385` (Cortex-M3) machine.

`SingleTask/` is the canonical FreeRTOS-on-QEMU example: SolidSyslog with
the portable `SolidSyslogCircularBuffer` + `SolidSyslogFreeRtosMutex`
drained by a dedicated FreeRTOS Service task, UdpSender over
FreeRTOS-Plus-TCP, the `SolidSyslogFreeRtosStaticResolver` pinned to the
QEMU slirp gateway (`10.0.2.2`), and the `Example/Common/ExampleInteractive`
runner. Drive identity / endpoint / PRIVAL fields over the UART command
channel with `set NAME VALUE`, emit N RFC 5424 datagrams with `send N`,
exit cleanly with `quit`. The BDD harness drives this binary by piping
the same commands through `qemu-system-arm`'s stdio UART; see
[`Bdd/README.md`](../../Bdd/README.md).

`Common/` carries the shared infrastructure (CMSDK UART driver, newlib
syscalls, mps2-an385 linker script, startup) and `cmake/` the
`arm-none-eabi.cmake` toolchain file. The `freertos-cross` CMake preset
and the `freertos-target` devcontainer service
([`docs/containers.md`](../../docs/containers.md)) carry everything
needed to build and run.

## In VS Code

The simplest path. Switch into the FreeRTOS target devcontainer and use
the standard Ctrl+Shift+B / F5 keys:

1. In [.devcontainer/devcontainer.json](../../.devcontainer/devcontainer.json),
   change `"service": "gcc"` to `"service": "freertos-target"`.
2. `Ctrl+Shift+P` → "Dev Containers: Rebuild Container".
3. **Build** — `Ctrl+Shift+B` runs the `build and test` task, which under
   `freertos-cross` builds the SingleTask ELF (skipping CppUTest, which
   isn't in the cross image).
4. **Run** — `Ctrl+Shift+P` → "Tasks: Run Task" → "run on QEMU (FreeRTOS)".
   The integrated terminal shows the `SolidSyslog>` prompt; type
   `send 1` / `quit` to exercise the example. `Ctrl+A` then `x` quits
   QEMU.
5. **Debug with breakpoints** — `F5` runs the
   `Debug FreeRTOS SingleTask (QEMU)` launch config (cortex-debug +
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
cmake --build --preset freertos-cross --target SolidSyslogFreeRtosSingleTask
```

The output ELF is at:

```text
build/freertos-cross/Example/FreeRtos/SingleTask/SolidSyslogFreeRtosSingleTask.elf
```

## Run under QEMU

```bash
qemu-system-arm \
    -M mps2-an385 -m 16M \
    -display none -serial stdio \
    -icount shift=auto,sleep=off,align=off \
    -netdev user,id=net0 \
    -net nic,netdev=net0,model=lan9118 \
    -kernel build/freertos-cross/Example/FreeRtos/SingleTask/SolidSyslogFreeRtosSingleTask.elf
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
    -kernel build/freertos-cross/Example/FreeRtos/SingleTask/SolidSyslogFreeRtosSingleTask.elf \
    -s -S
```

In a second terminal — attach GDB and run to `main`:

```bash
arm-none-eabi-gdb \
    build/freertos-cross/Example/FreeRtos/SingleTask/SolidSyslogFreeRtosSingleTask.elf
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
(running `ExampleInteractive_Run`) and the Service task (draining the
`SolidSyslogCircularBuffer` and pushing through the UDP sender). Each
`send N` line over the UART emits N RFC 5424 datagrams to
`{10.0.2.2, port=g_port}`.

The FreeRTOS GCC ARM_CM3 port supplies `vPortSVCHandler`,
`xPortPendSVHandler`, and `xPortSysTickHandler`.
[FreeRTOSConfig.h](SingleTask/FreeRTOSConfig.h) aliases them to the
standard CMSIS names (`SVC_Handler`, `PendSV_Handler`, `SysTick_Handler`)
so the vector table in `startup.c` can reference the canonical names.
