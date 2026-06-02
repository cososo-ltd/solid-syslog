#ifndef SOLIDSYSLOGERRORCATEGORY_H
#define SOLIDSYSLOGERRORCATEGORY_H

#include <stdint.h>

/*
 * Portable error-category axis. Categories are library-owned constants
 * organised errno-domain style: a low universal-lifecycle range, then one
 * base per role family (declared beside that role's *Definition.h, off the
 * bases below as BASE + n), then a reserved integrator range. 0xC000 and
 * above is reserved for integrator-defined roles so custom backends never
 * collide.
 *
 * The category constants carry their own (uint16_t) cast so emit sites stay
 * clean — SolidSyslog_Error's category parameter is uint16_t (not an enum
 * type), which is what lets integrator classes supply their own categories
 * in the reserved range without being boxed into a library enum.
 */

/* Universal lifecycle categories, available to every source. */
#define SOLIDSYSLOG_CAT_BAD_CONFIG ((uint16_t) 0x0001U)
#define SOLIDSYSLOG_CAT_BAD_ARGUMENT ((uint16_t) 0x0002U)
#define SOLIDSYSLOG_CAT_POOL_EXHAUSTED ((uint16_t) 0x0003U)
#define SOLIDSYSLOG_CAT_UNKNOWN_DESTROY ((uint16_t) 0x0004U)

/*
 * Per-role base ranges. A role occupies [BASE, BASE + 0xFF]. A base is listed
 * here only once a role family carries a role-specific category; roles that
 * emit only the universal categories above need none. (0x0100 Sender and
 * 0x0300 Stream are intentionally unallocated — those roles emit only
 * universal categories today.)
 */
#define SOLIDSYSLOG_CAT_RESOLVER_BASE ((uint16_t) 0x0200U)
#define SOLIDSYSLOG_CAT_TLSSTREAM_BASE ((uint16_t) 0x0400U)
#define SOLIDSYSLOG_CAT_SECURITYPOLICY_BASE ((uint16_t) 0x0500U)
#define SOLIDSYSLOG_CAT_BUFFER_BASE ((uint16_t) 0x0600U)

#endif /* SOLIDSYSLOGERRORCATEGORY_H */
