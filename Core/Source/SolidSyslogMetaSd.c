#include "SolidSyslogMetaSd.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogStructuredDataDefinition.h"

#include <stdint.h>

struct SolidSyslogMetaSd
{
    struct SolidSyslogStructuredData base;
    struct SolidSyslogAtomicCounter* counter;
};

static void Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter);

static struct SolidSyslogMetaSd instance;

struct SolidSyslogStructuredData* SolidSyslogMetaSd_Create(const struct SolidSyslogMetaSdConfig* config)
{
    instance.base.Format = Format;
    instance.counter     = config->counter;
    return &instance.base;
}

void SolidSyslogMetaSd_Destroy(void)
{
    instance.base.Format = NULL;
    instance.counter     = NULL;
}

static const char SD_PREFIX[] = "[meta sequenceId=\"";
static const char SD_SUFFIX[] = "\"]";

static void Format(struct SolidSyslogStructuredData* self, struct SolidSyslogFormatter* formatter)
{
    struct SolidSyslogMetaSd* meta = (struct SolidSyslogMetaSd*) self;
    uint_fast32_t             id   = SolidSyslogAtomicCounter_Increment(meta->counter);

    SolidSyslogFormatter_BoundedString(formatter, SD_PREFIX, sizeof(SD_PREFIX) - 1);
    SolidSyslogFormatter_Uint32(formatter, (uint32_t) id);
    SolidSyslogFormatter_BoundedString(formatter, SD_SUFFIX, sizeof(SD_SUFFIX) - 1);
}
