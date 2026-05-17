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
    struct SolidSyslogStructuredData Base;
    struct SolidSyslogAtomicCounter* Counter;
    SolidSyslogSysUpTimeFunction GetSysUpTime;
    SolidSyslogStringFunction GetLanguage;
};

static void MetaSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter);
static void MetaSd_NilMetaSdFormat(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter);

static inline struct SolidSyslogMetaSd* MetaSd_SelfFromBase(struct SolidSyslogStructuredData* base);
static inline void MetaSd_EmitSequenceId(struct SolidSyslogMetaSd* self, struct SolidSyslogFormatter* formatter);
static inline void MetaSd_EmitSysUpTime(struct SolidSyslogMetaSd* self, struct SolidSyslogFormatter* formatter);
static inline void MetaSd_EmitLanguage(struct SolidSyslogMetaSd* self, struct SolidSyslogFormatter* formatter);

static struct SolidSyslogMetaSd MetaSd_Instance;

struct SolidSyslogStructuredData* SolidSyslogMetaSd_Create(const struct SolidSyslogMetaSdConfig* config)
{
    static struct SolidSyslogStructuredData nilMetaSd = {.Format = MetaSd_NilMetaSdFormat};
    struct SolidSyslogStructuredData* result = &nilMetaSd;
    if (config == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_METASD_CREATE_NULL_CONFIG);
    }
    else if (config->Counter == NULL)
    {
        SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, SOLIDSYSLOG_ERROR_MSG_METASD_CREATE_NULL_COUNTER);
    }
    else
    {
        MetaSd_Instance.Base.Format = MetaSd_Format;
        MetaSd_Instance.Counter = config->Counter;
        MetaSd_Instance.GetSysUpTime = config->GetSysUpTime;
        MetaSd_Instance.GetLanguage = config->GetLanguage;
        result = &MetaSd_Instance.Base;
    }
    return result;
}

void SolidSyslogMetaSd_Destroy(void)
{
    MetaSd_Instance.Base.Format = NULL;
    MetaSd_Instance.Counter = NULL;
    MetaSd_Instance.GetSysUpTime = NULL;
    MetaSd_Instance.GetLanguage = NULL;
}

static void MetaSd_NilMetaSdFormat(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter)
{
    (void) base;
    (void) formatter;
}

static void MetaSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter)
{
    static const char sdPrefix[] = "[meta";
    struct SolidSyslogMetaSd* self = MetaSd_SelfFromBase(base);

    SolidSyslogFormatter_BoundedString(formatter, sdPrefix, sizeof(sdPrefix) - 1U);
    MetaSd_EmitSequenceId(self, formatter);
    MetaSd_EmitSysUpTime(self, formatter);
    MetaSd_EmitLanguage(self, formatter);
    SolidSyslogFormatter_AsciiCharacter(formatter, ']');
}

static inline struct SolidSyslogMetaSd* MetaSd_SelfFromBase(struct SolidSyslogStructuredData* base)
{
    return (struct SolidSyslogMetaSd*) base;
}

static inline void MetaSd_EmitSequenceId(struct SolidSyslogMetaSd* self, struct SolidSyslogFormatter* formatter)
{
    static const char sequenceIdSd[] = " sequenceId=\"";
    SolidSyslogFormatter_BoundedString(formatter, sequenceIdSd, sizeof(sequenceIdSd) - 1U);
    SolidSyslogFormatter_Uint32(formatter, SolidSyslogAtomicCounter_Increment(self->Counter));
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
}

static inline void MetaSd_EmitSysUpTime(struct SolidSyslogMetaSd* self, struct SolidSyslogFormatter* formatter)
{
    if (self->GetSysUpTime != NULL)
    {
        static const char sysUpTimeSd[] = " sysUpTime=\"";
        SolidSyslogFormatter_BoundedString(formatter, sysUpTimeSd, sizeof(sysUpTimeSd) - 1U);
        SolidSyslogFormatter_Uint32(formatter, self->GetSysUpTime());
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}

static inline void MetaSd_EmitLanguage(struct SolidSyslogMetaSd* self, struct SolidSyslogFormatter* formatter)
{
    if (self->GetLanguage != NULL)
    {
        static const char languageSd[] = " language=\"";
        SolidSyslogFormatter_BoundedString(formatter, languageSd, sizeof(languageSd) - 1U);
        self->GetLanguage(formatter);
        SolidSyslogFormatter_AsciiCharacter(formatter, '"');
    }
}
