#ifndef SOLIDSYSLOGSWITCHINGSENDER_H
#define SOLIDSYSLOGSWITCHINGSENDER_H

#include <stddef.h>
#include <stdint.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSender;

    typedef uint8_t (*SolidSyslogSwitchingSenderSelector)(void);

    struct SolidSyslogSwitchingSenderConfig
    {
        struct SolidSyslogSender** Senders;
        size_t SenderCount;
        SolidSyslogSwitchingSenderSelector Selector;
    };

    struct SolidSyslogSender* SolidSyslogSwitchingSender_Create(const struct SolidSyslogSwitchingSenderConfig* config);
    void SolidSyslogSwitchingSender_Destroy(struct SolidSyslogSender * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGSWITCHINGSENDER_H */
