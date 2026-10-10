/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* Starts the SolidSyslog BDD target at the end of usrRoot, in place of the
 * user-application hook. */
Component INCLUDE_SOLIDSYSLOG_VXWORKS64_BDD {
    NAME        SolidSyslog VxWorks 6.4 BDD target
    SYNOPSIS    Starts the SolidSyslog BDD target.
    _CHILDREN   FOLDER_APPLICATION
    PROTOTYPE   void BddTargetVxWorks64_Init (void);
    INIT_RTN    BddTargetVxWorks64_Init ();
    _INIT_ORDER usrRoot
}
