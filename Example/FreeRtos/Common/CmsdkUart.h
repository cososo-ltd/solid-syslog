#ifndef CMSDK_UART_H
#define CMSDK_UART_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef uint32_t (*CmsdkUartRead32Function)(uintptr_t address);
    typedef void (*CmsdkUartWrite32Function)(uintptr_t address, uint32_t value);
    typedef void (*CmsdkUartSleepFunction)(int milliseconds);

    typedef struct
    {
        CmsdkUartRead32Function  read32;
        CmsdkUartWrite32Function write32;
        CmsdkUartSleepFunction   sleep;
    } CmsdkUartMemoryAccess;

    void CmsdkUart_Init(const CmsdkUartMemoryAccess* access, uintptr_t baseAddress);
    void CmsdkUart_PutChar(char c);
    void CmsdkUart_Write(const char* buffer, size_t length);
    char CmsdkUart_GetChar(void);

#ifdef __cplusplus
}
#endif

#endif
