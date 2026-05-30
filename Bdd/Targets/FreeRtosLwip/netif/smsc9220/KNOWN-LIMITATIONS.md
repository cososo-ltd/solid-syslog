# Vendored SMSC9220 / LAN9118 driver — provenance and known limitations

`smsc9220_eth_drv.{c,h}` and `smsc9220_emac_config.h` are the Arm low-level
LAN9118 (SMSC9220) Ethernet driver, vendored from FreeRTOS-Plus-TCP's
`source/portable/NetworkInterface/MPS2_AN385/ether_lan9118/` (Apache-2.0;
copyright and license headers preserved). They are kept in this `smsc9220/`
subdirectory with a `DisableFormat` `.clang-format` so `analyze-format` leaves
the third-party source untouched.

## Provenance / snapshot

Our snapshot predates upstream commit
[`861750b9` (2025-05-22), "smsc9220_eth_drv.c; fix warnings from gcc
-Wconversion" (#1245)](https://github.com/FreeRTOS/FreeRTOS-Plus-TCP/pull/1245).
Relative to current upstream `main` we are missing only that commit's narrowing
casts (the `(char)` casts on the MAC bytes near line 958 and the `(uint32_t) -1`
sentinel near line 1226). We do not need them because this translation unit is
compiled with `-Wno-conversion -Wno-sign-conversion` (see the target
`CMakeLists.txt`). The driver is therefore **not** byte-for-byte identical to
upstream `main`; it is a known, slightly stale verbatim snapshot.

## Known defects (real silicon only — benign on QEMU)

Three defects exist in this driver, **unchanged in upstream `main`** as of
2026-05. They are real bugs against actual LAN9118 silicon but **cannot fire on
QEMU's MPS2 LAN9118 model**, which is deterministic and never enters the
divergent code path — which is why the S28.09/S28.10/S28.11 oracle lanes are
green. We run only under QEMU and have no real-hardware test rig, so these are
documented rather than patched (see "Why not patched" below).

### 1. (Critical) MAC CSR busy-bit poll in `smsc9220_mac_regwrite`

The busy-wait loop condition (≈line 467) is:

```c
} while( time_out &&
         ( register_map->mac_csr_cmd &
           GET_BIT( register_map->mac_csr_cmd, MAC_CSR_CMD_BUSY_INDEX ) ) );
```

`GET_BIT` already returns `0`/`1`, so this ANDs the whole register with bit 0
(the LSB of the MAC register address) instead of testing the BUSY bit. The
write can return before the operation completes and race the next MAC/PHY
access. The sibling `smsc9220_mac_regread` (≈line 416) shows the correct form:
condition on `GET_BIT( ... )` alone.

**Correct fix:** condition on `GET_BIT( register_map->mac_csr_cmd,
MAC_CSR_CMD_BUSY_INDEX )` alone, matching `smsc9220_mac_regread`.

**Benign on QEMU because:** QEMU processes a MAC-CSR command synchronously
inside the MMIO write handler, so the readback never shows BUSY set. The early
exit lands on an already-completed operation and reads correct data.

### 2. (Major) TX filler DWORD on word-aligned chunks

`fill_tx_fifo` (≈line 320) computes:

```c
uint32_t remainder_bytes = ( size_bytes % 4 );
uint32_t filler_bytes = ( 4 - remainder_bytes );
```

For a word-aligned chunk (`remainder_bytes == 0`) this is `4` — a full zero
DWORD is prepended and `smsc9220_send_by_chunks` sets `data_start_offset = 4`
(≈line 1150, same `4 - (current_size % 4)` shape). The emitted packet is still
correct (the offset tells the hardware to skip the filler), but a whole FIFO
DWORD is wasted, and the FIFO free-space check (≈line 1126) does not account for
it, so a chunk whose footprint is within 4 bytes of TX-FIFO capacity could
overrun.

**Correct fix:** use `(4 - remainder_bytes) % 4` at **both** `fill_tx_fifo`
(filler bytes) and `smsc9220_send_by_chunks` (`data_start_offset_bytes`) so the
two stay consistent — word-aligned chunks then prepend zero filler and set
offset 0.

**Benign on QEMU because:** QEMU honours `data_start_offset`, so the packet is
byte-correct; the off-by-4 free-space error is per-packet and size-deterministic
(never accumulates — the FIFO drains between packets), and syslog records are
far smaller than the TX FIFO, so the overrun condition is never reached.

### 3. (Major) `smsc9220_check_id` return type

`smsc9220_check_id` (≈line 855) is declared returning `int` and returns `1` on a
chip-ID mismatch:

```c
return( ( GET_BIT_FIELD( id, CHIP_ID_MASK, CHIP_ID_POS ) == CHIP_ID ) ? 0 : 1 );
```

Its only caller stores the result into an `enum smsc9220_error_t` (≈line 996),
where `1 == SMSC9220_ERROR_TIMEOUT` — so a genuine ID mismatch is misreported as
a timeout. Initialisation still aborts (non-zero), so this is a misleading
diagnostic rather than a functional failure.

**Correct fix:** change the signature to `enum smsc9220_error_t` and return
`SMSC9220_ERROR_NONE` / `SMSC9220_ERROR_INTERNAL`.

**Benign on QEMU because:** the model's chip ID always matches, so the mismatch
path is never taken.

## Why not patched

We run this driver only under QEMU's deterministic LAN9118 model, which cannot
exercise any of the three divergences (see each "Benign on QEMU" note). A fix
would therefore be unverifiable on the only axis that matters — real silicon —
and an untested change to hardware-facing code is a liability, not an
improvement, especially as it would *look* correct. This mirrors the project's
"no untested production code" discipline applied to vendored code.

**Do not "fix" these without a real LAN9118 hardware test rig.** QEMU cannot
show the difference; a plausible-looking change could regress real silicon with
no signal here.

## Upstream

All three are present and unchanged in upstream FreeRTOS-Plus-TCP `main`, and a
search (2026-05) found no public report, fix, fork, CVE, or erratum. They have
been raised with the upstream community via the FreeRTOS forum (Libraries
category) as a single good-citizen report; the driver is Arm-origin, so a fix
may land either in the FreeRTOS-Plus-TCP copy or at the Arm source. If upstream
lands hardware-validated fixes, prefer re-syncing to the upstream commit over
carrying local patches.

Tracked locally as story S28.12 (#479), closed as documented known-limitations.
