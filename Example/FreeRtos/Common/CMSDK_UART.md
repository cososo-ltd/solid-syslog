# CMSDK APB UART — driver contract

Reference document for `CmsdkUart.{h,c}` — the polled UART0 driver used by
the FreeRTOS SingleTask example to route `printf` over QEMU `-serial stdio`.
Written during S08.03 slice 2 (#290) review after a code-review pass found
the v1 driver was missing `STATE.TX_FULL` polling.

The aim of this document is to capture *what the hardware demands* (vs what
QEMU happens to tolerate), so that future readers — or readers porting to
real mps2-an385 silicon, or to a FreeRTOS-Plus-TCP environment that drives
UART concurrently with the IP stack — know which parts of the driver are
load-bearing and which parts are slice-2-scope simplifications.

Each section flags places where QEMU's behaviour and silicon's specified
behaviour may diverge — those are the spots where a QEMU-only test passes
and silicon fails.

---

## 1. Authoritative register reference

### 1.1 What "CMSDK APB UART" means

The peripheral is the `cmsdk_apb_uart` block from Arm's Cortex-M System
Design Kit (CMSDK), specified in the *Cortex-M System Design Kit Technical
Reference Manual* (ARM DDI 0479C/D), §4.3 *APB UART*. The MPS2 AN385 FPGA
image instantiates five copies in the CMSDK APB subsystem (UART0–UART4);
see *Application Note AN385: Cortex-M3 SMM on V2M-MPS2* (ARM DAI 0385D),
§3.7 "CMSDK APB subsystem".

Both the silicon RTL and QEMU implement the same programmer's model. The
mismatches are in **timing, side-effects, and what happens between the bus
access and the wire** — covered per-register below.

### 1.2 Base addresses on `mps2-an385`

From AN385 §3.7 Table 3-4 ("APB Memory Map"), corroborated by QEMU's
`hw/arm/mps2.c` machine model:

| UART  | Base       | RX IRQ | TX IRQ | Combined-overrun IRQ |
|-------|------------|--------|--------|----------------------|
| UART0 | 0x40004000 | 0      | 1      | 12 (shared)          |
| UART1 | 0x40005000 | 2      | 3      | 12 (shared)          |
| UART2 | 0x40006000 | 4      | 5      | 12 (shared)          |
| UART3 | 0x40007000 | 18     | 19     | none                 |
| UART4 | 0x40009000 | 20     | 21     | none                 |

The driver currently uses UART0 and is hardcoded to `0x40004000` in
`main.c`. Other UARTs are reference only.

### 1.3 Register map (offsets from base)

| Offset | Name                  | Access   | Reset    |
|--------|-----------------------|----------|----------|
| 0x000  | DATA                  | R/W      | 0        |
| 0x004  | STATE                 | mixed    | 0        |
| 0x008  | CTRL                  | R/W      | 0        |
| 0x00C  | INTSTATUS / INTCLEAR  | R/W1C    | 0        |
| 0x010  | BAUDDIV               | R/W      | 0        |
| 0xFD0–0xFFC | PID4..CID3       | RO       | fixed    |

### 1.4 Bit semantics

**DATA (0x000) — R/W, 8 valid bits**

- **Write:** loads transmit holding byte. Effective only if `CTRL.TX_EN=1`.
  In QEMU, a write while `STATE.TXFULL` is already set sets
  `STATE.TXOVERRUN` and *replaces* `txbuf` with the new value.
- **Read:** returns the last received byte. Reading clears `STATE.RXFULL`
  as a side-effect.

**STATE (0x004)**

| Bit | Name      | Direction  |
|-----|-----------|------------|
| 0   | TXFULL    | RO from software (HW sets/clears) |
| 1   | RXFULL    | RO from software (HW sets/clears) |
| 2   | TXOVERRUN | W1C        |
| 3   | RXOVERRUN | W1C        |

Software cannot directly clear `TXFULL`/`RXFULL`. They clear as side-effects
of HW transmit completion (TXFULL) and DATA reads (RXFULL). Overrun bits
are write-1-to-clear: write `STATE = 0x4` to clear TXOVERRUN, `STATE = 0x8`
for RXOVERRUN, `0xC` for both.

**CTRL (0x008)**

| Bit | Name      |
|-----|-----------|
| 0   | TX_EN     |
| 1   | RX_EN     |
| 2   | TX_INTEN  |
| 3   | RX_INTEN  |
| 4   | TXO_INTEN |
| 5   | RXO_INTEN |
| 6   | HSTEST    (high-speed test mode — leave 0) |

In QEMU the writable mask is `0x7F`; bits 7+ are dropped silently.

**INTSTATUS / INTCLEAR (0x00C)**

Same address; reads return INTSTATUS, writes act as INTCLEAR (W1C).

| Bit | Name | Set by HW when                                               |
|-----|------|--------------------------------------------------------------|
| 0   | TX   | TXFULL transitions 1→0 *and* `CTRL.TX_INTEN=1`               |
| 1   | RX   | RXFULL transitions 0→1 *and* `CTRL.RX_INTEN=1`               |
| 2   | TXO  | `STATE.TXOVERRUN=1` AND `CTRL.TXO_INTEN=1` (combinational)   |
| 3   | RXO  | `STATE.RXOVERRUN=1` AND `CTRL.RXO_INTEN=1` (combinational)   |

**Critical: TX is edge-triggered.** From the Zephyr driver: *"In CMSDK
UART [TX] is an edge interrupt, firing on a state change of TX buffer
from full to empty."* RX is also edge-triggered, on the empty→full
transition. Pertinent only if/when this driver moves to interrupt-driven
TX (out of scope for slice 2; polled-only).

The overrun bits in INTSTATUS are *not* independently latched — they are
recomputed by the QEMU update function as
`intstatus_overrun = state_overrun & ctrl_overrun_inten`. So clearing the
overrun-status bit in STATE (via W1C to STATE) immediately clears the
corresponding overrun bit in INTSTATUS too.

**BAUDDIV (0x010) — R/W, 20 valid bits in QEMU (`& 0xFFFFF`)**

Baud rate = PCLK / BAUDDIV. Minimum legal value is **16**.

- **QEMU:** below 16 the value is ignored — `uart_baudrate_ok()` returns
  false for `bauddiv < 16`, so `uart_update_parameters()` becomes a
  no-op. The UART will still transmit/receive on the host pipe regardless,
  because QEMU's transmit path doesn't gate on baud being valid — it
  gates on `CTRL.TX_EN` only.
- **Silicon:** undefined / will not produce valid serial framing. AN385
  doesn't specify the failure mode; ARM's mbed-OS `serial_baud()` calls
  `error("serial_baud")` if the requested rate is below 16.

**This is a major QEMU-vs-silicon divergence.** The driver hardcodes
`BAUD_DIVISOR = 16` to remove this trap.

---

## 2. Polled-mode TX contract — what `CmsdkUart_PutChar` implements

### 2.1 Required register sequence

**Initialization (once, before any `PutChar`):**

1. Write `BAUDDIV ← (PCLK / desired_baud)`, ensuring value ≥ 16.
2. Write `CTRL ← TX_EN` (or `TX_EN | RX_EN` if RX is needed; slice 2 is
   TX-only, so the driver writes `TX_EN` alone).

The Zephyr driver does these in this order. From a clean reset (which
QEMU and silicon both honour: CTRL=0, STATE=0, BAUDDIV=0) the order
BAUDDIV-then-CTRL is sufficient.

**Per-character TX:**

1. Spin while `STATE & TXFULL`.
2. Write `DATA ← byte`.

That is the entire contract. No INTCLEAR needed. No overrun handling
needed in the happy path — a polled producer that always waits for
`!TXFULL` cannot produce TXOVERRUN.

The driver is `CmsdkUart_PutChar`:

```c
while (TransmitterIsBusy())   /* (STATE & TX_FULL_BIT) != 0 */
{
    Yield();                  /* memoryAccess->sleep(1) */
}
WriteDataRegister(c);         /* memoryAccess->write32(base+DATA, c) */
```

Both Zephyr `uart_cmsdk_apb_poll_out()` and mbed-OS implement the same
spin-then-write idiom.

### 2.2 What to poll before writing DATA

`STATE.TXFULL` (bit 0). It is set by the hardware when DATA is written
and cleared when the holding-register byte is shifted out (or, in QEMU,
when the chardev backend has accepted the byte). Reading STATE has no
side-effect, so a tight `while (STATE & TXFULL)` loop is side-effect-clean.

### 2.3 TX_OVRE — how it arises and how to clear it

`STATE.TXOVERRUN` is set when software writes DATA while `TXFULL=1`. The
**new byte overwrites `txbuf`** in QEMU — the previously-pending byte is
*kept in flight* (because `uart_transmit()` is already scheduled on it),
and the new one is what gets transmitted next when the backend drains.
So an overrun means "exactly one byte was lost"; it does not mean
"buffer corrupted." Silicon behaviour for the lost-byte semantics is not
explicitly documented in DDI 0479D — treat as implementation-specific and
assume "byte loss occurred, contents unspecified."

Clearing: write `STATE ← 0x4` (W1C bit 2). This clears `STATE.TXOVERRUN`
and causes the next update to clear `INTSTATUS.TXO`. Writing to INTSTATUS
with bit 2 set *also* clears STATE.TXOVERRUN.

**For the polled, single-producer driver in slice 2** — which always
checks `!TXFULL` before writing DATA — TXOVERRUN cannot arise. The
overrun handling matters only if multiple tasks share the UART without
a mutex (see §5) or if something writes DATA from an ISR concurrently
with task-level putchar.

### 2.4 May BAUDDIV be 0 / below 16?

**On QEMU:** Yes, in the sense that the guest will not crash and TX
will still appear on `-serial stdio`.

**On silicon:** No. BAUDDIV<16 produces no valid serial framing.

The driver hardcodes `BAUD_DIVISOR = 16` and has no API to set a
different value, removing this trap entirely.

### 2.5 Must CTRL.TX_EN be set before DATA writes?

**On QEMU:** No, but the byte will not reach the host. With TX_EN=0,
TXFULL stays set, the byte never drains, and the *next* DATA write
will see TXFULL=1 and set TXOVERRUN.

**On silicon:** Per DDI 0479D, writes to a disabled transmitter are
implementation-specific. The driver `Init`s `CTRL ← TX_EN` before
any subsequent `PutChar`, ensuring this is never a problem.

---

## 3. Polled-mode RX contract (slice 3 scope, documented for context)

Slice 2 is TX-only; `_read` returns EOF. Slice 3 will wire `_read` to a
UART RX helper so `Example/Common/ExampleInteractive.c` can drive
`send N` / `quit` over `qemu -serial stdio`. The contract for that
slice, written down here so the reader doesn't have to chase it:

### 3.1 Required sequence

**Initialization (once):**
1. BAUDDIV ≥ 16.
2. CTRL.RX_EN = 1.

**Per-character RX (non-blocking poll):**
1. If `STATE & RXFULL` is 0, return "no data".
2. Read `DATA` → byte.

Reading DATA atomically clears `STATE.RXFULL`. There is no software
clear operation needed for RXFULL.

### 3.2 RXFULL set/cleared

- **Set** by hardware when a complete byte is shifted in; requires
  `CTRL.RX_EN=1` (in QEMU, `uart_can_receive()` returns 0 unless RX_EN
  is set; characters are dropped if they arrive with RX_EN clear).
- **Cleared** by a read of DATA. The read-then-clear is atomic from
  software's point of view — a single LDR completes both.

### 3.3 RX_OVRE behaviour

Set when a new byte arrives while `RXFULL=1`; the **new byte overwrites
`rxbuf`** in QEMU. Clear with `STATE ← 0x8` or `INTSTATUS ← 0x8`.

**Important QEMU divergence:** In QEMU, `uart_can_receive()` returns 0
when RXFULL is set, so the chardev backend stops feeding bytes until
the guest reads DATA. RXOVERRUN is therefore hard to provoke under
stdio backpressure. On silicon there is no such backpressure; if the
consumer is too slow, RXOVERRUN will fire and bytes will be lost.

### 3.4 Read-then-clear pseudocode

```
poll_in():
    if (STATE & RXFULL) == 0:
        return EMPTY
    byte = DATA          # this read also clears RXFULL atomically
    return byte

read_byte_blocking():
    while (STATE & RXFULL) == 0:
        Yield()          # vTaskDelay(1) — same pattern as TX spin
    return DATA
```

---

## 4. Reference implementations (cross-checked during slice-2 review)

In rough order of usefulness for cross-checking the contract:

### 4.1 Zephyr `drivers/serial/uart_cmsdk_apb.c`

**Most directly comparable.** Linaro 2016 (modernised 2021), Apache 2.0.
Contains the canonical `while (uart->state & UART_TX_BF) {} uart->data = c`
poll-out idiom and an explicit comment about edge-triggered TX interrupts.
URL: `github.com/zephyrproject-rtos/zephyr/blob/main/drivers/serial/uart_cmsdk_apb.c`.

### 4.2 mbed-OS `targets/TARGET_ARM_FM/TARGET_FVP_MPS2/serial_api.c`

Arm's own (pre-acquisition) HAL driver. Apache 2.0. Confirms the
`BAUDDIV ≥ 16` minimum, the disable-before-reconfigure idiom, and
INTCLEAR W1C usage. URL: `github.com/ARMmbed/mbed-os/blob/master/targets/TARGET_ARM_FM/TARGET_FVP_MPS2/serial_api.c`.

### 4.3 QEMU device model — `hw/char/cmsdk-apb-uart.c`

Peter Maydell at Linaro, 2017, GPLv2. **The authoritative source for
"what QEMU actually does" — read this any time a test passes on QEMU
and you suspect silicon will behave differently.** Header comment
points to DDI 0479C as the spec.

URL: `github.com/qemu/qemu/blob/master/hw/char/cmsdk-apb-uart.c`.

### 4.4 FreeRTOS demo `CORTEX_MPU_M3_MPS2_QEMU_GCC` — *not useful*

The upstream FreeRTOS demo for this board uses **semihosting**
(`BKPT 0xAB`) rather than the CMSDK UART (per FreeRTOS forum thread
22969 — `getchar()` produces a hardfault). Won't help cross-check the
UART contract.

---

## 5. Concurrency under FreeRTOS preemption + FreeRTOS-Plus-TCP

### 5.1 Setup (slice 3+ context)

- Cortex-M3 single-core, FreeRTOS with preemption.
- One application task drives the SolidSyslog example.
- FreeRTOS-Plus-TCP runs its IP task at a high priority.
- A second application task (the BDD harness driver) reads from UART RX
  (`send N` / `quit`).

### 5.2 Mutex requirement

The CMSDK UART has a single holding register — no hardware FIFO. The
TX critical section is `spin while STATE.TXFULL; write DATA`. If two
tasks share the UART without a mutex, lines from the two tasks become
garbled at byte granularity, and TXOVERRUN can occur.

**During S08 bring-up** (slice 2, single producer task printing one
greeting) no mutex was needed.

**For slice 3+** (`Example/Common/` brings a Service thread + interactive
task) a FreeRTOS mutex (not a binary semaphore — we want priority
inheritance) should serialise the spin-and-write critical section.
The driver API will need to grow a mutex slot at that point.

### 5.3 Spin loop and tick rate

At 115200 8N1, one character is ~87 µs. With `configTICK_RATE_HZ = 100`
(10 ms tick), a single character's spin will not delay the next tick.
But for sustained log bursts at high priority, the cumulative spin
time can cause missed deadlines.

The slice-2 driver injects a **`sleep(milliseconds)`** hook called
inside the spin. Production wires this to `vTaskDelay(1)` (rounds up
to one tick = 10 ms with the current config) so the task blocks rather
than busy-spins. On QEMU the spin doesn't iterate at all (chardev
backend always drains synchronously), so this is silicon-only behaviour
and cost-free in the development environment.

