#ifndef SOLIDSYSLOGPOSIXTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGPOSIXTCPSTREAMPRIVATE_H

#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogPosixTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogPosixTcpStreamConfig Config;
    int Fd;
};

void PosixTcpStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogPosixTcpStreamConfig* config);
void PosixTcpStream_Cleanup(struct SolidSyslogStream* base);

#endif /* SOLIDSYSLOGPOSIXTCPSTREAMPRIVATE_H */
