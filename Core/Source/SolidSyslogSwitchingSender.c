#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogSender.h"

struct SolidSyslogSwitchingSender
{
    struct SolidSyslogSender                base;
    struct SolidSyslogSwitchingSenderConfig config;
    struct SolidSyslogSender*               currentSender;
};

static bool                             Send(struct SolidSyslogSender* sender, const void* buffer, size_t size);
static void                             Disconnect(struct SolidSyslogSender* sender);
static inline void                      SelectSender(struct SolidSyslogSwitchingSender* self);
static inline bool                      SenderChanged(const struct SolidSyslogSwitchingSender* self, const struct SolidSyslogSender* requestedSender);
static inline void                      SwitchTo(struct SolidSyslogSwitchingSender* self, struct SolidSyslogSender* newCurrent);
static inline struct SolidSyslogSender* RequestedSender(const struct SolidSyslogSwitchingSender* self);
static bool                             NilSend(struct SolidSyslogSender* sender, const void* buffer, size_t size);
static void                             NilDisconnect(struct SolidSyslogSender* sender);

static struct SolidSyslogSender                NIL_SENDER       = {NilSend, NilDisconnect};
static const struct SolidSyslogSwitchingSender DEFAULT_INSTANCE = {.currentSender = &NIL_SENDER};
static struct SolidSyslogSwitchingSender       instance;

struct SolidSyslogSender* SolidSyslogSwitchingSender_Create(const struct SolidSyslogSwitchingSenderConfig* config)
{
    instance                 = DEFAULT_INSTANCE;
    instance.config          = *config;
    instance.base.Send       = Send;
    instance.base.Disconnect = Disconnect;
    return &instance.base;
}

void SolidSyslogSwitchingSender_Destroy(void)
{
    instance = DEFAULT_INSTANCE;
}

static bool Send(struct SolidSyslogSender* sender, const void* buffer, size_t size)
{
    struct SolidSyslogSwitchingSender* self = (struct SolidSyslogSwitchingSender*) sender;
    SelectSender(self);
    return SolidSyslogSender_Send(self->currentSender, buffer, size);
}

static void Disconnect(struct SolidSyslogSender* sender)
{
    struct SolidSyslogSwitchingSender* self = (struct SolidSyslogSwitchingSender*) sender;
    SolidSyslogSender_Disconnect(self->currentSender);
}

static inline void SelectSender(struct SolidSyslogSwitchingSender* self)
{
    struct SolidSyslogSender* requestedSender = RequestedSender(self);

    if (SenderChanged(self, requestedSender))
    {
        SwitchTo(self, requestedSender);
    }
}

static inline bool SenderChanged(const struct SolidSyslogSwitchingSender* self, const struct SolidSyslogSender* requestedSender)
{
    return requestedSender != self->currentSender;
}

static inline void SwitchTo(struct SolidSyslogSwitchingSender* self, struct SolidSyslogSender* newCurrent)
{
    SolidSyslogSender_Disconnect(self->currentSender);
    self->currentSender = newCurrent;
}

/* Falls back to the nil sender when the selector returns an out-of-range
 * index (including the empty-array case). Contains the application contract
 * violation of an invalid selector without corrupting memory or crashing. */
static inline struct SolidSyslogSender* RequestedSender(const struct SolidSyslogSwitchingSender* self)
{
    uint8_t                   index  = self->config.selector();
    struct SolidSyslogSender* result = &NIL_SENDER;

    if (index < self->config.senderCount)
    {
        result = self->config.senders[index];
    }

    return result;
}

static bool NilSend(struct SolidSyslogSender* sender, const void* buffer, size_t size)
{
    (void) sender;
    (void) buffer;
    (void) size;
    return false;
}

static void NilDisconnect(struct SolidSyslogSender* sender)
{
    (void) sender;
}
