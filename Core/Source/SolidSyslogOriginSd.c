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
    struct SolidSyslogStructuredData base;
    SolidSyslogOriginIpCountFunction getIpCount;
    SolidSyslogOriginIpAtFunction getIpAt;
    SolidSyslogFormatterStorage formattedStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(ORIGIN_FORMATTED_MAX)];
};

static const char SD_PREFIX[] = "[origin";
static const char SD_SOFTWARE_SD[] = " software=\"";
static const char SD_VERSION_SD[] = " swVersion=\"";
static const char SD_ENTERPRISE_ID_SD[] = " enterpriseId=\"";
static const char SD_IP_SD[] = " ip=\"";

static void OriginSd_Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);
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
static inline void OriginSd_EmitIps(struct SolidSyslogFormatter* formatter, const struct SolidSyslogOriginSd* origin);
static inline void OriginSd_EmitIp(
    struct SolidSyslogFormatter* formatter,
    const struct SolidSyslogOriginSd* origin,
    size_t index
);

static struct SolidSyslogOriginSd instance;

struct SolidSyslogStructuredData* SolidSyslogOriginSd_Create(const struct SolidSyslogOriginSdConfig* config)
{
    instance.base.Format = OriginSd_Format;
    instance.getIpCount = config->getIpCount;
    instance.getIpAt = config->getIpAt;
    OriginSd_PreFormatStaticPrefix(config);
    return &instance.base;
}

void SolidSyslogOriginSd_Destroy(void)
{
    instance.base.Format = NULL;
    instance.getIpCount = NULL;
    instance.getIpAt = NULL;
}

static void OriginSd_Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter)
{
    struct SolidSyslogOriginSd* origin = (struct SolidSyslogOriginSd*) self;
    struct SolidSyslogFormatter* preformat = SolidSyslogFormatter_FromStorage(origin->formattedStorage);

    SolidSyslogFormatter_BoundedString(
        formatter,
        SolidSyslogFormatter_AsFormattedBuffer(preformat),
        SolidSyslogFormatter_Length(preformat)
    );
    OriginSd_EmitIps(formatter, origin);
    SolidSyslogFormatter_AsciiCharacter(formatter, ']');
}

static inline void OriginSd_PreFormatStaticPrefix(const struct SolidSyslogOriginSdConfig* config)
{
    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(instance.formattedStorage, ORIGIN_FORMATTED_MAX);

    SolidSyslogFormatter_BoundedString(f, SD_PREFIX, sizeof(SD_PREFIX) - 1);
    OriginSd_EmitSoftware(f, config);
    OriginSd_EmitSwVersion(f, config);
    OriginSd_EmitEnterpriseId(f, config);
    /* closing ']' deferred to OriginSd_Format() so per-message ip params can be spliced in */
}

static inline void OriginSd_EmitSoftware(struct SolidSyslogFormatter* f, const struct SolidSyslogOriginSdConfig* config)
{
    if (config->software != NULL)
    {
        SolidSyslogFormatter_BoundedString(f, SD_SOFTWARE_SD, sizeof(SD_SOFTWARE_SD) - 1);
        SolidSyslogFormatter_EscapedString(f, config->software, ORIGIN_SOFTWARE_MAX);
        SolidSyslogFormatter_AsciiCharacter(f, '"');
    }
}

static inline void OriginSd_EmitSwVersion(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogOriginSdConfig* config
)
{
    if (config->swVersion != NULL)
    {
        SolidSyslogFormatter_BoundedString(f, SD_VERSION_SD, sizeof(SD_VERSION_SD) - 1);
        SolidSyslogFormatter_EscapedString(f, config->swVersion, ORIGIN_SWVERSION_MAX);
        SolidSyslogFormatter_AsciiCharacter(f, '"');
    }
}

static inline void OriginSd_EmitEnterpriseId(
    struct SolidSyslogFormatter* f,
    const struct SolidSyslogOriginSdConfig* config
)
{
    if (config->enterpriseId != NULL)
    {
        SolidSyslogFormatter_BoundedString(f, SD_ENTERPRISE_ID_SD, sizeof(SD_ENTERPRISE_ID_SD) - 1);
        SolidSyslogFormatter_EscapedString(f, config->enterpriseId, ORIGIN_ENTERPRISE_ID_MAX);
        SolidSyslogFormatter_AsciiCharacter(f, '"');
    }
}

static inline void OriginSd_EmitIps(struct SolidSyslogFormatter* formatter, const struct SolidSyslogOriginSd* origin)
{
    if ((origin->getIpCount != NULL) && (origin->getIpAt != NULL))
    {
        size_t count = origin->getIpCount();
        for (size_t i = 0; i < count; i++)
        {
            OriginSd_EmitIp(formatter, origin, i);
        }
    }
}

static inline void OriginSd_EmitIp(
    struct SolidSyslogFormatter* formatter,
    const struct SolidSyslogOriginSd* origin,
    size_t index
)
{
    SolidSyslogFormatter_BoundedString(formatter, SD_IP_SD, sizeof(SD_IP_SD) - 1);
    origin->getIpAt(formatter, index);
    SolidSyslogFormatter_AsciiCharacter(formatter, '"');
}
