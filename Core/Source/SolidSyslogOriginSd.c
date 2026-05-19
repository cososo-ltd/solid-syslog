#include "SolidSyslogOriginSd.h"

#include <stddef.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogOriginSdPrivate.h"
#include "SolidSyslogStructuredDataDefinition.h"

static void OriginSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter);

static inline struct SolidSyslogOriginSd* OriginSd_SelfFromBase(struct SolidSyslogStructuredData* base);
static inline void OriginSd_PreFormatStaticPrefix(
    struct SolidSyslogOriginSd* self,
    const struct SolidSyslogOriginSdConfig* config
);
static inline void OriginSd_EmitSoftware(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogOriginSdConfig* config
);
static inline void OriginSd_EmitSwVersion(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogOriginSdConfig* config
);
static inline void OriginSd_EmitEnterpriseId(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogOriginSdConfig* config
);
static inline void OriginSd_EmitIps(struct SolidSyslogFormatter* formatter, const struct SolidSyslogOriginSd* self);
static inline void OriginSd_EmitIp(
    struct SolidSyslogFormatter* formatter,
    const struct SolidSyslogOriginSd* self,
    size_t index
);

void OriginSd_Initialise(struct SolidSyslogStructuredData* base, const struct SolidSyslogOriginSdConfig* config)
{
    struct SolidSyslogOriginSd* self = OriginSd_SelfFromBase(base);
    self->Base.Format = OriginSd_Format;
    self->GetIpCount = config->GetIpCount;
    self->GetIpAt = config->GetIpAt;
    OriginSd_PreFormatStaticPrefix(self, config);
}

void OriginSd_Cleanup(struct SolidSyslogStructuredData* base)
{
    /* Overwrite the abstract base with the shared NullSd vtable so use-after-destroy
     * is a safe no-op. Derived fields (incl. the pre-formatted static-prefix Formatter
     * storage) are private to this TU; the next _Initialise rebuilds them. */
    *base = *SolidSyslogNullSd_Get();
}

static void OriginSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter)
{
    struct SolidSyslogOriginSd* self = OriginSd_SelfFromBase(base);
    struct SolidSyslogFormatter* preformat = SolidSyslogFormatter_FromStorage(self->FormattedStorage);

    SolidSyslogFormatter_BoundedString(
        formatter,
        SolidSyslogFormatter_AsFormattedBuffer(preformat),
        SolidSyslogFormatter_Length(preformat)
    );
    OriginSd_EmitIps(formatter, self);
    SolidSyslogFormatter_AsciiCharacter(formatter, ']');
}

static inline struct SolidSyslogOriginSd* OriginSd_SelfFromBase(struct SolidSyslogStructuredData* base)
{
    return (struct SolidSyslogOriginSd*) base;
}

static inline void OriginSd_PreFormatStaticPrefix(
    struct SolidSyslogOriginSd* self,
    const struct SolidSyslogOriginSdConfig* config
)
{
    static const char sdPrefix[] = "[origin";
    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(self->FormattedStorage, ORIGIN_FORMATTED_MAX);

    SolidSyslogFormatter_BoundedString(f, sdPrefix, sizeof(sdPrefix) - 1U);
    OriginSd_EmitSoftware(f, config);
    OriginSd_EmitSwVersion(f, config);
    OriginSd_EmitEnterpriseId(f, config);
    /* closing ']' deferred to OriginSd_Format() so per-message ip params can be spliced in */
}

static inline void OriginSd_EmitSoftware(struct SolidSyslogFormatter* f, const struct SolidSyslogOriginSdConfig* config)
{
    if (config->Software != NULL)
    {
        static const char softwareSd[] = " software=\"";
        SolidSyslogFormatter_BoundedString(f, softwareSd, sizeof(softwareSd) - 1U);
        SolidSyslogFormatter_EscapedString(f, config->Software, ORIGIN_SOFTWARE_MAX);
        SolidSyslogFormatter_AsciiCharacter(f, '"');
    }
}

static inline void OriginSd_EmitSwVersion(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogOriginSdConfig* config
)
{
    if (config->SwVersion != NULL)
    {
        static const char swVersionSd[] = " swVersion=\"";
        SolidSyslogFormatter_BoundedString(f, swVersionSd, sizeof(swVersionSd) - 1U);
        SolidSyslogFormatter_EscapedString(f, config->SwVersion, ORIGIN_SWVERSION_MAX);
        SolidSyslogFormatter_AsciiCharacter(f, '"');
    }
}

static inline void OriginSd_EmitEnterpriseId(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogOriginSdConfig* config
)
{
    if (config->EnterpriseId != NULL)
    {
        static const char enterpriseIdSd[] = " enterpriseId=\"";
        SolidSyslogFormatter_BoundedString(f, enterpriseIdSd, sizeof(enterpriseIdSd) - 1U);
        SolidSyslogFormatter_EscapedString(f, config->EnterpriseId, ORIGIN_ENTERPRISE_ID_MAX);
        SolidSyslogFormatter_AsciiCharacter(f, '"');
    }
}

static inline void OriginSd_EmitIps(struct SolidSyslogFormatter* formatter, const struct SolidSyslogOriginSd* self)
{
    if ((self->GetIpCount != NULL) && (self->GetIpAt != NULL))
    {
        size_t count = self->GetIpCount();
        for (size_t i = 0; i < count; i++)
        {
            OriginSd_EmitIp(formatter, self, i);
        }
    }
}

static inline void OriginSd_EmitIp(
    struct SolidSyslogFormatter* formatter,
    const struct SolidSyslogOriginSd* self,
    size_t index
)
{
    static const char ipSd[] = " ip=\"";
    SolidSyslogFormatter_BoundedString(formatter, ipSd, sizeof(ipSd) - 1U);
    self->GetIpAt(formatter, index);
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
}
