# FreeRTOS example

Cross-compiled FreeRTOS demos that run under `qemu-system-arm` with the
`mps2-an385` (Cortex-M3) machine.

| Subdir | Purpose |
|---|---|
| `HelloWorld/` | Single FreeRTOS task that prints `hello` via semihosting. Bring-up smoke test for E08; no syslog content yet. |

Future stories under E08 will add UDP / TCP / TLS examples alongside.

## In VS Code

The simplest path. Switch into the FreeRTOS target devcontainer and use
the standard Ctrl+Shift+B / F5 keys:

1. In [.devcontainer/devcontainer.json](../../.devcontainer/devcontainer.json),
   change `"service": "gcc"` to `"service": "freertos-target"`.
2. `Ctrl+Shift+P` → "Dev Containers: Rebuild Container".
3. **Build** — `Ctrl+Shift+B` runs the `build and test` task, which under
   `freertos-cross` builds the hello-world ELF (skipping CppUTest, which
   isn't in the cross image).
4. **Run** — `Ctrl+Shift+P` → "Tasks: Run Task" → "run on QEMU (FreeRTOS)".
   Output appears in the integrated terminal; `Ctrl+C` quits.
5. **Debug with breakpoints** — `F5` runs the
   `Debug FreeRTOS HelloWorld (QEMU)` launch config (cortex-debug +
   qemu-system-arm + arm-none-eabi-gdb). The session pauses at `main`;
   set breakpoints in the editor as normal. `F10` steps over, `F11`
   steps into, `F5` continues. The QEMU semihosting stdout streams to
   the Debug Console.

When done, revert `"service": "gcc"` and rebuild.

## On the command line

Inside the `cpputest-freertos-cross` devcontainer (or any environment that
has `arm-none-eabi-gcc`, `arm-none-eabi-gdb`, and `qemu-system-arm` on
`PATH`):

```bash
cmake --preset freertos-cross
cmake --build --preset freertos-cross
```

The output ELF is at:

```text
build/freertos-cross/Example/FreeRtos/HelloWorld/SolidSyslogFreeRtosHelloWorld.elf
```

## Run under QEMU

```bash
qemu-system-arm \
    -M mps2-an385 -m 16M \
    -nographic \
    -semihosting-config enable=on,target=native \
    -kernel build/freertos-cross/Example/FreeRtos/HelloWorld/SolidSyslogFreeRtosHelloWorld.elf
```

You should see:

```text
hello from FreeRTOS on QEMU mps2-an385
```

QEMU stays running because the FreeRTOS scheduler keeps idling. Quit with
`Ctrl+A` then `x`.

## GDB attach (manual)

The VS Code F5 path above does this for you under the hood. Use the
manual flow when you need to reach behaviour the launch config doesn't
expose, or when working outside VS Code.

In one terminal — start QEMU paused, listening for GDB on `:1234`:

```bash
qemu-system-arm \
    -M mps2-an385 -m 16M \
    -nographic \
    -semihosting-config enable=on,target=native \
    -kernel build/freertos-cross/Example/FreeRtos/HelloWorld/SolidSyslogFreeRtosHelloWorld.elf \
    -s -S
```

In a second terminal — attach GDB and run to `main`:

```bash
arm-none-eabi-gdb \
    build/freertos-cross/Example/FreeRtos/HelloWorld/SolidSyslogFreeRtosHelloWorld.elf
(gdb) target remote :1234
(gdb) break main
(gdb) continue
```

## How it works

mps2-an385 boots from `0x00000000`. The linker script ([mps2-an385.ld](HelloWorld/mps2-an385.ld))
places the Cortex-M3 vector table at the start of the ELF, which QEMU loads
at that address. `Reset_Handler` (in [startup.c](HelloWorld/startup.c))
initialises `.data` and `.bss`, calls `__libc_init_array`, then calls
`main()`.

`main()` opens stdout via `initialise_monitor_handles()` — a newlib rdimon
function that wires `printf` to QEMU's semihosting trap (`BKPT 0xAB`) — then
creates one task and starts the FreeRTOS scheduler. QEMU intercepts the trap
when launched with `-semihosting-config enable=on,target=native` and writes
the string to its own stdout.

The FreeRTOS GCC ARM_CM3 port supplies `vPortSVCHandler`, `xPortPendSVHandler`,
and `xPortSysTickHandler`. [FreeRTOSConfig.h](HelloWorld/FreeRTOSConfig.h)
aliases them to the standard CMSIS names (`SVC_Handler`, `PendSV_Handler`,
`SysTick_Handler`) so the vector table in `startup.c` can reference the
canonical names.
