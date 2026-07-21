#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogSwitchingSenderErrors.h"
#include "SolidSyslogSwitchingSenderPrivate.h"

const struct SolidSyslogErrorSource SwitchingSenderErrorSource = {"SwitchingSender"};

static bool SwitchingSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size);
static void SwitchingSender_Disconnect(struct SolidSyslogSender* base);

static inline struct SolidSyslogSwitchingSender* SwitchingSender_SelfFromBase(struct SolidSyslogSender* base);
static inline void SwitchingSender_SelectSender(struct SolidSyslogSwitchingSender* self);
static inline bool SwitchingSender_SenderChanged(
    const struct SolidSyslogSwitchingSender* self,
    const struct SolidSyslogSender* requestedSender
);
static inline void SwitchingSender_SwitchTo(
    struct SolidSyslogSwitchingSender* self,
    struct SolidSyslogSender* newCurrent
);
static inline struct SolidSyslogSender* SwitchingSender_RequestedSender(const struct SolidSyslogSwitchingSender* self);

void SwitchingSender_Initialise(struct SolidSyslogSender* base, const struct SolidSyslogSwitchingSenderConfig* config)
{
    struct SolidSyslogSwitchingSender* self = SwitchingSender_SelfFromBase(base);
    self->Base.Send = SwitchingSender_Send;
    self->Base.Disconnect = SwitchingSender_Disconnect;
    self->Config = *config;
    self->CurrentSender = SolidSyslogNullSender_Get();
}

void SwitchingSender_Cleanup(struct SolidSyslogSender* base)
{
    /* Overwrite the abstract base with the shared NullSender vtable so use-after-destroy
     * is a safe no-op. SwitchingSender does not own its inner senders' connections, so
     * unlike UdpSender/StreamSender there is nothing to disconnect first. Derived fields
     * are private to this TU; the next _Initialise overwrites them. */
    *base = *SolidSyslogNullSender_Get();
}

static bool SwitchingSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size)
{
    struct SolidSyslogSwitchingSender* self = SwitchingSender_SelfFromBase(base);
    SwitchingSender_SelectSender(self);
    return SolidSyslogSender_Send(self->CurrentSender, buffer, size);
}

static void SwitchingSender_Disconnect(struct SolidSyslogSender* base)
{
    struct SolidSyslogSwitchingSender* self = SwitchingSender_SelfFromBase(base);
    SolidSyslogSender_Disconnect(self->CurrentSender);
}

static inline struct SolidSyslogSwitchingSender* SwitchingSender_SelfFromBase(struct SolidSyslogSender* base)
{
    return (struct SolidSyslogSwitchingSender*) base;
}

static inline void SwitchingSender_SelectSender(struct SolidSyslogSwitchingSender* self)
{
    struct SolidSyslogSender* requestedSender = SwitchingSender_RequestedSender(self);

    if (SwitchingSender_SenderChanged(self, requestedSender))
    {
        SwitchingSender_SwitchTo(self, requestedSender);
    }
}

static inline bool SwitchingSender_SenderChanged(
    const struct SolidSyslogSwitchingSender* self,
    const struct SolidSyslogSender* requestedSender
)
{
    return requestedSender != self->CurrentSender;
}

static inline void SwitchingSender_SwitchTo(
    struct SolidSyslogSwitchingSender* self,
    struct SolidSyslogSender* newCurrent
)
{
    SolidSyslogSender_Disconnect(self->CurrentSender);
    self->CurrentSender = newCurrent;
}

/* Out-of-range selector index (including empty-array case) resolves to the
 * shared NullSender. NullSender.Send returns true so the Service algorithm
 * drops the message — a misconfigured selector must not retain messages in
 * the Store. */
static inline struct SolidSyslogSender* SwitchingSender_RequestedSender(const struct SolidSyslogSwitchingSender* self)
{
    struct SolidSyslogSender* result = SolidSyslogNullSender_Get();
    uint8_t index = self->Config.Selector(self->Config.SelectorContext);

    if (index < self->Config.SenderCount)
    {
        result = self->Config.Senders[index];
    }

    return result;
}
