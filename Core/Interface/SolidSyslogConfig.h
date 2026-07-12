#ifndef SOLIDSYSLOGCONFIG_H
#define SOLIDSYSLOGCONFIG_H

#include <stddef.h>

#include "SolidSyslogHeaderFieldFunction.h"
#include "SolidSyslogTimestamp.h"
#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslog;
    struct SolidSyslogBuffer;
    struct SolidSyslogHeaderField;
    struct SolidSyslogSender;
    struct SolidSyslogStore;
    struct SolidSyslogStructuredData;

    /** Wiring for SolidSyslog_Create. NULL is safe for any field: Buffer,
     *  Sender, and Store fall back to their Null object and report an error
     *  (a no-op unless an error handler is installed; inject the Null* object
     *  instead to stay silent), while the callbacks fall back to nil defaults
     *  with no error. Whatever you do provide must outlive the created handle;
     *  the config struct itself is only read during Create and may be transient. */
    struct SolidSyslogConfig
    {
        struct SolidSyslogBuffer* Buffer; /**< Also sets Log's blocking behaviour (inline vs enqueued). */
        struct SolidSyslogSender* Sender;
        SolidSyslogClockFunction Clock; /**< NULL leaves timestamps at the RFC 5424 nil value. */
        SolidSyslogHeaderFieldFunction GetHostname; /**< RFC 5424 HOSTNAME; NULL emits "-". */
        void* GetHostnameContext; /**< Passed back to the callback unchanged. */
        SolidSyslogHeaderFieldFunction GetAppName; /**< RFC 5424 APP-NAME; NULL emits "-". */
        void* GetAppNameContext; /**< Passed back to the callback unchanged. */
        SolidSyslogHeaderFieldFunction GetProcessId; /**< RFC 5424 PROCID; NULL emits "-". */
        void* GetProcessIdContext; /**< Passed back to the callback unchanged. */
        struct SolidSyslogStore* Store; /**< NULL (or SolidSyslogNullStore_Get()) means no store-and-forward. */
        struct SolidSyslogStructuredData** Sd; /**< Per-instance SD-ELEMENTs on every message, before per-message SD. */
        size_t SdCount;
    };

    /** Create a logger from @p config. Never returns NULL: on a NULL config or
     *  an exhausted instance pool it reports via SolidSyslog_Error and returns a
     *  shared null instance whose Log/Service silently drop, so callers need not
     *  null-check the result. */
    struct SolidSyslog* SolidSyslog_Create(const struct SolidSyslogConfig* config);

    /** Release the logger's pool slot. Does not free the injected Buffer,
     *  Sender, Store, or Sd objects; the caller owns those. A NULL, foreign, or
     *  already-destroyed handle is reported via SolidSyslog_Error and ignored. */
    void SolidSyslog_Destroy(struct SolidSyslog * handle);

EXTERN_C_END

#endif /* SOLIDSYSLOGCONFIG_H */
