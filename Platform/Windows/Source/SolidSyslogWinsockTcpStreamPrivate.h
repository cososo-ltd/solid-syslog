#ifndef SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H

#include <stdint.h>

#include <winsock2.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogWinsockTcpStream.h"
#include "SolidSyslogWinsockTcpStreamErrors.h"

struct SolidSyslogWinsockTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogWinsockTcpStreamConfig Config;
    SOCKET Fd;
};

void WinsockTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogWinsockTcpStreamConfig* config
);
void WinsockTcpStream_Cleanup(struct SolidSyslogStream* base);

static inline void WinsockTcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWinsockTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &WinsockTcpStreamErrorSource, category, code);
}

#endif /* SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H */
