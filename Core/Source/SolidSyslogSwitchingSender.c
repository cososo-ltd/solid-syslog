#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogSender.h"

struct SolidSyslogSwitchingSender
{
    struct SolidSyslogSender base;
    struct SolidSyslogSwitchingSenderConfig config;
    struct SolidSyslogSender* currentSender;
};

static bool SwitchingSender_Send(struct SolidSyslogSender* sender, const void* buffer, size_t size);
static void SwitchingSender_Disconnect(struct SolidSyslogSender* sender);
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
static bool SwitchingSender_NilSend(struct SolidSyslogSender* sender, const void* buffer, size_t size);
static void SwitchingSender_NilDisconnect(struct SolidSyslogSender* sender);

static struct SolidSyslogSender NIL_SENDER = {SwitchingSender_NilSend, SwitchingSender_NilDisconnect};
static const struct SolidSyslogSwitchingSender DEFAULT_INSTANCE = {.currentSender = &NIL_SENDER};
static struct SolidSyslogSwitchingSender instance;

struct SolidSyslogSender* SolidSyslogSwitchingSender_Create(const struct SolidSyslogSwitchingSenderConfig* config)
{
    instance = DEFAULT_INSTANCE;
    instance.config = *config;
    instance.base.Send = SwitchingSender_Send;
    instance.base.Disconnect = SwitchingSender_Disconnect;
    return &instance.base;
}

void SolidSyslogSwitchingSender_Destroy(void)
{
    instance = DEFAULT_INSTANCE;
}

static bool SwitchingSender_Send(struct SolidSyslogSender* sender, const void* buffer, size_t size)
{
    struct SolidSyslogSwitchingSender* self = (struct SolidSyslogSwitchingSender*) sender;
    SwitchingSender_SelectSender(self);
    return SolidSyslogSender_Send(self->currentSender, buffer, size);
}

static void SwitchingSender_Disconnect(struct SolidSyslogSender* sender)
{
    struct SolidSyslogSwitchingSender* self = (struct SolidSyslogSwitchingSender*) sender;
    SolidSyslogSender_Disconnect(self->currentSender);
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
    return requestedSender != self->currentSender;
}

static inline void SwitchingSender_SwitchTo(
    struct SolidSyslogSwitchingSender* self,
    struct SolidSyslogSender* newCurrent
)
{
    SolidSyslogSender_Disconnect(self->currentSender);
    self->currentSender = newCurrent;
}

/* Falls back to the nil sender when the selector returns an out-of-range
 * index (including the empty-array case). Contains the application contract
 * violation of an invalid selector without corrupting memory or crashing. */
static inline struct SolidSyslogSender* SwitchingSender_RequestedSender(const struct SolidSyslogSwitchingSender* self)
{
    uint8_t index = self->config.selector();
    struct SolidSyslogSender* result = &NIL_SENDER;

    if (index < self->config.senderCount)
    {
        result = self->config.senders[index];
    }

    return result;
}

static bool SwitchingSender_NilSend(struct SolidSyslogSender* sender, const void* buffer, size_t size)
{
    (void) sender;
    (void) buffer;
    (void) size;
    return false;
}

static void SwitchingSender_NilDisconnect(struct SolidSyslogSender* sender)
{
    (void) sender;
}
