#ifndef SOLIDSYSLOGSENDERCATEGORIES_H
#define SOLIDSYSLOGSENDERCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/*
 * Portable Sender-role error categories, shared by every Sender that can
 * observe whether a send reached the destination (StreamSender, UdpSender).
 * A portable handler switch on event->Category reacts to a delivery-health
 * transition identically regardless of which sender raised it; event->Source
 * still tells it which transport.
 */
#define SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_SENDER_BASE + 1U))
#define SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED ((uint16_t) (SOLIDSYSLOG_CAT_SENDER_BASE + 2U))

#endif /* SOLIDSYSLOGSENDERCATEGORIES_H */
