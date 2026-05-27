#ifndef LWIPPBUFFAKE_H
#define LWIPPBUFFAKE_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

#include "lwip/pbuf.h"

EXTERN_C_BEGIN

    void LwipPbufFake_Reset(void);

    /* pbuf_alloc configuration */
    void LwipPbufFake_SetPbufAllocFails(bool fails);

    /* pbuf_alloc spy */
    unsigned LwipPbufFake_PbufAllocCallCount(void);
    pbuf_layer LwipPbufFake_LastAllocLayer(void);
    uint16_t LwipPbufFake_LastAllocLength(void);
    pbuf_type LwipPbufFake_LastAllocType(void);
    struct pbuf* LwipPbufFake_LastAllocReturned(void);

    /* pbuf_free spy */
    unsigned LwipPbufFake_PbufFreeCallCount(void);

    /* Allocated-but-not-yet-freed pbuf count. Successful pbuf_alloc bumps it;
     * pbuf_free decrements. A test that ends with a non-zero value has
     * leaked a pbuf — pin this in teardown to catch leaks across the suite. */
    int LwipPbufFake_OutstandingPbufCount(void);

EXTERN_C_END

#endif /* LWIPPBUFFAKE_H */
