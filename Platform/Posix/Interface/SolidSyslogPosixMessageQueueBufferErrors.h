/** @file
 *  Error codes and Source identity for the PosixMessageQueueBuffer adapter.
 *
 *  @ingroup platform_posix */
#ifndef SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERERRORS_H
#define SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is PosixMessageQueueBufferErrorSource.
     *  A handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault (MQ_OPEN at Create, SEND on enqueue, RECEIVE
     *  on drain). */
    enum SolidSyslogPosixMessageQueueBufferErrors
    {
        POSIXMESSAGEQUEUEBUFFER_ERROR_POOL_EXHAUSTED,
        POSIXMESSAGEQUEUEBUFFER_ERROR_UNKNOWN_DESTROY,
        POSIXMESSAGEQUEUEBUFFER_ERROR_MQ_OPEN_FAILED,
        POSIXMESSAGEQUEUEBUFFER_ERROR_SEND_FAILED,
        POSIXMESSAGEQUEUEBUFFER_ERROR_RECEIVE_FAILED,
        POSIXMESSAGEQUEUEBUFFER_ERROR_MAX /**< One past the last code; never emitted. Bounds iteration. */
    };

    /** Identity for events raised by a PosixMessageQueueBuffer. A handler matches
     *  by address (event->Source == &PosixMessageQueueBufferErrorSource), then
     *  reads event->Detail as an enum SolidSyslogPosixMessageQueueBufferErrors. */
    extern const struct SolidSyslogErrorSource PosixMessageQueueBufferErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERERRORS_H */
