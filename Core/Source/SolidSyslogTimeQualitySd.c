#include "SolidSyslogTimeQualitySd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogStructuredDataDefinition.h"
#include "SolidSyslogTimeQualitySdPrivate.h"

struct SolidSyslogFormatter;

static void TimeQualitySd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter);

static inline struct SolidSyslogTimeQualitySd* TimeQualitySd_SelfFromBase(struct SolidSyslogStructuredData* base);
static inline void TimeQualitySd_FormatBoolParam(
    struct SolidSyslogFormatter* formatter,
    const char* name,
    size_t nameLength,
    bool value
);
static inline void TimeQualitySd_FormatSyncAccuracy(struct SolidSyslogFormatter* formatter, uint32_t value);

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

static void TimeQualitySd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter)
{
    static const char sdPrefix[] = "[timeQuality";
    static const char paramTzKnown[] = " tzKnown";
    static const char paramIsSynced[] = " isSynced";
    struct SolidSyslogTimeQualitySd* self = TimeQualitySd_SelfFromBase(base);
    struct SolidSyslogTimeQuality q = {0};

    self->GetTimeQuality(&q);

    SolidSyslogFormatter_BoundedString(formatter, sdPrefix, sizeof(sdPrefix) - 1U);
    TimeQualitySd_FormatBoolParam(formatter, paramTzKnown, sizeof(paramTzKnown) - 1U, q.TzKnown);
    TimeQualitySd_FormatBoolParam(formatter, paramIsSynced, sizeof(paramIsSynced) - 1U, q.IsSynced);
    TimeQualitySd_FormatSyncAccuracy(formatter, q.SyncAccuracyMicroseconds);
    SolidSyslogFormatter_AsciiCharacter(formatter, ']');
}

static inline struct SolidSyslogTimeQualitySd* TimeQualitySd_SelfFromBase(struct SolidSyslogStructuredData* base)
{
    return (struct SolidSyslogTimeQualitySd*) base;
}

static inline void TimeQualitySd_FormatBoolParam(
    struct SolidSyslogFormatter* formatter,
    const char* name,
    size_t nameLength,
    bool value
)
{
    SolidSyslogFormatter_BoundedString(formatter, name, nameLength);
    SolidSyslogFormatter_AsciiCharacter(formatter, '=');
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    SolidSyslogFormatter_AsciiCharacter(formatter, value ? '1' : '0');
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
}

static inline void TimeQualitySd_FormatSyncAccuracy(struct SolidSyslogFormatter* formatter, uint32_t value)
{
    if (value != SOLIDSYSLOG_SYNC_ACCURACY_OMIT)
    {
        static const char paramSyncAccuracy[] = " syncAccuracy=\"";
        SolidSyslogFormatter_BoundedString(formatter, paramSyncAccuracy, sizeof(paramSyncAccuracy) - 1U);
        SolidSyslogFormatter_Uint32(formatter, value);
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}
