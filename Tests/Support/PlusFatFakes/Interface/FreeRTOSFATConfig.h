/* Host-test FreeRTOS-Plus-FAT configuration for Tests/PlusFat/.
 *
 * Plus-FAT is header-configured: ff_headers.h includes this file (after
 * FreeRTOS.h) and FreeRTOSFATConfigDefaults.h fills in every value not set
 * here. The three settings below are the minimum the defaults header demands
 * — byte order, a thread-local-storage slot for the CWD, and a portINLINE
 * fallback for the host compiler — matching Plus-FAT's own DefaultConf sample.
 *
 * Note: configNUM_THREAD_LOCAL_STORAGE_POINTERS (which ff_stdio.h requires to
 * be >= 3) is a kernel setting and so lives in FreeRtosFakes/FreeRTOSConfig.h,
 * which is included earlier via FreeRTOS.h. */
#ifndef FREERTOSFATCONFIG_H
#define FREERTOSFATCONFIG_H

#define ffconfigBYTE_ORDER (pdFREERTOS_LITTLE_ENDIAN)
#define ffconfigCWD_THREAD_LOCAL_INDEX (0)

#if !defined(portINLINE)
#define portINLINE __inline
#endif

#endif /* FREERTOSFATCONFIG_H */
