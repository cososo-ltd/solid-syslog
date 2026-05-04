#include "SolidSyslogTimeQualitySd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogFormatter;

struct SolidSyslogTimeQualitySd
{
    struct SolidSyslogStructuredData base;
    SolidSyslogTimeQualityFunction   getTimeQuality;
};

static void        Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);
static inline void FormatBoolParam(struct SolidSyslogFormatter* formatter, const char* name, size_t nameLength, bool value);
static inline void FormatSyncAccuracy(struct SolidSyslogFormatter* formatter, uint32_t value);

static struct SolidSyslogTimeQualitySd instance;

struct SolidSyslogStructuredData* SolidSyslogTimeQualitySd_Create(SolidSyslogTimeQualityFunction getTimeQuality)
{
    instance.base.Format    = Format;
    instance.getTimeQuality = getTimeQuality;
    return &instance.base;
}

void SolidSyslogTimeQualitySd_Destroy(void)
{
    instance.base.Format    = NULL;
    instance.getTimeQuality = NULL;
}

static const char SD_PREFIX[]           = "[timeQuality";
static const char PARAM_TZ_KNOWN[]      = " tzKnown";
static const char PARAM_IS_SYNCED[]     = " isSynced";
static const char PARAM_SYNC_ACCURACY[] = " syncAccuracy=\"";

static void Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter)
{
    struct SolidSyslogTimeQualitySd* tq = (struct SolidSyslogTimeQualitySd*) self;
    struct SolidSyslogTimeQuality    q  = {0};

    tq->getTimeQuality(&q);

    SolidSyslogFormatter_BoundedString(formatter, SD_PREFIX, sizeof(SD_PREFIX) - 1);
    FormatBoolParam(formatter, PARAM_TZ_KNOWN, sizeof(PARAM_TZ_KNOWN) - 1, q.tzKnown);
    FormatBoolParam(formatter, PARAM_IS_SYNCED, sizeof(PARAM_IS_SYNCED) - 1, q.isSynced);
    FormatSyncAccuracy(formatter, q.syncAccuracyMicroseconds);
    SolidSyslogFormatter_AsciiCharacter(formatter, ']');
}

static inline void FormatBoolParam(struct SolidSyslogFormatter* formatter, const char* name, size_t nameLength, bool value)
{
    SolidSyslogFormatter_BoundedString(formatter, name, nameLength);
    SolidSyslogFormatter_AsciiCharacter(formatter, '=');
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    SolidSyslogFormatter_AsciiCharacter(formatter, value ? '1' : '0');
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
}

static inline void FormatSyncAccuracy(struct SolidSyslogFormatter* formatter, uint32_t value)
{
    if (value != SOLIDSYSLOG_SYNC_ACCURACY_OMIT)
    {
        SolidSyslogFormatter_BoundedString(formatter, PARAM_SYNC_ACCURACY, sizeof(PARAM_SYNC_ACCURACY) - 1);
        SolidSyslogFormatter_Uint32(formatter, value);
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}
