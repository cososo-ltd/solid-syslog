/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogErrorCategory.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogDatagramCategories.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogStreamCategories.h"
#include "SolidSyslogTlsStreamCategories.h"

/* A switch rather than a table: the values are sparse. */
const char* SolidSyslogErrorCategory_Name(uint16_t category)
{
    const char* name = NULL;
    switch (category)
    {
        case SOLIDSYSLOG_CAT_BAD_CONFIG:
            name = "BAD_CONFIG";
            break;
        case SOLIDSYSLOG_CAT_BAD_ARGUMENT:
            name = "BAD_ARGUMENT";
            break;
        case SOLIDSYSLOG_CAT_POOL_EXHAUSTED:
            name = "POOL_EXHAUSTED";
            break;
        case SOLIDSYSLOG_CAT_UNKNOWN_DESTROY:
            name = "UNKNOWN_DESTROY";
            break;
        case SOLIDSYSLOG_CAT_NATIVE_ERROR:
            name = "NATIVE_ERROR";
            break;
        case SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED:
            name = "SENDER_DELIVERY_FAILED";
            break;
        case SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED:
            name = "SENDER_DELIVERY_RESTORED";
            break;
        case SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED:
            name = "RESOLVER_RESOLVE_FAILED";
            break;
        case SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED:
            name = "STREAM_CONNECT_FAILED";
            break;
        case SOLIDSYSLOG_CAT_STREAM_OPTION_REFUSED:
            name = "STREAM_OPTION_REFUSED";
            break;
        case SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED:
            name = "TLS_STREAM_INIT_FAILED";
            break;
        case SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED:
            name = "TLS_STREAM_HANDSHAKE_FAILED";
            break;
        case SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE:
            name = "SECURITY_POLICY_KEY_UNAVAILABLE";
            break;
        case SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED:
            name = "SECURITY_POLICY_SEAL_FAILED";
            break;
        case SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED:
            name = "SECURITY_POLICY_OPEN_FAILED";
            break;
        case SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED:
            name = "BUFFER_BACKEND_FAILED";
            break;
        case SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST:
            name = "BUFFER_RECORDS_LOST";
            break;
        case SOLIDSYSLOG_CAT_FILE_IO_FAILED:
            name = "FILE_IO_FAILED";
            break;
        case SOLIDSYSLOG_CAT_STORE_WRITE_FAILED:
            name = "STORE_WRITE_FAILED";
            break;
        case SOLIDSYSLOG_CAT_STORE_FROZEN:
            name = "STORE_FROZEN";
            break;
        case SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED:
            name = "STORE_RECORD_DISCARDED";
            break;
        case SOLIDSYSLOG_CAT_STORE_RECORDS_LOST:
            name = "STORE_RECORDS_LOST";
            break;
        case SOLIDSYSLOG_CAT_DATAGRAM_NEXT_HOP_UNRESOLVED:
            name = "DATAGRAM_NEXT_HOP_UNRESOLVED";
            break;
        default:
            /* Unassigned, or in the integrator range. */
            break;
    }
    return name;
}
