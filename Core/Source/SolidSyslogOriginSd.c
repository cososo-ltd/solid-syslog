#include "SolidSyslogOriginSd.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogStructuredDataDefinition.h"

enum
{
    ORIGIN_SOFTWARE_MAX  = 48,
    ORIGIN_SWVERSION_MAX = 32,
    ORIGIN_LITERAL_BYTES = 33, /* [origin software="" swVersion=""] */
    ORIGIN_CONTENT_MAX   = ORIGIN_LITERAL_BYTES + SOLIDSYSLOG_ESCAPED_MAX_SIZE(ORIGIN_SOFTWARE_MAX) + SOLIDSYSLOG_ESCAPED_MAX_SIZE(ORIGIN_SWVERSION_MAX),
    ORIGIN_FORMATTED_MAX = ORIGIN_CONTENT_MAX + 1 /* null terminator */
};

struct SolidSyslogOriginSd
{
    struct SolidSyslogStructuredData base;
    SolidSyslogFormatterStorage      formattedStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(ORIGIN_FORMATTED_MAX)];
};

static const char SD_SOFTWARE_PREFIX[] = "[origin software=\"";
static const char SD_VERSION_PREFIX[]  = "\" swVersion=\"";
static const char SD_SUFFIX[]          = "\"]";

static void Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);

static struct SolidSyslogOriginSd instance;

struct SolidSyslogStructuredData* SolidSyslogOriginSd_Create(const struct SolidSyslogOriginSdConfig* config)
{
    if ((config->software == NULL) || (config->swVersion == NULL))
    {
        return NULL;
    }

    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(instance.formattedStorage, ORIGIN_FORMATTED_MAX);

    instance.base.Format = Format;
    SolidSyslogFormatter_BoundedString(f, SD_SOFTWARE_PREFIX, sizeof(SD_SOFTWARE_PREFIX) - 1);
    SolidSyslogFormatter_EscapedString(f, config->software, ORIGIN_SOFTWARE_MAX);
    SolidSyslogFormatter_BoundedString(f, SD_VERSION_PREFIX, sizeof(SD_VERSION_PREFIX) - 1);
    SolidSyslogFormatter_EscapedString(f, config->swVersion, ORIGIN_SWVERSION_MAX);
    SolidSyslogFormatter_BoundedString(f, SD_SUFFIX, sizeof(SD_SUFFIX) - 1);

    return &instance.base;
}

void SolidSyslogOriginSd_Destroy(void)
{
    instance.base.Format = NULL;
}

static void Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter)
{
    struct SolidSyslogOriginSd*  origin    = (struct SolidSyslogOriginSd*) self;
    struct SolidSyslogFormatter* preformat = SolidSyslogFormatter_FromStorage(origin->formattedStorage);

    SolidSyslogFormatter_BoundedString(formatter, SolidSyslogFormatter_AsFormattedBuffer(preformat), SolidSyslogFormatter_Length(preformat));
}