The host fake counts `sleep()` calls (`CmsdkUartFake_SleepCallCount()`),
which lets the spin behaviour be unit-tested without threading.

### 5.4 What QEMU will not catch

| Trap                                       | QEMU              | Silicon           |
|--------------------------------------------|-------------------|-------------------|
| BAUDDIV=0 or <16                           | Output works      | No output         |
| Write DATA before TX_EN set                | Byte lost silently| Implementation-defined |
| Spin on TXFULL                             | Never iterates    | Iterates ~87 µs at 115200 |
| RXOVERRUN under fast-producer/slow-consumer| Hidden by chardev backpressure | Bytes lost |
| RX poll loop without yield                 | Doesn't burn CPU  | Burns CPU at 100% |

The first three are addressed by the slice-2 driver:
- `BAUDDIV ≥ 16` is a hardcoded constant.
- `CTRL.TX_EN` is set in `Init`, before any `PutChar`.
- `STATE.TXFULL` is polled before each DATA write.

The last two are slice-3+ concerns (RX path).

---

## References

- **Arm DDI 0479C/D** — *Cortex-M System Design Kit Technical Reference Manual*, §4.3 *APB UART*. Programmer's model, register layout, reset values.
- **Arm DAI 0385D** — *Application Note AN385: Cortex-M3 SMM on V2M-MPS2*, §3.7 *CMSDK APB subsystem*. UART base addresses on `mps2-an385`.
- **QEMU `hw/char/cmsdk-apb-uart.c`** — modelled semantics (W1C handling, edge-triggered TX, chardev backpressure that hides RX_OVRE under stdio, BAUDDIV<16 ignored).
- **QEMU `hw/arm/mps2.c`** — UART base address and IRQ assignment table for the AN385 board model.
- **Zephyr `drivers/serial/uart_cmsdk_apb.c`** — canonical poll-out idiom; the edge-triggered-TX comment.
- **mbed-OS `targets/TARGET_ARM_FM/TARGET_FVP_MPS2/serial_api.c`** — Arm HAL; corroborates BAUDDIV ≥ 16 minimum.
