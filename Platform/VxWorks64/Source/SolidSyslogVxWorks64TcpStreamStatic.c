/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64TcpStream.h"

#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogVxWorks64TcpStreamPrivate.h"

static struct SolidSyslogVxWorks64TcpStream VxWorks64TcpStream_Instance;

struct SolidSyslogStream* SolidSyslogVxWorks64TcpStream_Create(const struct SolidSyslogVxWorks64TcpStreamConfig* config)
{
    SolidSyslogVxWorks64TcpStream_Initialise(&VxWorks64TcpStream_Instance.Base, config);
    return &VxWorks64TcpStream_Instance.Base;
}

void SolidSyslogVxWorks64TcpStream_Destroy(struct SolidSyslogStream* base)
{
    (void) base;
}
