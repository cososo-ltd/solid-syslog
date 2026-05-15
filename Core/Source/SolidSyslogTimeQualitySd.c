#include "SolidSyslogTimeQualitySd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogFormatter;

struct SolidSyslogTimeQualitySd
{
    struct SolidSyslogStructuredData Base;
    SolidSyslogTimeQualityFunction GetTimeQuality;
};

static void TimeQualitySd_Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);
static inline void TimeQualitySd_FormatBoolParam(
    struct SolidSyslogFormatter* formatter,
    const char* name,
    size_t nameLength,
    bool value
);
static inline void TimeQualitySd_FormatSyncAccuracy(struct SolidSyslogFormatter* formatter, uint32_t value);

static struct SolidSyslogTimeQualitySd instance;

struct SolidSyslogStructuredData* SolidSyslogTimeQualitySd_Create(SolidSyslogTimeQualityFunction getTimeQuality)
{
    instance.Base.Format = TimeQualitySd_Format;
    instance.GetTimeQuality = getTimeQuality;
    return &instance.Base;
}

void SolidSyslogTimeQualitySd_Destroy(void)
{
    instance.Base.Format = NULL;
    instance.GetTimeQuality = NULL;
}

static const char SD_PREFIX[] = "[timeQuality";
static const char PARAM_TZ_KNOWN[] = " tzKnown";
static const char PARAM_IS_SYNCED[] = " isSynced";
static const char PARAM_SYNC_ACCURACY[] = " syncAccuracy=\"";

static void TimeQualitySd_Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter)
{
    struct SolidSyslogTimeQualitySd* tq = (struct SolidSyslogTimeQualitySd*) self;
    struct SolidSyslogTimeQuality q = {0};

    tq->GetTimeQuality(&q);

    SolidSyslogFormatter_BoundedString(formatter, SD_PREFIX, sizeof(SD_PREFIX) - 1);
    TimeQualitySd_FormatBoolParam(formatter, PARAM_TZ_KNOWN, sizeof(PARAM_TZ_KNOWN) - 1, q.TzKnown);
    TimeQualitySd_FormatBoolParam(formatter, PARAM_IS_SYNCED, sizeof(PARAM_IS_SYNCED) - 1, q.IsSynced);
    TimeQualitySd_FormatSyncAccuracy(formatter, q.SyncAccuracyMicroseconds);
    SolidSyslogFormatter_AsciiCharacter(formatter, ']');
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
        SolidSyslogFormatter_BoundedString(formatter, PARAM_SYNC_ACCURACY, sizeof(PARAM_SYNC_ACCURACY) - 1);
        SolidSyslogFormatter_Uint32(formatter, value);
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}
