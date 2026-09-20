#ifndef LITTLEFSFAKE_H
#define LITTLEFSFAKE_H

#include "SolidSyslogExternC.h"
#include "lfs.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void LittleFsFake_Reset(void);

    /* A mounted filesystem whose cfg->cache_size is what the adapter checks a
     * caller's file buffer against. The storage lives in the fake. */
    lfs_t* LittleFsFake_MountedWithCacheSize(lfs_size_t cacheSize);

SOLIDSYSLOG_EXTERN_C_END

#endif /* LITTLEFSFAKE_H */
