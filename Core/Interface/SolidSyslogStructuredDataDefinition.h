/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The StructuredData vtable (Format) - the SD-source contract an implementor
 *  fills in (the StructuredData extension point). */
#ifndef SOLIDSYSLOGSTRUCTUREDDATADEFINITION_H
#define SOLIDSYSLOGSTRUCTUREDDATADEFINITION_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogSdElement;

    /** A structured-data source. Implementors embed this as the first member of
     *  their struct so Format can downcast @c base back to the instance.
     *
     *  Format runs inside SolidSyslog_Log, on the application's thread, so an
     *  implementation that blocks stalls the caller's logging. Writing nothing
     *  is a valid answer - a message where no source writes emits NILVALUE - so
     *  a source with nothing to report need not invent a value. */
    struct SolidSyslogStructuredData
    {
        /** Write this source's SD-ELEMENT(s) into @p element. @p base is this
         *  same object, carrying any per-instance state. The library never
         *  allocates the object, so it lives in the implementor's storage. */
        void (*Format)(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTRUCTUREDDATADEFINITION_H */
