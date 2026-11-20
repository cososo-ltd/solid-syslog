/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the NorFlashBlockDevice. */
#ifndef SOLIDSYSLOGNORFLASHBLOCKDEVICEERRORS_H
#define SOLIDSYSLOGNORFLASHBLOCKDEVICEERRORS_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogNorFlashBlockDeviceErrorSource.
     *  A handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogNorFlashBlockDeviceErrors
    {
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_NULL_FLASH,
        /** The run of blocks asked for is not all within the flash. */
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCKS_OUTSIDE_FLASH,
        /** The block size asked for is more than a flash block holds after the
         *  block's state, or a flash block holds nothing after it. */
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCK_SIZE_UNAVAILABLE,
        /** Bytes read back after a program were not the bytes programmed. Raised
         *  under SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED. */
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_VERIFY_FAILED,
        SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The detail's name, read beside the category's: VERIFY_FAILED for
     *  SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_VERIFY_FAILED. NULL for a code outside the enum. */
    const char* SolidSyslogNorFlashBlockDeviceErrors_Name(int32_t detail);

    /** The identity for events raised by a NorFlashBlockDevice. A handler matches by
     *  address (event->Source == &SolidSyslogNorFlashBlockDeviceErrorSource), then reads
     *  event->Detail as an enum SolidSyslogNorFlashBlockDeviceErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogNorFlashBlockDeviceErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNORFLASHBLOCKDEVICEERRORS_H */
