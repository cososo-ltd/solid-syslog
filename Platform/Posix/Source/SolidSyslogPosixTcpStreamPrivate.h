#ifndef SOLIDSYSLOGPOSIXTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGPOSIXTCPSTREAMPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogPosixTcpStreamErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogPosixTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogPosixTcpStreamConfig Config;
    int Fd;
};

void PosixTcpStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogPosixTcpStreamConfig* config);
void PosixTcpStream_Cleanup(struct SolidSyslogStream* base);

static inline void PosixTcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPosixTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &PosixTcpStreamErrorSource, category, code);
}

#endif /* SOLIDSYSLOGPOSIXTCPSTREAMPRIVATE_H */
