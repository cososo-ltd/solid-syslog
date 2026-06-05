#include "SolidSyslog.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMessageFormatter.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrivate.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStringFunction.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTunables.h"

const struct SolidSyslogErrorSource SolidSyslogErrorSource = {"SolidSyslog"};

struct SolidSyslogBuffer;
struct SolidSyslogFormatter;
struct SolidSyslogSender;
struct SolidSyslogStore;
struct SolidSyslogStructuredData;

static inline void SolidSyslog_DrainBufferIntoStore(struct SolidSyslog* self);
static inline void SolidSyslog_SendOneFromStore(struct SolidSyslog* self);
static void SolidSyslog_InstallAppName(struct SolidSyslog* self, SolidSyslogStringFunction configured);
static void SolidSyslog_InstallBuffer(struct SolidSyslog* self, struct SolidSyslogBuffer* configured);
static void SolidSyslog_InstallClock(struct SolidSyslog* self, SolidSyslogClockFunction configured);
static void SolidSyslog_InstallHostname(struct SolidSyslog* self, SolidSyslogStringFunction configured);
static void SolidSyslog_InstallProcessId(struct SolidSyslog* self, SolidSyslogStringFunction configured);
static void SolidSyslog_InstallSender(struct SolidSyslog* self, struct SolidSyslogSender* configured);
static void SolidSyslog_InstallStore(struct SolidSyslog* self, struct SolidSyslogStore* configured);
static void SolidSyslog_InstallStructuredData(
    struct SolidSyslog* self,
    struct SolidSyslogStructuredData** configured,
    size_t count
);
static inline bool SolidSyslog_IsServiceEnabled(struct SolidSyslog* self);
static void SolidSyslog_ProcessMessages(struct SolidSyslog* self);
static void SolidSyslog_ResetToDefaults(struct SolidSyslog* self);

void SolidSyslog_Initialise(struct SolidSyslog* self, const struct SolidSyslogConfig* config)
{
    SolidSyslog_ResetToDefaults(self);
    SolidSyslog_InstallBuffer(self, config->Buffer);
    SolidSyslog_InstallSender(self, config->Sender);
    SolidSyslog_InstallStore(self, config->Store);
    SolidSyslog_InstallClock(self, config->Clock);
    SolidSyslog_InstallHostname(self, config->GetHostname);
    SolidSyslog_InstallAppName(self, config->GetAppName);
    SolidSyslog_InstallProcessId(self, config->GetProcessId);
    SolidSyslog_InstallStructuredData(self, config->Sd, config->SdCount);
}

void SolidSyslog_Cleanup(struct SolidSyslog* self)
{
    /* Reset to safe defaults so a stale-handle Log/Service after Destroy is a
     * silent no-op rather than a NULL-fn-pointer crash. The slot is then
     * re-claimable by the next _Create. */
    SolidSyslog_ResetToDefaults(self);
}

static void SolidSyslog_ResetToDefaults(struct SolidSyslog* self)
{
    self->Buffer = SolidSyslogNullBuffer_Get();
    self->Sender = SolidSyslogNullSender_Get();
    self->Store = SolidSyslogNullStore_Get();
    self->Format.Clock = SolidSyslog_NullClock;
    self->Format.GetHostname = SolidSyslog_NullStringFunction;
    self->Format.GetAppName = SolidSyslog_NullStringFunction;
    self->Format.GetProcessId = SolidSyslog_NullStringFunction;
    self->Format.Sd = NULL;
    self->Format.SdCount = 0;
}

static void SolidSyslog_InstallBuffer(struct SolidSyslog* self, struct SolidSyslogBuffer* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_ERROR_CREATE_NULL_BUFFER
        );
    }
    else
    {
        self->Buffer = configured;
    }
}

static void SolidSyslog_InstallSender(struct SolidSyslog* self, struct SolidSyslogSender* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_ERROR_CREATE_NULL_SENDER
        );
    }
    else
    {
        self->Sender = configured;
    }
}

static void SolidSyslog_InstallStore(struct SolidSyslog* self, struct SolidSyslogStore* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_ERROR_CREATE_NULL_STORE
        );
    }
    else
    {
        self->Store = configured;
    }
}

