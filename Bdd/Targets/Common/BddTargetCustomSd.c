#include "BddTargetCustomSd.h"

#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStructuredDataDefinition.h"

/* IANA-reserved "example" Private Enterprise Number — safe for documentation. */
enum
{
    EXAMPLE_ENTERPRISE_NUMBER = 32473U
};

static void CustomSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    (void) base; /* Stateless: this example emits a fixed element. */

    SolidSyslogSdElement_Begin(element, "example", EXAMPLE_ENTERPRISE_NUMBER);
    SolidSyslogSdValue_String(SolidSyslogSdElement_Param(element, "detail"), "Hello World");
    SolidSyslogSdElement_End(element);
}

static struct SolidSyslogStructuredData customSd = {CustomSd_Format};

struct SolidSyslogStructuredData* BddTargetCustomSd_Get(void)
{
    return &customSd;
}
