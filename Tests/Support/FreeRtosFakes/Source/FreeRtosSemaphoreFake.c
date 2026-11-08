#include "FreeRtosSemaphoreFake.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

static unsigned createMutexStaticCallCount = 0U;
static unsigned semaphoreTakeCallCount = 0U;
static unsigned semaphoreGiveCallCount = 0U;
static unsigned semaphoreDeleteCallCount = 0U;
static bool createMutexStaticFails = false;

void FreeRtosSemaphoreFake_Reset(void)
{
    createMutexStaticCallCount = 0;
    semaphoreTakeCallCount = 0;
    semaphoreGiveCallCount = 0;
    semaphoreDeleteCallCount = 0;
    createMutexStaticFails = false;
}

void FreeRtosSemaphoreFake_SetCreateMutexStaticFails(bool fails)
{
    createMutexStaticFails = fails;
}

unsigned FreeRtosSemaphoreFake_CreateMutexStaticCallCount(void)
{
    return createMutexStaticCallCount;
}

unsigned FreeRtosSemaphoreFake_SemaphoreTakeCallCount(void)
{
    return semaphoreTakeCallCount;
}

unsigned FreeRtosSemaphoreFake_SemaphoreGiveCallCount(void)
{
    return semaphoreGiveCallCount;
}

unsigned FreeRtosSemaphoreFake_SemaphoreDeleteCallCount(void)
{
    return semaphoreDeleteCallCount;
}

SemaphoreHandle_t xQueueCreateMutexStatic(const uint8_t ucQueueType, StaticQueue_t* pxStaticQueue)
{
    (void) ucQueueType;
    ++createMutexStaticCallCount;
    return createMutexStaticFails ? NULL : (SemaphoreHandle_t) pxStaticQueue;
}

BaseType_t xQueueSemaphoreTake(QueueHandle_t xQueue, TickType_t xTicksToWait)
{
    (void) xQueue;
    (void) xTicksToWait;
    ++semaphoreTakeCallCount;
    return pdTRUE;
}

BaseType_t xQueueGenericSend(
    QueueHandle_t xQueue,
    const void* const pvItemToQueue,
    TickType_t xTicksToWait,
    const BaseType_t xCopyPosition
)
{
    (void) xQueue;
    (void) pvItemToQueue;
    (void) xTicksToWait;
    (void) xCopyPosition;
    ++semaphoreGiveCallCount;
    return pdTRUE;
}

void vQueueDelete(QueueHandle_t xQueue)
{
    (void) xQueue;
    ++semaphoreDeleteCallCount;
}
