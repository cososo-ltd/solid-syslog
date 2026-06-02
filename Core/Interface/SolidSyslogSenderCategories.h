#ifndef SOLIDSYSLOGSENDERCATEGORIES_H
#define SOLIDSYSLOGSENDERCATEGORIES_H

#include "ExternC.h"

#include "SolidSyslogErrorCategory.h"

EXTERN_C_BEGIN

    /*
     * Portable Sender-role error categories. Any Sender implementation
     * (UDP, stream, switching, or an integrator's own) reuses these; a
     * portable handler switch on event->Category works identically across
     * all of them.
     */
    enum
    {
        /* SOLIDSYSLOG_CAT_SENDER_BASE + 1 */
        SOLIDSYSLOG_CAT_SENDER_SEND_NULL_BUFFER = 0x0101
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSENDERCATEGORIES_H */
