/** @file
 *  Error codes and Source identity for the FileBlockDevice. */
#ifndef SOLIDSYSLOGFILEBLOCKDEVICEERRORS_H
#define SOLIDSYSLOGFILEBLOCKDEVICEERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is FileBlockDeviceErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogFileBlockDeviceErrors
    {
        SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a FileBlockDevice. A handler matches by
     *  address (event->Source == &FileBlockDeviceErrorSource), then reads
     *  event->Detail as an enum SolidSyslogFileBlockDeviceErrors. */
    extern const struct SolidSyslogErrorSource FileBlockDeviceErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFILEBLOCKDEVICEERRORS_H */
