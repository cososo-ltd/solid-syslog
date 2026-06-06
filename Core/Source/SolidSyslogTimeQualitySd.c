#include "SolidSyslogTimeQualitySd.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStructuredDataDefinition.h"
#include "SolidSyslogTimeQualitySdErrors.h"
#include "SolidSyslogTimeQualitySdPrivate.h"

const struct SolidSyslogErrorSource TimeQualitySdErrorSource = {"TimeQualitySd"};

static void TimeQualitySd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element);

static inline struct SolidSyslogTimeQualitySd* TimeQualitySd_SelfFromBase(struct SolidSyslogStructuredData* base);

void TimeQualitySd_Initialise(struct SolidSyslogStructuredData* base, SolidSyslogTimeQualityFunction getTimeQuality)
{
    struct SolidSyslogTimeQualitySd* self = TimeQualitySd_SelfFromBase(base);
    self->Base.Format = TimeQualitySd_Format;
    self->GetTimeQuality = getTimeQuality;
}

void TimeQualitySd_Cleanup(struct SolidSyslogStructuredData* base)
{
    /* Overwrite the abstract base with the shared NullSd vtable so use-after-destroy
     * is a safe no-op. Derived fields are private to this TU; the next _Initialise
     * overwrites them. */
    *base = *SolidSyslogNullSd_Get();
}

static void TimeQualitySd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    struct SolidSyslogTimeQualitySd* self = TimeQualitySd_SelfFromBase(base);
    struct SolidSyslogTimeQuality q = {0};

    self->GetTimeQuality(&q);

    SolidSyslogSdElement_Begin(element, "timeQuality", 0U);
    SolidSyslogSdValue_Uint32(SolidSyslogSdElement_Param(element, "tzKnown"), q.TzKnown ? 1U : 0U);
    SolidSyslogSdValue_Uint32(SolidSyslogSdElement_Param(element, "isSynced"), q.IsSynced ? 1U : 0U);
    if (q.SyncAccuracyMicroseconds != SOLIDSYSLOG_SYNC_ACCURACY_OMIT)
    {
        SolidSyslogSdValue_Uint32(SolidSyslogSdElement_Param(element, "syncAccuracy"), q.SyncAccuracyMicroseconds);
    }
    SolidSyslogSdElement_End(element);
}

static inline struct SolidSyslogTimeQualitySd* TimeQualitySd_SelfFromBase(struct SolidSyslogStructuredData* base)
{
    return (struct SolidSyslogTimeQualitySd*) base;
}
