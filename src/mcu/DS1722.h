/**
    DS1722 SPI digital thermometer driver (uses STM32L432KC_SPI).
    @file DS1722.h
*/

#ifndef DS1722_H
#define DS1722_H

#include <stdint.h>
#include "STM32L432KC_SPI.h"

// Register addresses 
#define DS1722_CFG_READ    0x00
//new
#define DS1722_CFG_WRITE   0x80
#define DS1722_TEMP_LSB    0x01
#define DS1722_TEMP_MSB    0x02

// Configuration register: 1 1 1 1SHOT R2 R1 R0 SD
#define DS1722_CFG_BASE    0xE0
#define DS1722_CFG_RES(r)  ((uint8_t)(((r) & 0x7) << 1))   // r = bits - 8

/* Writes the configuration register for continuous conversion
 * at the given resolution (8..12 bits). Values outside are clamped. */
void ds1722SetResolution(int bits);

/* Reads back the configuration register. */
uint8_t ds1722ReadConfig(void);

/* Reads the temperature registers in one burst */
int16_t ds1722ReadRaw(void);

/* Conversion time in ms for a given resolution (datasheet max values). */
uint32_t ds1722ConversionMs(int bits);

#endif
