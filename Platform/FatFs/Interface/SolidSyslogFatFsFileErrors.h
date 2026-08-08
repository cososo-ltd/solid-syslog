/** @file
 *  Error codes and Source identity for the FatFsFile adapter.
 *
 *  @ingroup platform_fatfs */
#ifndef SOLIDSYSLOGFATFSFILEERRORS_H
#define SOLIDSYSLOGFATFSFILEERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is FatFsFileErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogFatFsFileErrors
    {
        FATFSFILE_ERROR_POOL_EXHAUSTED,
        FATFSFILE_ERROR_UNKNOWN_DESTROY,
        FATFSFILE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a FatFsFile. A handler matches by address
     *  (event->Source == &FatFsFileErrorSource), then reads event->Detail as an
     *  enum SolidSyslogFatFsFileErrors. */
    extern const struct SolidSyslogErrorSource FatFsFileErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFATFSFILEERRORS_H */
