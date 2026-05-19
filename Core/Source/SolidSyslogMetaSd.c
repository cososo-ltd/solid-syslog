#include "SolidSyslogMetaSd.h"

#include <stddef.h>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMetaSdPrivate.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogFormatter;

static void MetaSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter);

static inline struct SolidSyslogMetaSd* MetaSd_SelfFromBase(struct SolidSyslogStructuredData* base);
static inline void MetaSd_EmitSequenceId(struct SolidSyslogMetaSd* self, struct SolidSyslogFormatter* formatter);
static inline void MetaSd_EmitSysUpTime(struct SolidSyslogMetaSd* self, struct SolidSyslogFormatter* formatter);
static inline void MetaSd_EmitLanguage(struct SolidSyslogMetaSd* self, struct SolidSyslogFormatter* formatter);

void MetaSd_Initialise(struct SolidSyslogStructuredData* base, const struct SolidSyslogMetaSdConfig* config)
{
    struct SolidSyslogMetaSd* self = MetaSd_SelfFromBase(base);
    self->Base.Format = MetaSd_Format;
    self->Counter = config->Counter;
    self->GetSysUpTime = config->GetSysUpTime;
    self->GetLanguage = config->GetLanguage;
}

void MetaSd_Cleanup(struct SolidSyslogStructuredData* base)
{
    /* Overwrite the abstract base with the shared NullSd vtable so use-after-destroy
     * is a safe no-op rather than a NULL-fn-pointer crash. Derived fields are private
     * to this TU; the next _Initialise overwrites them. */
    *base = *SolidSyslogNullSd_Get();
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
