#ifndef SOLIDSYSLOGERRORCATEGORY_H
#define SOLIDSYSLOGERRORCATEGORY_H

#include "ExternC.h"

EXTERN_C_BEGIN

    /*
     * Portable error-category axis. Values are library-owned enum constants
     * organised errno-domain style: a low universal-lifecycle range, then one
     * base per role family (declared beside that role's *Definition.h), then a
     * reserved integrator range. The wire type is uint16_t (see
     * SolidSyslogErrorEvent.Category) — not an enum type — so integrator
     * classes can supply their own categories in the reserved range without
     * being boxed into a library enum.
     *
     * Anonymous enums: the constants are what callers use; no tag is needed,
     * which keeps MISRA 2.4 (unused tag) quiet. Role categories use explicit
     * literal values rather than `BASE + n` arithmetic so each initialiser
     * stays within one essential-type category (MISRA 10.4).
     */
    enum
    {
        SOLIDSYSLOG_CAT_NONE = 0x0000,
        SOLIDSYSLOG_CAT_BAD_CONFIG = 0x0001,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED = 0x0002,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY = 0x0003
    };

    /*
     * Per-role base ranges. A role occupies [BASE, BASE + 0xFF]. A base is
     * listed here only once a role family carries a role-specific category;
     * roles that emit only the universal categories above need none. 0xC000
     * and above is reserved for integrator-defined roles.
     */
    enum
    {
        SOLIDSYSLOG_CAT_SENDER_BASE = 0x0100
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGERRORCATEGORY_H */
