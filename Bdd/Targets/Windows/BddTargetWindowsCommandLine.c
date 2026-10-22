#include "BddTargetWindowsCommandLine.h"

#include "BddTargetStoreSettings.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum
{
    STORE_OPTION_PREFIX_LENGTH = 2
};

/* Whether a flag can be a store option: it has the dashes a setting's name
   lacks. The shared settings say whether the name is one of theirs. */
static bool IsStoreOption(const char* flag)
{
    return strncmp(flag, "--", STORE_OPTION_PREFIX_LENGTH) == 0;
}

void BddTargetWindowsCommandLine_Parse(int argc, char* argv[], struct BddTargetWindowsOptions* options)
{
    options->Facility = SOLIDSYSLOG_FACILITY_LOCAL0;
    options->Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
    options->Transport = "udp";
    options->MessageId = NULL;
    options->Msg = NULL;
    options->AppName = NULL;
    options->Store = "null";
    options->ErrorsFatal = "1";
    BddTargetStoreSettings_Reset();

    for (int i = 1; i < argc; i++)
    {
        if (((i + 1) < argc) && (strcmp(argv[i], "--facility") == 0))
        {
            options->Facility = (enum SolidSyslogFacility) atoi(argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--severity") == 0))
        {
            options->Severity = (enum SolidSyslogSeverity) atoi(argv[++i]);
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--msgid") == 0))
        {
            options->MessageId = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--message") == 0))
        {
            options->Msg = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--transport") == 0))
        {
            options->Transport = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--app-name") == 0))
        {
            options->AppName = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--errors-fatal") == 0))
        {
            options->ErrorsFatal = argv[++i];
        }
        else if (((i + 1) < argc) && (strcmp(argv[i], "--store") == 0))
        {
            options->Store = argv[++i];
        }
        else if (((i + 1) < argc) && IsStoreOption(argv[i]) &&
                 BddTargetStoreSettings_SetByName(argv[i] + STORE_OPTION_PREFIX_LENGTH, argv[i + 1]))
        {
            /* A store option whose value the shared settings took; the flag
               is the setting's name after the dashes. A value they refused is
               ignored like any unknown flag, and its text is looked at next. */
            i++;
        }
        else if (strcmp(argv[i], "--halt-exit") == 0)
        {
            (void) BddTargetStoreSettings_SetByName("halt-exit", "1");
        }
        else if (strcmp(argv[i], "--no-sd") == 0)
        {
            (void) BddTargetStoreSettings_SetByName("no-sd", "1");
        }
    }
}
