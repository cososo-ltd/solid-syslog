/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Datagram.h"

#include <stdbool.h>

#include "vxWorks.h"

#include <sys/socket.h>

#include "sockLib.h"

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogVxWorks64DatagramErrors.h"
#include "SolidSyslogVxWorks64DatagramPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogVxWorks64DatagramErrorSource = {"VxWorks64Datagram"};

static bool VxWorks64Datagram_Open(struct SolidSyslogDatagram* base);

static inline struct SolidSyslogVxWorks64Datagram* VxWorks64Datagram_SelfFromBase(struct SolidSyslogDatagram* base);

void SolidSyslogVxWorks64Datagram_Initialise(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogVxWorks64Datagram* self = VxWorks64Datagram_SelfFromBase(base);
    self->Base.Open = VxWorks64Datagram_Open;
}

static inline struct SolidSyslogVxWorks64Datagram* VxWorks64Datagram_SelfFromBase(struct SolidSyslogDatagram* base)
{
    return (struct SolidSyslogVxWorks64Datagram*) base;
}

void SolidSyslogVxWorks64Datagram_Cleanup(struct SolidSyslogDatagram* base)
{
    (void) base;
}

static bool VxWorks64Datagram_Open(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogVxWorks64Datagram* self = VxWorks64Datagram_SelfFromBase(base);
    self->Fd = socket(0, 0, 0);
    return true;
}
