#include "SolidSyslogMetaSd.h"

#include <stddef.h>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogFormatter;

struct SolidSyslogMetaSd
{
    struct SolidSyslogStructuredData base;
    struct SolidSyslogAtomicCounter* counter;
    SolidSyslogSysUpTimeFunction getSysUpTime;
    SolidSyslogStringFunction getLanguage;
};

static void MetaSd_Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);
static void MetaSd_NilMetaSdFormat(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);
static inline void MetaSd_EmitSequenceId(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter);
static inline void MetaSd_EmitSysUpTime(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter);
static inline void MetaSd_EmitLanguage(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter);

static struct SolidSyslogMetaSd instance;
static struct SolidSyslogStructuredData NilMetaSd = {.Format = MetaSd_NilMetaSdFormat};

struct SolidSyslogStructuredData* SolidSyslogMetaSd_Create(const struct SolidSyslogMetaSdConfig* config)
{
    struct SolidSyslogStructuredData* result = &NilMetaSd;
    if (config == NULL)
    {
        SolidSyslog_Error(SolidSyslogSeverity_Warning, SOLIDSYSLOG_ERROR_MSG_METASD_CREATE_NULL_CONFIG);
    }
    else if (config->counter == NULL)
    {
        SolidSyslog_Error(SolidSyslogSeverity_Warning, SOLIDSYSLOG_ERROR_MSG_METASD_CREATE_NULL_COUNTER);
    }
    else
    {
        instance.base.Format = MetaSd_Format;
        instance.counter = config->counter;
        instance.getSysUpTime = config->getSysUpTime;
        instance.getLanguage = config->getLanguage;
        result = &instance.base;
    }
    return result;
}

void SolidSyslogMetaSd_Destroy(void)
{
    instance.base.Format = NULL;
    instance.counter = NULL;
    instance.getSysUpTime = NULL;
    instance.getLanguage = NULL;
}

static void MetaSd_NilMetaSdFormat(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter)
{
    (void) self;
    (void) formatter;
}

static const char SD_PREFIX[] = "[meta";
static const char SEQUENCE_ID_SD[] = " sequenceId=\"";
static const char SYS_UP_TIME_SD[] = " sysUpTime=\"";
static const char LANGUAGE_SD[] = " language=\"";

static void MetaSd_Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter)
{
    struct SolidSyslogMetaSd* meta = (struct SolidSyslogMetaSd*) self;

    SolidSyslogFormatter_BoundedString(formatter, SD_PREFIX, sizeof(SD_PREFIX) - 1);
    MetaSd_EmitSequenceId(meta, formatter);
    MetaSd_EmitSysUpTime(meta, formatter);
    MetaSd_EmitLanguage(meta, formatter);
    SolidSyslogFormatter_AsciiCharacter(formatter, ']');
}

static inline void MetaSd_EmitSequenceId(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_BoundedString(formatter, SEQUENCE_ID_SD, sizeof(SEQUENCE_ID_SD) - 1);
    SolidSyslogFormatter_Uint32(formatter, SolidSyslogAtomicCounter_Increment(meta->counter));
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
}

static inline void MetaSd_EmitSysUpTime(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter)
{
    if (meta->getSysUpTime != NULL)
    {
        SolidSyslogFormatter_BoundedString(formatter, SYS_UP_TIME_SD, sizeof(SYS_UP_TIME_SD) - 1);
        SolidSyslogFormatter_Uint32(formatter, meta->getSysUpTime());
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}

static inline void MetaSd_EmitLanguage(struct SolidSyslogMetaSd* meta, struct SolidSyslogFormatter* formatter)
{
    if (meta->getLanguage != NULL)
    {
        SolidSyslogFormatter_BoundedString(formatter, LANGUAGE_SD, sizeof(LANGUAGE_SD) - 1);
        meta->getLanguage(formatter);
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}
