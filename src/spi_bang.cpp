/*

* * * * * * * * * * * * * * * * * * * * * * * * * * *

* * * * Bit Banging for Reading SPI NAND FLASH * * *

* * * * * * * * * * * * * * * * * * * * * * * * * * *

* 6 SLK --> PB5/13

* 5 SI / SIO 0 --> PB1/12

* 2 SO / SIO 1 --> PB2/11

* 3 WP# / SIO 2 --> PB3/10

* 7 HOLD# / SIO 3 --> PB4/9

* 1 CS# --> PB0/8

* 8 VCC
* 4 GND

* 001 1 1110
* 0 0  5    4   |   3    2   1  0
*      CLK  IO3 |   IO2 IO1 IO0
*/

#include <Arduino.h>
#include "spi_bang.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void readChipID()
{
  setCS(true);
  sendCmdSpi(0x9f);
  sendDummites(8);
  uint16_t id = 0;
  id = (id << 8) + readSpiByte();
  id = (id << 8) + readSpiByte();
  setCS(false);
  char buff[50];
  sprintf(buff, "Chip id: 0x%x", id);
  Serial.println(buff);
}

void ThePageRead()
{
  setCS(true);
  sendCmdSpi(0x6B);
  sendCmdSpi(0x00);
  sendCmdSpi(0x00);
  sendDummites(8);
  for (uint8_t i = 0; i < 32; i++)
  {
    Serial.print(readQSpiByte(), HEX);
  }
  setCS(false);
}

// Single SPI
void sendCmdSpi(uint8_t data)
{
  for (int b = 7; b >= 0; b--)
  {
    sendBang((data >> b) | 0x0e);
  }
}

// SPI Read x1
uint8_t readSpiByte()
{
  /* Reading from SO at PB2 */
  uint8_t data = 0;
  for (int i = 0; i < 8; i++)
  {
    data = (data << 1) + ((readBang() >> 1) & 0x01);
  }
  return data;
}

// SPI Read x2
uint8_t readDSpiByte()
{
  uint8_t data = 0;
  for (int i = 0; i < 4; i++)
  {
    data = (data << 2) + (readBang() & 0x03);
  }
  return data;
}

// SPI Read x4
uint8_t readQSpiByte()
{
  uint8_t data = 0;
  for (int i = 0; i < 2; i++)
  {
    data = (data << 4) + (readBang() & 0x0f);
  }
  return data;
}

void setCS(bool val)
{
  DDRB |= (1 << CS) | (1 << CLK);
  // Setting CLK LOW for mode 0
  PORTB &= ~(1 << CLK);
  val ? PORTB &= ~(1 << CS) : PORTB |= (1 << CS);
}

// Support up to Quad SPI or setting pins
void sendBang(uint8_t data)
{
  data &= 0x0f;
  DDRB |= 0x3e; // DATA & CLK OUT 0011 1110
  PORTB = (PORTB & 0xc1) | (data << 1);
  // Rise
  PORTB |= (1 << CLK);
  delayMicroseconds(BDELAY);
  PORTB &= ~(1 << CLK);
  delayMicroseconds(BDELAY);
}

uint8_t readBang()
{
  uint8_t data;
  DDRB = (DDRB & 0xe1);      // DATA IN
  data = (PINB >> 1) & 0x0f; // Sample first
  // Rise
  PORTB |= (1 << CLK);
  delayMicroseconds(BDELAY);
  // Fall
  PORTB &= ~(1 << CLK);
  delayMicroseconds(BDELAY);
  return data;
}

void sendDummites(uint8_t count)
{
  DDRB |= (DDRB & 0xe1) | (1 << CLK); // CLK Set DATA IN
  PORTB |= 0x1e;                      // Set DATA PULL_UPS
  for (int i = 0; i < count; i++)
  {
    // Rise
    PORTB |= (1 << CLK);
    delayMicroseconds(BDELAY);
    // Fall
    PORTB &= ~(1 << CLK);
    delayMicroseconds(BDELAY);
  }
}

// Classic Bit Bang with Some Nand Additions
// TODO: have to fix timing while testing Including out
// Support Quad SPI FullDuplex
uint8_t fBang(uint8_t data)
{
  // make sure data is 4 bits
  data &= 0x0f;
  // Setting up Data & clocks (Rising Phase)
  DDRB |= 0x1e; // Setting data out
  PORTB = (PORTB & 0xe1) | (data << 1);
  // Rise
  PORTB |= (1 << CLK);
  delayMicroseconds(BDELAY);
  // Change of hearts data in
  DDRB &= 0xe1;
  // Fall
  PORTB &= ~(1 << CLK);
  delayMicroseconds(BDELAY);
  // start reading
  data = (PINB >> 1) & 0x0f;
  return data;
}