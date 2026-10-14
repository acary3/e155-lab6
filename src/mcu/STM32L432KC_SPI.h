/**
    SPI1 driver for the STM32L432KC, written against the CMSIS device
    templates in stm32l432xx.h (register structs + bit-field macros).
    @file STM32L432KC_SPI.h
*/

#ifndef STM32L4_SPI_H
#define STM32L4_SPI_H

#include <stdint.h>
#include <stm32l432xx.h>
#include "STM32L432KC_GPIO.h"

///////////////////////////////////////////////////////////////////////////////
// Pin assignments: SPI1 on AF5 of PB3/PB4/PB5, chip enable on GPIO PB1
///////////////////////////////////////////////////////////////////////////////

#define SPI_SCK  PB3   // SPI1_SCK  -> DS1722 SCLK (pin 3)   header 16
#define SPI_SDI  PB4   // SPI1_MISO <- DS1722 SDO  (pin 5)   header 15
#define SPI_SDO  PB5   // SPI1_MOSI -> DS1722 SDI  (pin 6)   header 14
#define SPI_CE   PB1   // GPIO      -> DS1722 CE   (pin 2)   header 9, ACTIVE HIGH

// Baud-rate divisor field values: f_SCK = f_PCLK2 / 2^(BR+1)
#define SPI_BR_DIV2    0b000
#define SPI_BR_DIV4    0b001
#define SPI_BR_DIV8    0b010
#define SPI_BR_DIV16   0b011
#define SPI_BR_DIV32   0b100
#define SPI_BR_DIV64   0b101
#define SPI_BR_DIV128  0b110
#define SPI_BR_DIV256  0b111

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
///////////////////////////////////////////////////////////////////////////////

/* Enables the SPI1 peripheral as an 8-bit, MSB-first, full-duplex master.
 *    br   - baud-rate divisor (SPI_BR_DIVx), f_SCK = f_PCLK2 / 2^(br+1)
 *    cpol - clock idle level (0 = idle low, 1 = idle high)
 *    cpha - 0 = sample on first edge, 1 = sample on second edge
 * Chip enable is driven in software on SPI_CE (initialized LOW). */
void initSPI(int br, int cpol, int cpha);

/* Transmits one byte and returns the byte received at the same time.
 * Blocks until the transfer has completely finished (BSY cleared). */
char spiSendReceive(char send);

#endif
