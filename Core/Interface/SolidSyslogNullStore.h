#ifndef SOLIDSYSLOGNULLSTORE_H
#define SOLIDSYSLOGNULLSTORE_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogStore* SolidSyslogNullStore_Create(void);
    void                     SolidSyslogNullStore_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGNULLSTORE_H */
