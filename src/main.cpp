#include <Arduino.h>
#include "config.h"
#include "spi_bang.h"
#include "main.h"
#include "interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void setup()
{
    Serial.begin(115200);
    Serial.println(F("=== SPI NAND Reader ==="));
    Serial.println(F("Pin Config: CLK=D13, SIO0=D11, SIO1=D12"));
    Serial.println(F("            SIO2=D10, SIO3=D9, NAND_CS=D8"));
    Serial.println(F("            SD_CS=D7"));
    
    // Initialize CLK as output
    DDRB |= MASK_CLK;
    setCS(false);
    
    delay(1000);
    readChipID();
    initInterface();
}

void loop()
{
    // Serial commands are handled in serialEvent()
}

void ThePageRead()
{
    setCS(true);
    sendCmdSpi(0x6B);
    sendCmdSpi(0x00);
    sendCmdSpi(0x00);
    sendDummites(8);
    uint16_t bytesRead = readQSpiBytes(dataBuffer, 32);
    Serial.print(F("Bytes Read: "));
    Serial.println(bytesRead);
    printBufferHex(dataBuffer, 32);
    Serial.println();
    setCS(false);
}