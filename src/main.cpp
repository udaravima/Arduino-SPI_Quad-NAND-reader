#include <Arduino.h>
#include "spi_bang.h"
#include "main.h"
#include "interface.h"
#include "api.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void setup()
{
  Serial.begin(115200);
  Serial.println("Project MiniPro B14 13An6");
  // Set CLK Always a output
  DDRB |= (1 << CLK);
  setCS(false);
  delay(5000);
  readChipID();
  initInterface();
}

void loop()
{
}

void ThePageRead()
{
  setCS(true);
  sendCmdSpi(0x6B);
  sendCmdSpi(0x00);
  sendCmdSpi(0x00);
  sendDummites(8);
  uint16_t bytesRead = readQSpiBytes(dataBuffer, 32);
  Serial.print("Bytes Read: ");
  Serial.println(bytesRead);
  printBufferHex(dataBuffer, 32);
  Serial.println(); // Add a newline after printing the buffer
  setCS(false);
}