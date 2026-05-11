#ifndef CMSDK_UART_FAKE_H
#define CMSDK_UART_FAKE_H

#include "CmsdkUart.h"
#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

EXTERN_C_BEGIN

    /* Reset the fake's internal state. baseAddress must match the value passed
     * to CmsdkUart_Init so the fake can decode register offsets. */
    void CmsdkUartFake_Reset(uintptr_t baseAddress);

    /* CmsdkUartMemoryAccess wired to the fake's intercept functions. Pass to
     * CmsdkUart_Init in tests instead of the real MMIO access. */
    const CmsdkUartMemoryAccess* CmsdkUartFake_Access(void);

    uint32_t CmsdkUartFake_GetData(void);
    uint32_t CmsdkUartFake_GetCtrl(void);
    uint32_t CmsdkUartFake_GetBaudDiv(void);

    /* Number of STATE reads after a DATA write before the fake clears TX_FULL.
     * Default is 2. Set to 0 for "always ready" (TX_FULL never observed set).
     * Models the per-character drain delay the QEMU stdio backend hides. */
    void CmsdkUartFake_SetReadsBeforeTxReady(int reads);

    /* True when the driver wrote to DATA while STATE.TX_FULL was set — i.e.
     * the spin loop was missing or broken. Mirrors STATE.TX_OVRE on silicon. */
    bool CmsdkUartFake_TxOverrunOccurred(void);

    /* Number of times the driver called the sleep hook on the access struct.
     * Lets tests assert the spin loop yielded the CPU between status reads. */
    int CmsdkUartFake_SleepCallCount(void);

    /* Pre-arm a received byte: the next DATA read returns this value, with
     * STATE.RXFULL=1 reported on the preceding STATE read so the driver's
     * RX-ready check sees data available. */
    void CmsdkUartFake_SetReceivedByte(char byte);

    /* Number of STATE reads after SetReceivedByte before the fake asserts
     * STATE.RXFULL. Default is 0 — RXFULL set immediately. Use a positive
     * value to force the driver to spin on STATE before reading DATA. */
    void CmsdkUartFake_SetReadsBeforeRxReady(int reads);

EXTERN_C_END

#endif
