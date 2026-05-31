#include "BddTargetCommandLine.h"

#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
    DEFAULT_MAX_BLOCKS = 10,
    DEFAULT_MAX_BLOCK_SIZE = 65536,
    OPT_MAX_BLOCKS = 256,
    OPT_MAX_BLOCK_SIZE = 257,
    OPT_DISCARD_POLICY = 258,
    OPT_NO_SD = 259,
    OPT_HALT_EXIT = 260,
    OPT_CAPACITY_THRESHOLD = 261,
    OPT_APP_NAME = 262,
    OPT_SECURITY_POLICY = 263
};

static bool ParsePositiveNumber(const char* str, size_t* result)
{
    char* end = NULL;
    long value = strtol(str, &end, 10);

    if ((*str == '\0') || (*end != '\0') || (value < 0))
    {
        return false;
    }

    *result = (size_t) value;
    return true;
}

static bool IsValidDiscardPolicy(const char* policy)
{
    return (strcmp(policy, "oldest") == 0) || (strcmp(policy, "newest") == 0) || (strcmp(policy, "halt") == 0);
}

static bool IsValidSecurityPolicy(const char* policy)
{
    return (strcmp(policy, "crc16") == 0) || (strcmp(policy, "hmac-sha256") == 0) || (strcmp(policy, "null") == 0);
}

/* Applies one parsed option to `options`. Returns 0 on success, 1 if the
 * option's argument is invalid (which fails the whole parse). Split out of
 * _Parse to keep the loop a thin driver and the per-option logic in one place. */
static int ApplyOption(int opt, const char* value, struct BddTargetOptions* options)
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
        case OPT_MAX_BLOCKS:
            result = ParsePositiveNumber(value, &options->MaxBlocks) ? 0 : 1;
            break;
        case OPT_MAX_BLOCK_SIZE:
            result = ParsePositiveNumber(value, &options->MaxBlockSize) ? 0 : 1;
            break;
        case OPT_DISCARD_POLICY:
            result = IsValidDiscardPolicy(value) ? 0 : 1;
            if (result == 0)
            {
                options->DiscardPolicy = value;
            }
            break;
        case OPT_SECURITY_POLICY:
            result = IsValidSecurityPolicy(value) ? 0 : 1;
            if (result == 0)
            {
                options->SecurityPolicy = value;
            }
            break;
        case OPT_CAPACITY_THRESHOLD:
            result = ParsePositiveNumber(value, &options->CapacityThreshold) ? 0 : 1;
            break;
        case OPT_NO_SD:
            options->NoSd = true;
            break;
        case OPT_HALT_EXIT:
            options->HaltExit = true;
            break;
        case OPT_APP_NAME:
            options->AppName = value;
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
    options->MaxBlocks = DEFAULT_MAX_BLOCKS;
    options->MaxBlockSize = DEFAULT_MAX_BLOCK_SIZE;
    options->DiscardPolicy = "oldest";
    options->SecurityPolicy = "crc16";
    options->CapacityThreshold = 0;
    options->NoSd = false;
    options->HaltExit = false;

    static struct option longOptions[] = {
        {"facility", required_argument, NULL, 'f'},
        {"severity", required_argument, NULL, 's'},
        {"msgid", required_argument, NULL, 'i'},
        {"message", required_argument, NULL, 'm'},
        {"transport", required_argument, NULL, 't'},
        {"store", required_argument, NULL, 'o'},
        {"max-blocks", required_argument, NULL, OPT_MAX_BLOCKS},
        {"max-block-size", required_argument, NULL, OPT_MAX_BLOCK_SIZE},
        {"discard-policy", required_argument, NULL, OPT_DISCARD_POLICY},
        {"security-policy", required_argument, NULL, OPT_SECURITY_POLICY},
        {"capacity-threshold", required_argument, NULL, OPT_CAPACITY_THRESHOLD},
        {"no-sd", no_argument, NULL, OPT_NO_SD},
        {"halt-exit", no_argument, NULL, OPT_HALT_EXIT},
        {"app-name", required_argument, NULL, OPT_APP_NAME},
        {NULL, 0, NULL, 0},
    };

    int opt = 0;
    int result = 0;
    while ((result == 0) && ((opt = getopt_long(argc, argv, "f:s:i:m:t:o:", longOptions, NULL)) != -1))
    {
        result = ApplyOption(opt, optarg, options);
    }

    return result;
}
