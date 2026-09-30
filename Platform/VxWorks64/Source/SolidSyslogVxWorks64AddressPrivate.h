/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGVXWORKS64ADDRESSPRIVATE_H
#define SOLIDSYSLOGVXWORKS64ADDRESSPRIVATE_H

#include <stdint.h>

#include "vxWorks.h"

#include <netinet/in.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogVxWorks64AddressErrors.h"

struct SolidSyslogAddress;

struct SolidSyslogVxWorks64Address
{
    struct sockaddr_in Sockaddr;
};

void SolidSyslogVxWorks64Address_Initialise(struct SolidSyslogAddress* base);
void SolidSyslogVxWorks64Address_Cleanup(struct SolidSyslogAddress* base);

static inline void VxWorks64Address_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogAddressErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogVxWorks64AddressErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGVXWORKS64ADDRESSPRIVATE_H */
