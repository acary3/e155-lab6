/**
    Main Header: Contains general defines and selected portions of CMSIS files
    @file main.h
    @author Josh Brake
    @version 1.0 10/7/2020
*/

#ifndef MAIN_H
#define MAIN_H

#include "STM32L432KC.h"
#include "STM32L432KC_SPI.h"
#include "DS1722.h"

#define LED_PIN PA6      
#define BUFF_LEN 32

#define DEFAULT_RES 12   

#endif // MAIN_H
