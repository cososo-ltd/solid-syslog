#ifndef SOLIDSYSLOGBUFFERCATEGORIES_H
#define SOLIDSYSLOGBUFFERCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/*
 * Portable Buffer-role error categories. Any Buffer implementation reuses
 * these; a portable handler switch on event->Category reacts to a buffer
 * backend failure identically regardless of the underlying mechanism
 * (POSIX message queue, ring, ...).
 */
#define SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_BUFFER_BASE + 1U))

#endif /* SOLIDSYSLOGBUFFERCATEGORIES_H */
