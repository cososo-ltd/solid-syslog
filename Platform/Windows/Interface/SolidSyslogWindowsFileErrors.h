/** @file
 *  Error codes and Source identity for the WindowsFile adapter.
 *
 *  @ingroup platform_windows */
#ifndef SOLIDSYSLOGWINDOWSFILEERRORS_H
#define SOLIDSYSLOGWINDOWSFILEERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is WindowsFileErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogWindowsFileErrors
    {
        WINDOWSFILE_ERROR_POOL_EXHAUSTED,
        WINDOWSFILE_ERROR_UNKNOWN_DESTROY,
        WINDOWSFILE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a WindowsFile. A handler matches by address
     *  (event->Source == &WindowsFileErrorSource), then reads event->Detail as an
     *  enum SolidSyslogWindowsFileErrors. */
    extern const struct SolidSyslogErrorSource WindowsFileErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSFILEERRORS_H */
