#ifndef LWIPPBUFFAKE_H
#define LWIPPBUFFAKE_H

#include "ExternC.h"

#include <stdbool.h>
#include <stdint.h>

#include "lwip/pbuf.h"

EXTERN_C_BEGIN

    void LwipPbufFake_Reset(void);

    void LwipPbufFake_SetPbufAllocFails(bool fails);

    unsigned LwipPbufFake_PbufAllocCallCount(void);
    pbuf_layer LwipPbufFake_LastAllocLayer(void);
    uint16_t LwipPbufFake_LastAllocLength(void);
    pbuf_type LwipPbufFake_LastAllocType(void);
    struct pbuf* LwipPbufFake_LastAllocReturned(void);

    unsigned LwipPbufFake_PbufFreeCallCount(void);

    /* Allocated-but-not-yet-freed pbuf count. Successful pbuf_alloc bumps it;
     * pbuf_free decrements. A test that ends with a non-zero value has
     * leaked a pbuf — pin this in teardown to catch leaks across the suite. */
    int LwipPbufFake_OutstandingPbufCount(void);

    /* RX-side helper: pbufs handed to the wrapper via the tcp_recv callback
     * come from lwIP's machinery, not our pbuf_alloc — but the wrapper still
     * pbuf_free's them when fully drained. Tests fabricate stack pbufs and
     * call this to balance the outstanding count so the leak invariant
     * stays honest. */
    void LwipPbufFake_NoteIncomingPbuf(void);

EXTERN_C_END

#endif /* LWIPPBUFFAKE_H */
