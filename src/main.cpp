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
    enableQuadMode();
    initInterface();
}

void loop()
{
    // Serial commands are handled in serialEvent()
}

void ThePageRead()
{
    // Load page 0 into NAND cache first
    loadPageToCache(0);
    
    // Read 32 bytes from cache at column 0
    uint16_t bytesRead = readFromCache(0, dataBuffer, 32);
    Serial.print(F("Bytes Read: "));
    Serial.println(bytesRead);
    printBufferHex(dataBuffer, 32);
    Serial.println();
}