#ifndef SOLIDSYSLOGFILEBLOCKDEVICEPRIVATE_H
#define SOLIDSYSLOGFILEBLOCKDEVICEPRIVATE_H

#include <stdint.h>

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBlockDeviceDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogFileBlockDeviceErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogFile;

/* OpenHandle caches the single SolidSyslogFile the device holds. The handle is
 * re-pointed only when the targeted blockIndex changes; same-block runs reuse
 * it. This is the structural enforcement of the S27.01 single-handle-per-path
 * invariant — by construction the device has exactly one underlying file. */
struct OpenHandle
{
    struct SolidSyslogFile* File;
    size_t BlockIndex;
    bool IsOpen;
};

struct SolidSyslogFileBlockDevice
{
    struct SolidSyslogBlockDevice Base;
    struct OpenHandle Handle;
    const char* PathPrefix;
};

void FileBlockDevice_Initialise(
    struct SolidSyslogBlockDevice* base,
    struct SolidSyslogFile* file,
    const char* pathPrefix
);
void FileBlockDevice_Cleanup(struct SolidSyslogBlockDevice* base);

static inline void FileBlockDevice_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogFileBlockDeviceErrors code
)
{
    SolidSyslog_Error(severity, &FileBlockDeviceErrorSource, category, code);
}

#endif /* SOLIDSYSLOGFILEBLOCKDEVICEPRIVATE_H */
