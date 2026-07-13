/** @file
 *  CRC-16/CCITT-FALSE checksum (poly 0x1021, init 0xFFFF, no reflection, no
 *  final XOR; a.k.a. CRC-16/IBM-3740, check value 0x29B1). A pure function over
 *  a byte range — no state, no lifecycle. Used by SolidSyslogCrc16Policy for an
 *  unkeyed at-rest integrity trailer. */
#ifndef SOLIDSYSLOGCRC16_H
#define SOLIDSYSLOGCRC16_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    /** Compute the CRC-16/CCITT-FALSE checksum (poly 0x1021, init 0xFFFF, no
     *  reflection, no final XOR; a.k.a. CRC-16/IBM-3740, check value 0x29B1)
     *  over @p data[0 .. length). */
    uint16_t SolidSyslogCrc16_Compute(const uint8_t* data, uint16_t length);

EXTERN_C_END

#endif /* SOLIDSYSLOGCRC16_H */
