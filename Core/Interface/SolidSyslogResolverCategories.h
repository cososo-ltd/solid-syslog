#ifndef SOLIDSYSLOGRESOLVERCATEGORIES_H
#define SOLIDSYSLOGRESOLVERCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/*
 * Portable Resolver-role error categories. Any Resolver implementation
 * reuses these; a portable handler switch on event->Category works
 * identically across every resolver backend.
 */
#define SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_RESOLVER_BASE + 1U))

#endif /* SOLIDSYSLOGRESOLVERCATEGORIES_H */
