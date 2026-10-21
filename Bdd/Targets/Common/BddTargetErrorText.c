#include "BddTargetErrorText.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"

/* The library's own names, so the targets' output and the documentation agree;
   a fixed word where the library has none, so a step never reads a NULL. */
const char* BddTargetErrorText_Category(uint16_t category)
{
    const char* name = SolidSyslogErrorCategory_Name(category);
    return (name != NULL) ? name : "unknown";
}

const char* BddTargetErrorText_Detail(const struct SolidSyslogErrorEvent* event)
{
    const char* name = SolidSyslogErrorEvent_DetailName(event);
    return (name != NULL) ? name : "-";
}
