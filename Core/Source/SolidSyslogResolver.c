/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogTransport.h"

struct SolidSyslogAddress;

bool SolidSyslogResolver_Resolve(
    struct SolidSyslogResolver* resolver,
    enum SolidSyslogTransport transport,
    const char* host,
    uint16_t port,
    struct SolidSyslogAddress* result
)
{
    return resolver->Resolve(resolver, transport, host, port, result);
}
