#ifndef SOLIDSYSLOGERRORMESSAGES_H
#define SOLIDSYSLOGERRORMESSAGES_H

#define SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_CONFIG "SolidSyslog_Create called with NULL config"
#define SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_BUFFER "SolidSyslog_Create config.buffer is NULL"
#define SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_SENDER "SolidSyslog_Create config.sender is NULL"
#define SOLIDSYSLOG_ERROR_MSG_CREATE_NULL_STORE "SolidSyslog_Create config.store is NULL"
#define SOLIDSYSLOG_ERROR_MSG_CREATE_ALREADY_INITIALISED "SolidSyslog_Create called while already initialised - call _Destroy first"
#define SOLIDSYSLOG_ERROR_MSG_LOG_NULL_MESSAGE "SolidSyslog_Log called with NULL message"
#define SOLIDSYSLOG_ERROR_MSG_NIL_BUFFER_USED "SolidSyslog_Log called with no buffer configured"
#define SOLIDSYSLOG_ERROR_MSG_NIL_SENDER_USED "SolidSyslog_Service tried to send with no sender configured"

#define SOLIDSYSLOG_ERROR_MSG_UDP_CREATE_NULL_CONFIG "SolidSyslogUdpSender_Create called with NULL config"
#define SOLIDSYSLOG_ERROR_MSG_UDP_CREATE_NULL_RESOLVER "SolidSyslogUdpSender_Create config.resolver is NULL"
#define SOLIDSYSLOG_ERROR_MSG_UDP_CREATE_NULL_DATAGRAM "SolidSyslogUdpSender_Create config.datagram is NULL"
#define SOLIDSYSLOG_ERROR_MSG_UDP_NIL_RESOLVER_USED "SolidSyslogUdpSender_Send tried to resolve with no resolver configured"
#define SOLIDSYSLOG_ERROR_MSG_UDP_NIL_DATAGRAM_USED "SolidSyslogUdpSender_Send tried to open with no datagram configured"

#endif /* SOLIDSYSLOGERRORMESSAGES_H */
