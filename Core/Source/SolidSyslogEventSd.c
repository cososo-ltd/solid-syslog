/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogEventSd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStructuredDataDefinition.h"

static void EventSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element);
static inline const struct SolidSyslogEventSd* EventSd_SelfFromBase(struct SolidSyslogStructuredData* base);
static inline bool EventSd_HasSourceName(const struct SolidSyslogErrorEvent* event);
static inline void EventSd_Detail(struct SolidSyslogSdValue* value, const struct SolidSyslogErrorEvent* event);
static inline void EventSd_Int32(struct SolidSyslogSdValue* value, int32_t number);

struct SolidSyslogStructuredData* SolidSyslogEventSd_Initialise(
    struct SolidSyslogEventSd* self,
    const struct SolidSyslogErrorEvent* event
)
{
    self->Base.Format = EventSd_Format;
    self->Event = event;
    return &self->Base;
}

static void EventSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    /* The element's SD-ID is solidsyslog@67478: a private name under the IANA
     * private enterprise number assigned to Cozens Software Solutions Limited. */
    static const char name[] = "solidsyslog";
    const uint32_t enterpriseNumber = 67478U;
    const struct SolidSyslogErrorEvent* event = EventSd_SelfFromBase(base)->Event;

    SolidSyslogSdElement_Begin(element, name, enterpriseNumber);
    if (EventSd_HasSourceName(event))
    {
        SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "source"), event->Source->Name);
    }
    SolidSyslogSdValue_String(
        SolidSyslogSdElement_Param(element, "category"),
        SolidSyslogErrorCategory_Name(event->Category)
    );
    EventSd_Detail(SolidSyslogSdElement_Param(element, "detail"), event);
    if (event->HasNativeCode)
    {
        EventSd_Int32(SolidSyslogSdElement_Param(element, "native"), event->NativeCode);
    }
    SolidSyslogSdElement_End(element);
}

static inline const struct SolidSyslogEventSd* EventSd_SelfFromBase(struct SolidSyslogStructuredData* base)
{
    return (const struct SolidSyslogEventSd*) base;
}

/* An integrator's own SolidSyslog_Error call may carry no source, or one
 * with no name. */
static inline bool EventSd_HasSourceName(const struct SolidSyslogErrorEvent* event)
{
    return (event->Source != NULL) && (event->Source->Name != NULL);
}

static inline void EventSd_Detail(struct SolidSyslogSdValue* value, const struct SolidSyslogErrorEvent* event)
{
    const char* name = SolidSyslogErrorEvent_DetailName(event);

    if (name != NULL)
    {
        SolidSyslogSdValue_String(value, name);
    }
    else
    {
        EventSd_Int32(value, event->Detail);
    }
}

/* SdValue writes unsigned numbers only, so a negative one is its sign and
 * then its magnitude, taken in unsigned arithmetic so that INT32_MIN has one. */
static inline void EventSd_Int32(struct SolidSyslogSdValue* value, int32_t number)
{
    uint32_t magnitude = (uint32_t) number;

    if (number < 0)
    {
        SolidSyslogSdValue_String(value, "-");
        magnitude = 0U - magnitude;
    }

    SolidSyslogSdValue_Uint32(value, magnitude);
}
