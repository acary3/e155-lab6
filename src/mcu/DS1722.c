/**
    DS1722 SPI digital thermometer driver.
    SPI mode: CE active HIGH, data shifted MSB first, CPHA = 1.
    (The DS1722 latches the SCLK idle level when CE rises, so CPOL 0 or 1 works;
    this driver uses CPOL = 0.)
    @file DS1722.c
*/

#include "DS1722.h"

// Short busy-wait, satisfies CE-to-SCLK setup/hold and CE inactive time
static void ceDelay(void) {
    for (volatile int i = 0; i < 20; i++);
}

static void ceAssert(void)   { digitalWrite(SPI_CE, PIO_HIGH); ceDelay(); }
static void ceDeassert(void) { ceDelay(); digitalWrite(SPI_CE, PIO_LOW); ceDelay(); }

void ds1722SetResolution(int bits) {
    if (bits < 8)  bits = 8;
    if (bits > 12) bits = 12;

    uint8_t cfg = DS1722_CFG_BASE | DS1722_CFG_RES(bits - 8);   

    ceAssert();
    spiSendReceive(DS1722_CFG_WRITE);
    spiSendReceive(cfg);
    ceDeassert();
}

uint8_t ds1722ReadConfig(void) {
    ceAssert();
    spiSendReceive(DS1722_CFG_READ);
    uint8_t cfg = (uint8_t)spiSendReceive(0x00);   
    ceDeassert();
    return cfg;
}

int16_t ds1722ReadRaw(void) {
    // Burst read: address auto-increments from LSB (0x01) to MSB (0x02)
    ceAssert();
    spiSendReceive(DS1722_TEMP_LSB);
    uint8_t lsb = (uint8_t)spiSendReceive(0x00);
    uint8_t msb = (uint8_t)spiSendReceive(0x00);
    ceDeassert();

    return (int16_t)(((uint16_t)msb << 8) | lsb);
}

uint32_t ds1722ConversionMs(int bits) {
    switch (bits) {
        case 8:  return 75;
        case 9:  return 150;
        case 10: return 300;
        case 11: return 600;
        default: return 1200;
    }
}
