# FreeRTOS + lwIP (Raw API) link-probe target

`SolidSyslogBddTargetLwip` is a **cross-build link probe**, not a runnable BDD
target. It exists to prove — cheaply, in CI — that the `Platform/LwipRaw/`
adapter tree (`Address`, `Resolver`, `Datagram`, `TcpStream`, `Marshal`)
cross-compiles and links for a Cortex-M3 FreeRTOS target against lwIP core,
**with zero dependency on `Platform/PlusTcp/` or FreeRTOS-Plus-TCP** (S28.07,
epic E28).

It is selected by `SOLIDSYSLOG_FREERTOS_NET=LWIP` (see the top-level
`CMakeLists.txt`), built by the `freertos-cross-lwip` preset and the advisory
`build-freertos-target-lwip` CI lane. There is no LAN9118 netif driver and no
QEMU run: `main.c` starts the scheduler and a single probe task that
`_Create`/`_Destroy`s each adapter, so the linker must resolve every adapter
entry point and the lwIP core symbols behind it.

`lwipopts.h` pins `NO_SYS=1` (the LwipRaw default direct-call marshal is
correct for a single lwIP-owning context). The worked `NO_SYS=0` runtime — a
real netif, `tcpip_init`, the `tcpip_callback` marshal (S28.06), and the UDP
BDD feature green on QEMU — lands with **S28.09**, which reuses this
directory's `lwipopts.h`, `FreeRTOSConfig.h`, and `arch/cc.h` and grows
`main.c` into the full integration.

## Build

```sh
cmake --preset freertos-cross-lwip
cmake --build --preset freertos-cross-lwip --target SolidSyslogBddTargetLwip
```

Requires the `cpputest-freertos-cross` container (arm-none-eabi toolchain,
`FREERTOS_KERNEL_PATH=/opt/freertos/kernel`, `LWIP_PATH=/opt/lwip`).
