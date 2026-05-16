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

static const char SD_PREFIX[] = "[origin";
static const char SD_SOFTWARE_SD[] = " software=\"";
static const char SD_VERSION_SD[] = " swVersion=\"";
static const char SD_ENTERPRISE_ID_SD[] = " enterpriseId=\"";
static const char SD_IP_SD[] = " ip=\"";

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

static struct SolidSyslogOriginSd instance;

struct SolidSyslogStructuredData* SolidSyslogOriginSd_Create(const struct SolidSyslogOriginSdConfig* config)
{
    instance.Base.Format = OriginSd_Format;
    instance.GetIpCount = config->GetIpCount;
    instance.GetIpAt = config->GetIpAt;
    OriginSd_PreFormatStaticPrefix(config);
    return &instance.Base;
}

void SolidSyslogOriginSd_Destroy(void)
{
    instance.Base.Format = NULL;
    instance.GetIpCount = NULL;
    instance.GetIpAt = NULL;
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
    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(instance.FormattedStorage, ORIGIN_FORMATTED_MAX);

    SolidSyslogFormatter_BoundedString(f, SD_PREFIX, sizeof(SD_PREFIX) - 1U);
    OriginSd_EmitSoftware(f, config);
    OriginSd_EmitSwVersion(f, config);
    OriginSd_EmitEnterpriseId(f, config);
    /* closing ']' deferred to OriginSd_Format() so per-message ip params can be spliced in */
}

static inline void OriginSd_EmitSoftware(struct SolidSyslogFormatter* f, const struct SolidSyslogOriginSdConfig* config)
{
    if (config->Software != NULL)
    {
        SolidSyslogFormatter_BoundedString(f, SD_SOFTWARE_SD, sizeof(SD_SOFTWARE_SD) - 1U);
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
        SolidSyslogFormatter_BoundedString(f, SD_VERSION_SD, sizeof(SD_VERSION_SD) - 1U);
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
        SolidSyslogFormatter_BoundedString(f, SD_ENTERPRISE_ID_SD, sizeof(SD_ENTERPRISE_ID_SD) - 1U);
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
    SolidSyslogFormatter_BoundedString(formatter, SD_IP_SD, sizeof(SD_IP_SD) - 1U);
    self->GetIpAt(formatter, index);
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
}
