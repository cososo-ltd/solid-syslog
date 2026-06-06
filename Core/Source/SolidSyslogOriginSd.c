#include "SolidSyslogOriginSd.h"

#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNullSd.h"
#include "SolidSyslogOriginSdErrors.h"
#include "SolidSyslogOriginSdPrivate.h"
#include "SolidSyslogSdElement.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogStructuredDataDefinition.h"

const struct SolidSyslogErrorSource OriginSdErrorSource = {"OriginSd"};

static void OriginSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element);

static inline struct SolidSyslogOriginSd* OriginSd_SelfFromBase(struct SolidSyslogStructuredData* base);
static inline void OriginSd_EmitSoftware(struct SolidSyslogSdElement* element, const char* software);
static inline void OriginSd_EmitSwVersion(struct SolidSyslogSdElement* element, const char* swVersion);
static inline void OriginSd_EmitEnterpriseId(struct SolidSyslogSdElement* element, const char* enterpriseId);
static inline void OriginSd_EmitIps(struct SolidSyslogSdElement* element, const struct SolidSyslogOriginSd* self);

void OriginSd_Initialise(struct SolidSyslogStructuredData* base, const struct SolidSyslogOriginSdConfig* config)
{
    struct SolidSyslogOriginSd* self = OriginSd_SelfFromBase(base);
    self->Base.Format = OriginSd_Format;
    self->Software = config->Software;
    self->SwVersion = config->SwVersion;
    self->EnterpriseId = config->EnterpriseId;
    self->GetIpCount = config->GetIpCount;
    self->GetIpAt = config->GetIpAt;
    self->IpContext = config->IpContext;
}

void OriginSd_Cleanup(struct SolidSyslogStructuredData* base)
{
    /* Overwrite the abstract base with the shared NullSd vtable so use-after-destroy
     * is a safe no-op. Derived fields are private to this TU; the next _Initialise
     * overwrites them. */
    *base = *SolidSyslogNullSd_Get();
}

static void OriginSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    struct SolidSyslogOriginSd* self = OriginSd_SelfFromBase(base);

    SolidSyslogSdElement_Begin(element, "origin", 0U);
    OriginSd_EmitSoftware(element, self->Software);
    OriginSd_EmitSwVersion(element, self->SwVersion);
    OriginSd_EmitEnterpriseId(element, self->EnterpriseId);
    OriginSd_EmitIps(element, self);
    SolidSyslogSdElement_End(element);
}

static inline struct SolidSyslogOriginSd* OriginSd_SelfFromBase(struct SolidSyslogStructuredData* base)
{
    return (struct SolidSyslogOriginSd*) base;
}

static inline void OriginSd_EmitSoftware(struct SolidSyslogSdElement* element, const char* software)
{
    if (software != NULL)
    {
        SolidSyslogSdValue_BoundedString(
            SolidSyslogSdElement_Param(element, "software"),
            software,
            ORIGIN_SOFTWARE_MAX
        );
    }
}

static inline void OriginSd_EmitSwVersion(struct SolidSyslogSdElement* element, const char* swVersion)
{
    if (swVersion != NULL)
    {
        SolidSyslogSdValue_BoundedString(
            SolidSyslogSdElement_Param(element, "swVersion"),
            swVersion,
            ORIGIN_SWVERSION_MAX
        );
    }
}

static inline void OriginSd_EmitEnterpriseId(struct SolidSyslogSdElement* element, const char* enterpriseId)
{
    if (enterpriseId != NULL)
    {
        SolidSyslogSdValue_BoundedString(
            SolidSyslogSdElement_Param(element, "enterpriseId"),
            enterpriseId,
            ORIGIN_ENTERPRISE_ID_MAX
        );
    }
}

static inline void OriginSd_EmitIps(struct SolidSyslogSdElement* element, const struct SolidSyslogOriginSd* self)
{
    if ((self->GetIpCount != NULL) && (self->GetIpAt != NULL))
    {
        size_t count = self->GetIpCount();
        for (size_t i = 0; i < count; i++)
        {
            self->GetIpAt(SolidSyslogSdElement_Param(element, "ip"), self->IpContext, i);
        }
    }
}
