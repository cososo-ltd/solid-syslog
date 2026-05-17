/*
 * CRC-16/CCITT-FALSE (ITU-T V.41)
 *   Polynomial: 0x1021  Init: 0xFFFF  RefIn: false  RefOut: false  XorOut: 0x0000
 *
 * Check value 0x29B1 from Greg Cook's CRC catalogue
 *   (reveng.sourceforge.io/crc-catalogue/16.htm, entry crc-16-ibm-3740).
 */

#include "SolidSyslogCrc16.h"

enum
{
    CRC16_CCITT_INIT = 0xFFFFU,
    CRC16_CCITT_POLY = 0x1021U,
    MSB_MASK = 0x8000U
};

uint16_t SolidSyslogCrc16_Compute(const uint8_t* data, uint16_t length)
{
    uint16_t crc = CRC16_CCITT_INIT;

    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= (uint16_t) ((uint16_t) data[i] << 8U);

        for (uint_fast8_t bit = 0; bit < 8U; bit++)
        {
            if ((crc & MSB_MASK) != 0U)
            {
                crc = (uint16_t) ((crc << 1) ^ CRC16_CCITT_POLY);
            }
            else
            {
                crc = (uint16_t) (crc << 1);
            }
        }
    }

    return crc;
}