static void SolidSyslog_InstallClock(struct SolidSyslog* self, SolidSyslogClockFunction configured)
{
    if (configured != NULL)
    {
        self->Format.Clock = configured;
    }
}

static void SolidSyslog_InstallHostname(struct SolidSyslog* self, SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        self->Format.GetHostname = configured;
    }
}

static void SolidSyslog_InstallAppName(struct SolidSyslog* self, SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        self->Format.GetAppName = configured;
    }
}

static void SolidSyslog_InstallProcessId(struct SolidSyslog* self, SolidSyslogStringFunction configured)
{
    if (configured != NULL)
    {
        self->Format.GetProcessId = configured;
    }
}

static void SolidSyslog_InstallStructuredData(
    struct SolidSyslog* self,
    struct SolidSyslogStructuredData** configured,
    size_t count
)
{
    if ((configured == NULL) && (count > 0U))
    {
        /* Inconsistent pairing — the formatter would dereference Sd[i] for
         * i < SdCount against a NULL array. Report and leave the reset
         * defaults (no SD) in place so Log() degrades safely. */
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_ERROR_CREATE_INCONSISTENT_SD
        );
    }
    else
    {
        self->Format.Sd = configured;
        self->Format.SdCount = count;
    }
}

void SolidSyslog_Service(struct SolidSyslog* handle)
{
    if (handle == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_ERROR_SERVICE_NULL_HANDLE
        );
    }
    else if (SolidSyslog_IsServiceEnabled(handle))
    {
        SolidSyslog_ProcessMessages(handle);
    }
    else
    {
        /* Halted store — skip drain/send. */
    }
}

static inline bool SolidSyslog_IsServiceEnabled(struct SolidSyslog* self)
{
    return !SolidSyslogStore_IsHalted(self->Store);
}

static void SolidSyslog_ProcessMessages(struct SolidSyslog* self)
{
    SolidSyslog_DrainBufferIntoStore(self);
    SolidSyslog_SendOneFromStore(self);
}

/* Eagerly drain the buffer so the producer-side shock absorber stays small while
 * the sender is slow or down — overflow then engages the store's discard policy
 * rather than silently dropping at the buffer. The fall-through to a direct
 * Sender_Send on Store_Write rejection is *only* taken when the store is
 * transient (NullStore): a NullStore Write rejection means "I never retain
 * anything, please try the sender." For a real BlockStore, rejection is the
 * discard policy speaking — letting that message escape via direct send would
 * break the discard-newest contract (a newer message would bypass older stored
 * ones once the sender recovered). */
static inline void SolidSyslog_DrainBufferIntoStore(struct SolidSyslog* self)
{
    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t len = 0;

    while (SolidSyslogBuffer_Read(self->Buffer, buf, sizeof(buf), &len))
    {
        if (!SolidSyslogStore_Write(self->Store, buf, len) && SolidSyslogStore_IsTransient(self->Store))
        {
            (void) SolidSyslogSender_Send(self->Sender, buf, len);
        }
    }
}

static inline void SolidSyslog_SendOneFromStore(struct SolidSyslog* self)
{
    char buf[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t len = 0;

    if (SolidSyslogStore_ReadNextUnsent(self->Store, buf, sizeof(buf), &len) &&
        SolidSyslogSender_Send(self->Sender, buf, len))
    {
        SolidSyslogStore_MarkSent(self->Store);
    }
}

void SolidSyslog_Log(struct SolidSyslog* handle, const struct SolidSyslogMessage* message)
{
    if (handle == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_ERROR_LOG_NULL_HANDLE
        );
    }
    else if (message == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_ERROR_LOG_NULL_MESSAGE
        );
    }
    else
    {
        SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
        struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE);

        SolidSyslogMessageFormatter_Format(f, message, &handle->Format);
        SolidSyslogBuffer_Write(
            handle->Buffer,
            SolidSyslogFormatter_AsFormattedBuffer(f),
            SolidSyslogFormatter_Length(f)
        );
    }
}

void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts)
{
    (void) ts;
}

void SolidSyslog_NullStringFunction(struct SolidSyslogFormatter* formatter)
{
    (void) formatter;
}
