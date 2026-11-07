#include "BddTargetCommandLine.h"

#include "BddTargetStoreSettings.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
    OPT_MAX_BLOCKS = 256,
    OPT_MAX_BLOCK_SIZE = 257,
    OPT_DISCARD_POLICY = 258,
    OPT_NO_SD = 259,
    OPT_HALT_EXIT = 260,
    OPT_CAPACITY_THRESHOLD = 261,
    OPT_APP_NAME = 262,
    OPT_SECURITY_POLICY = 263,
    OPT_ERRORS_FATAL = 264,
    OPT_NO_REPORTING = 265,
    OPT_BUFFER = 266
};

static bool IsAStoreBehindAPassthroughBuffer(const struct BddTargetOptions* options);

static bool IsValidErrorsFatal(const char* value)
{
    return (strcmp(value, "0") == 0) || (strcmp(value, "1") == 0);
}

/* Takes @p value into @p field when it is one the option accepts. Returns 0
 * when taken, 1 when not - the parse's own result. */
static int SetIfValid(bool valid, const char* value, const char** field)
{
    if (valid)
    {
        *field = value;
    }
    return valid ? 0 : 1;
}

/* Hands a store option to the shared settings, which know it by the long
 * option's own name. Returns 0 when taken, 1 when not - the parse's own
 * result. */
static int SetStoreSetting(const char* name, const char* value)
{
    return BddTargetStoreSettings_SetByName(name, value) ? 0 : 1;
}

/* Applies one parsed option to `options`; `name` is the long option's name,
 * or NULL for a short one. Returns 0 on success, 1 if the option's argument
 * is invalid (which fails the whole parse). Split out of
 * BddTargetCommandLine_Parse to keep the loop a thin driver and the per-option
 * logic in one place. */
static int ApplyOption(int opt, const char* name, const char* value, struct BddTargetOptions* options)
{
    int result = 0;
    switch (opt)
    {
        case 'f':
            options->Facility = (enum SolidSyslogFacility) atoi(value);
            break;
        case 's':
            options->Severity = (enum SolidSyslogSeverity) atoi(value);
            break;
        case 'i':
            options->MessageId = value;
            break;
        case 'm':
            options->Msg = value;
            break;
        case 't':
            if ((strcmp(value, "udp") != 0) && (strcmp(value, "tcp") != 0) && (strcmp(value, "tls") != 0) &&
                (strcmp(value, "mtls") != 0))
            {
                result = 1;
            }
            else
            {
                options->Transport = value;
            }
            break;
        case 'o':
            if ((strcmp(value, "null") != 0) && (strcmp(value, "file") != 0))
            {
                result = 1;
            }
            else
            {
                options->Store = value;
            }
            break;
        case OPT_BUFFER:
            if ((strcmp(value, "queue") != 0) && (strcmp(value, "passthrough") != 0))
            {
                result = 1;
            }
            else
            {
                options->Buffer = value;
            }
            break;
        case OPT_MAX_BLOCKS:
        case OPT_MAX_BLOCK_SIZE:
        case OPT_DISCARD_POLICY:
        case OPT_SECURITY_POLICY:
        case OPT_CAPACITY_THRESHOLD:
            result = SetStoreSetting(name, value);
            break;
        case OPT_NO_SD:
        case OPT_HALT_EXIT:
        case OPT_NO_REPORTING:
            /* Bare flags; the shared settings take every value as a number. */
            result = SetStoreSetting(name, "1");
            break;
        case OPT_APP_NAME:
            options->AppName = value;
            break;
        case OPT_ERRORS_FATAL:
            result = SetIfValid(IsValidErrorsFatal(value), value, &options->ErrorsFatal);
            break;
        default:
            result = 1;
            break;
    }
    return result;
}

int BddTargetCommandLine_Parse(int argc, char* argv[], struct BddTargetOptions* options)
{
    options->Facility = SOLIDSYSLOG_FACILITY_LOCAL0;
    options->Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
    options->MessageId = NULL;
    options->Msg = NULL;
    options->AppName = NULL;
    options->Transport = "udp";
    options->Store = "null";
    options->Buffer = "queue";
    options->ErrorsFatal = "1";
    BddTargetStoreSettings_Reset();

    static struct option longOptions[] = {
        {"facility", required_argument, NULL, 'f'},
        {"severity", required_argument, NULL, 's'},
        {"msgid", required_argument, NULL, 'i'},
        {"message", required_argument, NULL, 'm'},
        {"transport", required_argument, NULL, 't'},
        {"store", required_argument, NULL, 'o'},
        {"buffer", required_argument, NULL, OPT_BUFFER},
        {"max-blocks", required_argument, NULL, OPT_MAX_BLOCKS},
        {"max-block-size", required_argument, NULL, OPT_MAX_BLOCK_SIZE},
        {"discard-policy", required_argument, NULL, OPT_DISCARD_POLICY},
        {"security-policy", required_argument, NULL, OPT_SECURITY_POLICY},
        {"capacity-threshold", required_argument, NULL, OPT_CAPACITY_THRESHOLD},
        {"no-sd", no_argument, NULL, OPT_NO_SD},
        {"halt-exit", no_argument, NULL, OPT_HALT_EXIT},
        {"no-reporting", no_argument, NULL, OPT_NO_REPORTING},
        {"app-name", required_argument, NULL, OPT_APP_NAME},
        {"errors-fatal", required_argument, NULL, OPT_ERRORS_FATAL},
        {NULL, 0, NULL, 0},
    };

    int opt = 0;
    int result = 0;
    int longIndex = -1;
    while ((result == 0) && ((opt = getopt_long(argc, argv, "f:s:i:m:t:o:", longOptions, &longIndex)) != -1))
    {
        /* getopt_long leaves longIndex alone for a short option. */
        const char* name = (longIndex >= 0) ? longOptions[longIndex].name : NULL;
        result = ApplyOption(opt, name, optarg, options);
        longIndex = -1;
    }

    if ((result == 0) && IsAStoreBehindAPassthroughBuffer(options))
    {
        result = 1;
    }

    return result;
}

/* A passthrough buffer hands every record straight to the sender, so a store
 * behind it would hold only the logger's reports while the application's
 * records still skipped it - a wiring that looks retained and is not. */
static bool IsAStoreBehindAPassthroughBuffer(const struct BddTargetOptions* options)
{
    return (strcmp(options->Buffer, "passthrough") == 0) && (strcmp(options->Store, "file") == 0);
}
