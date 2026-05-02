#ifndef SOLIDSYSLOGFILESTORE_H
#define SOLIDSYSLOGFILESTORE_H

#include "SolidSyslog.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogStore.h"

#include <stddef.h>
#include <stdint.h>

EXTERN_C_BEGIN

    enum SolidSyslogDiscardPolicy
    {
        SOLIDSYSLOG_DISCARD_OLDEST,
        SOLIDSYSLOG_DISCARD_NEWEST,
        SOLIDSYSLOG_HALT
    };

    typedef void (*SolidSyslogStoreFullCallback)(void* context);

    struct SolidSyslogFileStoreConfig
    {
        struct SolidSyslogFile*           readFile;
        struct SolidSyslogFile*           writeFile;
        const char*                       pathPrefix;
        size_t                            maxFileSize;
        size_t                            maxFiles;
        enum SolidSyslogDiscardPolicy     discardPolicy;
        struct SolidSyslogSecurityPolicy* securityPolicy;
        SolidSyslogStoreFullCallback      onStoreFull;
        void*                             storeFullContext;
    };

    enum
    {
        SOLIDSYSLOG_FILESTORE_STORAGE_SIZE = (sizeof(intptr_t) * 32) + SOLIDSYSLOG_MAX_MESSAGE_SIZE + SOLIDSYSLOG_MAX_INTEGRITY_SIZE + 16
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_FILESTORE_STORAGE_SIZE + sizeof(intptr_t) - 1) / sizeof(intptr_t)];
    } SolidSyslogFileStoreStorage;

    struct SolidSyslogStore* SolidSyslogFileStore_Create(SolidSyslogFileStoreStorage * storage, const struct SolidSyslogFileStoreConfig* config);
    void                     SolidSyslogFileStore_Destroy(struct SolidSyslogStore * store);

EXTERN_C_END

#endif /* SOLIDSYSLOGFILESTORE_H */
