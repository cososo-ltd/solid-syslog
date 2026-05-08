#ifndef CMSDK_UART_FAKE_H
#define CMSDK_UART_FAKE_H

#include "CmsdkUart.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

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

#ifdef __cplusplus
}
#endif

#endif
