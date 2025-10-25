#include <Arduino.h>
#include "spi_bang.h"

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
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y)
{
  return x + y;
}