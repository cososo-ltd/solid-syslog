#ifndef DISCARDPOLICYANSWER_H
#define DISCARDPOLICYANSWER_H

#include "SolidSyslogDiscardPolicy.h"

/* A discard-policy function that answers whatever policy its context points
 * at, so a test can change the answer between calls. */
// cppcheck-suppress constParameterCallback -- the signature is SolidSyslogDiscardPolicyFunction's.
inline enum SolidSyslogDiscardPolicy AnswersConfiguredPolicy(void* context)
{
    return *static_cast<const enum SolidSyslogDiscardPolicy*>(context);
}

#endif /* DISCARDPOLICYANSWER_H */
