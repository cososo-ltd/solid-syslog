/* A test stand-in for the CMSIS-RTOS2 API header.
 *
 * The real cmsis_os2.h is pure declarations, so the adapter under test needs no
 * upstream tree to compile - this file supplies the subset Platform/CmsisRtos
 * calls, spelled exactly as CMSIS-RTOS2 v2.3 spells it. It deliberately depends
 * on no SolidSyslog header: the real one does not, and a stand-in that did
 * would stop standing in.
 *
 * The subset is a drift risk, and the cross-compiled BDD target is what closes
 * it - there the adapter compiles against the real header.
 */
#ifndef CMSIS_OS2_H_
#define CMSIS_OS2_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* These are #defines because they are #defines upstream: a stand-in that spelt
 * them as enum constants would stop standing in, and osWaitForever is
 * 0xFFFFFFFFU, which is not an enum constant's type. The suppression lives in
 * the header rather than in a .clang-tidy because clang-tidy resolves its
 * config from the translation unit, and both the pack's sources and the tests
 * include this. */
/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define osWaitForever 0xFFFFFFFFU

#define osMutexRecursive 0x00000001U
#define osMutexPrioInherit 0x00000002U
#define osMutexRobust 0x00000008U

    /* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

    typedef enum
    {
        osOK = 0,
        osError = -1,
        osErrorTimeout = -2,
        osErrorResource = -3,
        osErrorParameter = -4,
        osErrorNoMemory = -5,
        osErrorISR = -6,
        osErrorSafetyClass = -7,
        osStatusReserved = 0x7FFFFFFF
    } osStatus_t;

    typedef void* osMutexId_t;

    typedef struct
    {
        const char* name;
        uint32_t attr_bits;
        void* cb_mem;
        uint32_t cb_size;
    } osMutexAttr_t;

    uint32_t osKernelGetTickCount(void);
    uint32_t osKernelGetTickFreq(void);

    osMutexId_t osMutexNew(const osMutexAttr_t* attr);
    osStatus_t osMutexAcquire(osMutexId_t mutex_id, uint32_t timeout);
    osStatus_t osMutexRelease(osMutexId_t mutex_id);
    osStatus_t osMutexDelete(osMutexId_t mutex_id);

#ifdef __cplusplus
}
#endif

#endif /* CMSIS_OS2_H_ */
