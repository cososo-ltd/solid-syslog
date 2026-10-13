#ifndef BDDTARGETKEY_H
#define BDDTARGETKEY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* DEMO KEY ONLY. A real integrator supplies key material from a secure
       element, a KDF, or encrypted NVM via their own SolidSyslogKeyFunction -
       never a hard-coded constant. This exists so the BDD scenarios can exercise
       the keyed at-rest policies end-to-end with real crypto, and take the key
       away with `set key withheld` to show what the library does without it. */
    void BddTargetKey_Reset(void);
    bool BddTargetKey_SetByName(const char* name, const char* value);
    /* A SolidSyslogKeyFunction. */
    bool BddTargetKey_Get(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETKEY_H */
