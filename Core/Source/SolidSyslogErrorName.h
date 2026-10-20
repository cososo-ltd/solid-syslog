/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGERRORNAME_H
#define SOLIDSYSLOGERRORNAME_H

#include <stdint.h>

/* The bounded lookup every name table shares: the name at detail, or NULL when
 * detail is outside [0, count). Library-internal; the public functions are the
 * SolidSyslog<Vocabulary>Errors_Name family. */
const char* SolidSyslogErrorName_Lookup(const char* const * names, int32_t count, int32_t detail);

#endif /* SOLIDSYSLOGERRORNAME_H */
