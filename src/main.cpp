#include <Arduino.h>
#include "spi_bang.h"
#include "main.h"
// put function declarations here:
int myFunction(int, int);

void setup()
{
  Serial.begin(115200);
  Serial.println("Project MiniPro B14 13An6");
  // Set CLK Always a output
  DDRB |= (1 << CLK);
  setCS(false);
  delay(5000);
  readChipID();
  ThePageRead();
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
  for (uint8_t i = 0; i < 64; i++)
  {
    Serial.print(readQSpiByte(), HEX);
  }
  setCS(false);
}