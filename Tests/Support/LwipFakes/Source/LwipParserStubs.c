/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* Link-time stand-ins for the stack entry points upstream lwIP's address
 * parsers reference but never reach from a parse. ip.c supplies the
 * dual-stack ipaddr_aton alongside ip_input, which dispatches to the two
 * families' input paths; ip6addr_aton looks a zone's interface up by name.
 * No test parses a packet or names a zone, so each stub answers as if the
 * stack were empty. */

#include <stddef.h>

#include "lwip/err.h"
#include "lwip/ip4.h"
#include "lwip/ip6.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"

err_t ip4_input(struct pbuf* p, struct netif* inp)
{
    (void) p;
    (void) inp;
    return ERR_IF;
}

err_t ip6_input(struct pbuf* p, struct netif* inp)
{
    (void) p;
    (void) inp;
    return ERR_IF;
}

struct netif* netif_find(const char* name)
{
    (void) name;
    return NULL;
}
