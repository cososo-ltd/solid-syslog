/** @file
 *  A composite Sender that fronts several inner senders and routes each message
 *  through the one the Selector picks — the wiring for dual-SIEM fan-out or an
 *  active/standby failover stack. The Selector is called on every Send and
 *  returns an index into the Senders array; a value at or beyond SenderCount
 *  (including any value when the array is empty) routes to the shared
 *  NullSender, which drops the message so a misconfigured selector never retains
 *  records in the Store. Switching away from a sender disconnects it, so only
 *  the currently selected inner sender holds a live connection; the inner
 *  senders themselves are caller-owned and are not disconnected or freed on
 *  Destroy. */
#ifndef SOLIDSYSLOGSWITCHINGSENDER_H
#define SOLIDSYSLOGSWITCHINGSENDER_H

#include <stddef.h>
#include <stdint.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSender;

    /** Returns the index into Senders of the sender to route the next message
     *  through. Called on every Send, so it must be cheap; an index at or beyond
     *  SenderCount (including any value when SenderCount is 0) routes to the shared
     *  NullSender, which drops the message so a misconfigured selector never
     *  retains records in the Store. @p context is SelectorContext, passed through unchanged. */
    typedef uint8_t (*SolidSyslogSwitchingSenderSelector)(void* context);

    /** Wiring for SolidSyslogSwitchingSender_Create, a sender that fronts several
     *  inner senders and picks one per message via Selector (dual-SIEM, failover).
     *  Senders and Selector are required (NULL makes Create fall back to the shared
     *  NullSender). The Senders array and the inner senders it points to must
     *  outlive the created handle, but the config struct itself is copied at Create
     *  and may be transient. Switching away from a sender disconnects it. */
    struct SolidSyslogSwitchingSenderConfig
    {
        struct SolidSyslogSender** Senders;
        /** Length of Senders; also the exclusive upper bound the Selector's index is checked against. */
        size_t SenderCount;
        SolidSyslogSwitchingSenderSelector Selector;
        void* SelectorContext; /**< Passed to Selector unchanged. */
    };

    /** Create a switching sender from @p config. Never returns NULL: a NULL or
     *  invalid config, or an exhausted sender pool, reports via SolidSyslog_Error
     *  and returns the shared NullSender (Send drops on the floor), so the result
     *  is safe to wire without a null-check. */
    struct SolidSyslogSender* SolidSyslogSwitchingSender_Create(const struct SolidSyslogSwitchingSenderConfig* config);

    /** Release the sender's pool slot. Does not disconnect or free the inner
     *  senders; the caller owns those. */
    void SolidSyslogSwitchingSender_Destroy(struct SolidSyslogSender * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGSWITCHINGSENDER_H */
