#include "SolidSyslogMetaSd.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogStructuredDataDefinition.h"

#include <stdint.h>

struct SolidSyslogMetaSd
{
    struct SolidSyslogStructuredData base;
    struct SolidSyslogAtomicCounter* counter;
    SolidSyslogSysUpTimeFunction     getSysUpTime;
    SolidSyslogStringFunction        getLanguage;
};

static void Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);

static struct SolidSyslogMetaSd instance;

struct SolidSyslogStructuredData* SolidSyslogMetaSd_Create(const struct SolidSyslogMetaSdConfig* config)
{
    instance.base.Format  = Format;
    instance.counter      = config->counter;
    instance.getSysUpTime = config->getSysUpTime;
    instance.getLanguage  = config->getLanguage;
    return &instance.base;
}

void SolidSyslogMetaSd_Destroy(void)
{
    instance.base.Format  = NULL;
    instance.counter      = NULL;
    instance.getSysUpTime = NULL;
    instance.getLanguage  = NULL;
}

static const char SD_PREFIX[]      = "[meta sequenceId=\"";
static const char SYS_UP_TIME_SD[] = " sysUpTime=\"";
static const char LANGUAGE_SD[]    = " language=\"";

static void Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter)
{
    struct SolidSyslogMetaSd* meta = (struct SolidSyslogMetaSd*) self;
    uint_fast32_t             id   = SolidSyslogAtomicCounter_Increment(meta->counter);

    SolidSyslogFormatter_BoundedString(formatter, SD_PREFIX, sizeof(SD_PREFIX) - 1);
    SolidSyslogFormatter_Uint32(formatter, (uint32_t) id);
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    if (meta->getSysUpTime != NULL)
    {
        SolidSyslogFormatter_BoundedString(formatter, SYS_UP_TIME_SD, sizeof(SYS_UP_TIME_SD) - 1);
        SolidSyslogFormatter_Uint32(formatter, meta->getSysUpTime());
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
    if (meta->getLanguage != NULL)
    {
        SolidSyslogFormatter_BoundedString(formatter, LANGUAGE_SD, sizeof(LANGUAGE_SD) - 1);
        meta->getLanguage(formatter);
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
    SolidSyslogFormatter_AsciiCharacter(formatter, ']');
}
