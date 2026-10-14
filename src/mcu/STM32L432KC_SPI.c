/**
    SPI1 driver for the STM32L432KC using the CMSIS device templates.
    Pins: SCK = PB3, MISO = PB4, MOSI = PB5 (all AF5), CE = PB1 (GPIO).
    @file STM32L432KC_SPI.c
*/

#include "STM32L432KC_SPI.h"

void initSPI(int br, int cpol, int cpha) {
    // 1. Clocks: GPIOB for the pins, SPI1 lives on APB2
    gpioEnable(GPIO_PORT_B);
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // 2. GPIO: SCK/MISO/MOSI to alternate function 5 (SPI1), CE as plain output.
    //    (PB3/PB4 default to JTAG at reset; reassigning them is fine because
    //    the board is programmed over SWD on PA13/PA14.)
    pinMode(SPI_SCK, GPIO_ALT);
    pinMode(SPI_SDI, GPIO_ALT);
    pinMode(SPI_SDO, GPIO_ALT);
    pinMode(SPI_CE,  GPIO_OUTPUT);
    digitalWrite(SPI_CE, PIO_LOW);   // DS1722 CE is active HIGH -> idle low

    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL3 | GPIO_AFRL_AFSEL4 | GPIO_AFRL_AFSEL5);
    GPIOB->AFR[0] |=  _VAL2FLD(GPIO_AFRL_AFSEL3, 5)
                   |  _VAL2FLD(GPIO_AFRL_AFSEL4, 5)
                   |  _VAL2FLD(GPIO_AFRL_AFSEL5, 5);

    // Faster edges on SCK/MOSI so the logic-analyzer traces are clean
    GPIOB->OSPEEDR |= GPIO_OSPEEDR_OSPEED3 | GPIO_OSPEEDR_OSPEED5;

    // 3. SPI1 configuration (peripheral must be disabled while configuring)
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 = 0;
    SPI1->CR1 |= _VAL2FLD(SPI_CR1_BR, br);       // baud rate
    SPI1->CR1 |= _VAL2FLD(SPI_CR1_CPOL, cpol);   // clock polarity
    SPI1->CR1 |= _VAL2FLD(SPI_CR1_CPHA, cpha);   // clock phase
    SPI1->CR1 |= SPI_CR1_MSTR;                   // master mode
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;      // software NSS

    // 8-bit frames; FRXTH so RXNE fires once 8 bits are in the RX FIFO
    SPI1->CR2 = _VAL2FLD(SPI_CR2_DS, 0b0111) | SPI_CR2_FRXTH;

    // 4. Enable
    SPI1->CR1 |= SPI_CR1_SPE;
}

char spiSendReceive(char send) {
    // Waits for room in the TX FIFO
    while (!(SPI1->SR & SPI_SR_TXE));

    // 8-bit access to DR so only one frame is queued 
    *(volatile uint8_t *)(&SPI1->DR) = (uint8_t)send;

    // Waits for the received byte, then read it with an 8-bit access
    while (!(SPI1->SR & SPI_SR_RXNE));
    char rec = (char)(*(volatile uint8_t *)(&SPI1->DR));

    // Makes sure the last clock edge is done before the caller drops CE
    while (SPI1->SR & SPI_SR_BSY);

    return rec;
}
