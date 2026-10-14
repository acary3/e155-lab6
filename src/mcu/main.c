/*
File: main.c  (E155 Lab 6 - IoT LED + DS1722 temperature sensor)
Starter code: Josh Brake (jbrake@hmc.edu), 9/14/19

MCU side of the IoT device:
  - receives /REQ:<tag>\n from the ESP8266 over USART1 (PA9/PA10, 125000 baud)
  - tags: ledon, ledoff, res8, res9, res10, res11, res12 (anything else = refresh)
  - reads the DS1722 over SPI1 (PB3 SCK, PB4 MISO, PB5 MOSI, PB1 CE)
  - drives the LED on PA6
  - returns a complete HTML page (<!DOCTYPE html> ... </html>)
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"

/////////////////////////////////////////////////////////////////
//  Provided Constants and Functions
/////////////////////////////////////////////////////////////////

char* webpageStart = "<!DOCTYPE html><html><head><title>E155 Web Server Demo Webpage</title>\
	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
	</head>\
	<body><h1>E155 Web Server Demo Webpage</h1>";
char* ledStr = "<p>LED Control:</p><form action=\"ledon\"><input type=\"submit\" value=\"Turn the LED on!\"></form>\
	<form action=\"ledoff\"><input type=\"submit\" value=\"Turn the LED off!\"></form>";
char* resStr = "<p>Temperature resolution:</p>\
	<form action=\"res8\"  style=\"display:inline\"><input type=\"submit\" value=\"8-bit\"></form> \
	<form action=\"res9\"  style=\"display:inline\"><input type=\"submit\" value=\"9-bit\"></form> \
	<form action=\"res10\" style=\"display:inline\"><input type=\"submit\" value=\"10-bit\"></form> \
	<form action=\"res11\" style=\"display:inline\"><input type=\"submit\" value=\"11-bit\"></form> \
	<form action=\"res12\" style=\"display:inline\"><input type=\"submit\" value=\"12-bit\"></form>";
char* refreshStr = "<form action=\"/\"><input type=\"submit\" value=\"Refresh temperature\"></form>";
char* webpageEnd = "</body></html>";

/////////////////////////////////////////////////////////////////
// Request handling
/////////////////////////////////////////////////////////////////

// returns 1 if des is in request, -1 if not
int inString(char request[], char des[]) {
	if (strstr(request, des) != NULL) {return 1;}
	return -1;
}

// Remembers the LED state (static) so a plain refresh still reports it correctly
int updateLEDStatus(char request[])
{
	static int led_status = 0;
	if (inString(request, "ledoff")==1) {
		digitalWrite(LED_PIN, PIO_LOW);
		led_status = 0;
	}
	else if (inString(request, "ledon")==1) {
		digitalWrite(LED_PIN, PIO_HIGH);
		led_status = 1;
	}
	return led_status;
}

// Returns the requested resolution (8..12) or 0 if the request has none.
int parseResolution(char request[]) {
	if (inString(request, "res12") == 1) return 12;
	if (inString(request, "res11") == 1) return 11;
	if (inString(request, "res10") == 1) return 10;
	if (inString(request, "res9")  == 1) return 9;
	if (inString(request, "res8")  == 1) return 8;
	return 0;
}

/////////////////////////////////////////////////////////////////
// Temperature formatting 
/////////////////////////////////////////////////////////////////

void formatTemp(char *out, int16_t raw, int bits) {
	uint16_t mask = (uint16_t)(0xFFFFu << (16 - bits));
	int16_t  t    = (int16_t)((uint16_t)raw & mask);

	int32_t sixteenths = t / 16;                
	int32_t tenThou    = sixteenths * 625;       // units of 0.0001 C

	const char *sign = "";
	if (tenThou < 0) { sign = "-"; tenThou = -tenThou; }

	int32_t whole = tenThou / 10000;
	int32_t frac  = tenThou % 10000;
	int decimals  = bits - 8;                    // 0..4 decimal places

	// Build the fraction digits by hand 
	char fracStr[6];
	int32_t place = 1000;
	for (int i = 0; i < decimals; i++) {
		fracStr[i] = (char)('0' + (frac / place) % 10);
		place /= 10;
	}
	fracStr[decimals] = '\0';

	if (decimals == 0)
		sprintf(out, "%s%d", sign, (int)whole);
	else
		sprintf(out, "%s%d.%s", sign, (int)whole, fracStr);
}

// Two-digit uppercase hex without relying on printf width support
void hexByte(char *out, uint8_t v) {
	const char *h = "0123456789ABCDEF";
	out[0] = h[v >> 4];
	out[1] = h[v & 0xF];
	out[2] = '\0';
}

/////////////////////////////////////////////////////////////////
// Main
/////////////////////////////////////////////////////////////////

int main(void) {
  configureFlash();
  configureClock();

  gpioEnable(GPIO_PORT_A);
  gpioEnable(GPIO_PORT_B);
  gpioEnable(GPIO_PORT_C);

  pinMode(LED_PIN, GPIO_OUTPUT);
  digitalWrite(LED_PIN, PIO_LOW);

  RCC->APB2ENR |= (RCC_APB2ENR_TIM15EN);
  initTIM(TIM15);

  USART_TypeDef * USART = initUSART(USART1_ID, 125000);

  // SPI1: f_PCLK2/64 = 1.25 MHz at 80 MHz (DS1722 max is 5 MHz),
  // CPOL = 0, CPHA = 1 as required by the DS1722
  initSPI(SPI_BR_DIV64, 0, 1);

  // Start conversions
  int resolution = DEFAULT_RES;
  ds1722SetResolution(resolution);
  delay_millis(TIM15, ds1722ConversionMs(resolution));

  while(1) {
    /* Wait for ESP8266 to send a request of the form '/REQ:<tag>\n'. */
    char request[BUFF_LEN];
    memset(request, 0, BUFF_LEN);
    int charIndex = 0;

    while(inString(request, "\n") == -1) {
      // Clear 
      if (USART->ISR & USART_ISR_ORE) USART->ICR = USART_ICR_ORECF;
      if (!(USART->ISR & USART_ISR_RXNE)) continue;

      char c = readChar(USART);
      if (charIndex < BUFF_LEN - 1) request[charIndex++] = c;  
      else if (c == '\n') request[BUFF_LEN - 2] = '\n';         // still terminate
    }

    //Refreshes shows a fresh conversion
    int newRes = parseResolution(request);
    if (newRes != 0) {
      resolution = newRes;
      ds1722SetResolution(resolution);
    }

    // Reads temperature over SPI
    int16_t raw = ds1722ReadRaw();
    uint8_t cfg = ds1722ReadConfig();

    char tempStr[24];
    formatTemp(tempStr, raw, resolution);

    // Updates LED
    int led_status = updateLEDStatus(request);

    char ledStatusStr[20];
    if (led_status == 1)
      sprintf(ledStatusStr,"LED is on!");
    else
      sprintf(ledStatusStr,"LED is off!");

    char line[96];

    //transmits the webpage over UART
    sendString(USART, webpageStart);
    sendString(USART, ledStr);

    sendString(USART, "<h2>LED Status</h2><p>");
    sendString(USART, ledStatusStr);
    sendString(USART, "</p>");

    sendString(USART, "<h2>Temperature</h2>");
    sprintf(line, "<p style=\"font-size:1.5em\">%s &deg;C</p>", tempStr);
    sendString(USART, line);
    char cfgHex[3];
    hexByte(cfgHex, cfg);
    sprintf(line, "<p>Resolution: %d-bit (config register 0x%s)</p>",
            resolution, cfgHex);
    sendString(USART, line);
    sendString(USART, refreshStr);
    sendString(USART, resStr);

    sendString(USART, webpageEnd);
  }
}
