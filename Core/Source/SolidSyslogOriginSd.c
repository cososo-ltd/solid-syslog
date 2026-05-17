#include "SolidSyslogOriginSd.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogStructuredDataDefinition.h"

enum
{
    ORIGIN_SOFTWARE_MAX = 48,
    ORIGIN_SWVERSION_MAX = 32,
    ORIGIN_ENTERPRISE_ID_MAX = 64,
    ORIGIN_IP_MAX = 64,
    ORIGIN_LITERAL_BYTES =
        48, /* [origin software="" swVersion="" enterpriseId="" — closing ']' deferred to per-message OriginSd_Format */
    ORIGIN_CONTENT_MAX = ORIGIN_LITERAL_BYTES + SOLIDSYSLOG_ESCAPED_MAX_SIZE(ORIGIN_SOFTWARE_MAX) +
                         SOLIDSYSLOG_ESCAPED_MAX_SIZE(ORIGIN_SWVERSION_MAX) +
                         SOLIDSYSLOG_ESCAPED_MAX_SIZE(ORIGIN_ENTERPRISE_ID_MAX),
    ORIGIN_FORMATTED_MAX = ORIGIN_CONTENT_MAX + 1 /* null terminator */
};

struct SolidSyslogOriginSd
{
    struct SolidSyslogStructuredData Base;
    SolidSyslogOriginIpCountFunction GetIpCount;
    SolidSyslogOriginIpAtFunction GetIpAt;
    SolidSyslogFormatterStorage FormattedStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(ORIGIN_FORMATTED_MAX)];
};

static void OriginSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter);

static inline struct SolidSyslogOriginSd* OriginSd_SelfFromBase(struct SolidSyslogStructuredData* base);
static inline void OriginSd_PreFormatStaticPrefix(const struct SolidSyslogOriginSdConfig* config);
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

static struct SolidSyslogOriginSd OriginSd_Instance;

struct SolidSyslogStructuredData* SolidSyslogOriginSd_Create(const struct SolidSyslogOriginSdConfig* config)
{
    OriginSd_Instance.Base.Format = OriginSd_Format;
    OriginSd_Instance.GetIpCount = config->GetIpCount;
    OriginSd_Instance.GetIpAt = config->GetIpAt;
    OriginSd_PreFormatStaticPrefix(config);
    return &OriginSd_Instance.Base;
}

void SolidSyslogOriginSd_Destroy(void)
{
    OriginSd_Instance.Base.Format = NULL;
    OriginSd_Instance.GetIpCount = NULL;
    OriginSd_Instance.GetIpAt = NULL;
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

static inline void OriginSd_PreFormatStaticPrefix(const struct SolidSyslogOriginSdConfig* config)
{
    static const char sdPrefix[] = "[origin";
    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(OriginSd_Instance.FormattedStorage, ORIGIN_FORMATTED_MAX);

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
