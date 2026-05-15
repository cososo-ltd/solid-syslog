#ifndef SOLIDSYSLOGSWITCHINGSENDER_H
#define SOLIDSYSLOGSWITCHINGSENDER_H

#include <stddef.h>
#include <stdint.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    typedef uint8_t (*SolidSyslogSwitchingSenderSelector)(void); // NOLINT(modernize-redundant-void-arg) -- C idiom

    struct SolidSyslogSwitchingSenderConfig
    {
        struct SolidSyslogSender** Senders;
        size_t SenderCount;
        SolidSyslogSwitchingSenderSelector Selector;
    };

    struct SolidSyslogSender* SolidSyslogSwitchingSender_Create(const struct SolidSyslogSwitchingSenderConfig* config);
    void SolidSyslogSwitchingSender_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGSWITCHINGSENDER_H */
