#include "SolidSyslogMetaSd.h"

#include <stddef.h>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogFormatter;

struct SolidSyslogMetaSd
{
    struct SolidSyslogStructuredData base;
    struct SolidSyslogAtomicCounter* counter;
    SolidSyslogSysUpTimeFunction     getSysUpTime;
    SolidSyslogStringFunction        getLanguage;
};

static void        Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);
static inline void EmitSequenceId(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter);
static inline void EmitSysUpTime(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter);
static inline void EmitLanguage(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter);

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

static const char SD_PREFIX[]      = "[meta";
static const char SEQUENCE_ID_SD[] = " sequenceId=\"";
static const char SYS_UP_TIME_SD[] = " sysUpTime=\"";
static const char LANGUAGE_SD[]    = " language=\"";

static void Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter)
{
    struct SolidSyslogMetaSd* meta = (struct SolidSyslogMetaSd*) self;

    SolidSyslogFormatter_BoundedString(formatter, SD_PREFIX, sizeof(SD_PREFIX) - 1);
    EmitSequenceId(meta, formatter);
    EmitSysUpTime(meta, formatter);
    EmitLanguage(meta, formatter);
    SolidSyslogFormatter_AsciiCharacter(formatter, ']');
}

static inline void EmitSequenceId(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter)
{
    if (meta->counter != NULL)
    {
        SolidSyslogFormatter_BoundedString(formatter, SEQUENCE_ID_SD, sizeof(SEQUENCE_ID_SD) - 1);
        SolidSyslogFormatter_Uint32(formatter, SolidSyslogAtomicCounter_Increment(meta->counter));
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}

static inline void EmitSysUpTime(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter)
{
    if (meta->getSysUpTime != NULL)
    {
        SolidSyslogFormatter_BoundedString(formatter, SYS_UP_TIME_SD, sizeof(SYS_UP_TIME_SD) - 1);
        SolidSyslogFormatter_Uint32(formatter, meta->getSysUpTime());
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}

static inline void EmitLanguage(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter)
{
    if (meta->getLanguage != NULL)
    {
        SolidSyslogFormatter_BoundedString(formatter, LANGUAGE_SD, sizeof(LANGUAGE_SD) - 1);
        meta->getLanguage(formatter);
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}
