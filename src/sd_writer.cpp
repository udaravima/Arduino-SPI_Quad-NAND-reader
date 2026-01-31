/*
 * sd_writer.cpp - SD Card Writer for NAND dump
 */

#include "sd_writer.h"
#include "config.h"
#include <SPI.h>
#include <SD.h>

static File dumpFile;
static bool sdInitialized = false;
static uint32_t totalBytesWritten = 0;

bool initSD(void)
{
    // Ensure NAND CS is HIGH (deselected) before SD init
    pinMode(8, OUTPUT);  // D8 = NAND CS (PB0)
    digitalWrite(8, HIGH);
    
    // Initialize SD card on PIN_SD_CS
    if (SD.begin(PIN_SD_CS)) {
        sdInitialized = true;
        Serial.println(F("SD card initialized"));
        return true;
    } else {
        sdInitialized = false;
        Serial.println(F("SD card init failed!"));
        return false;
    }
}

bool isSDReady(void)
{
    return sdInitialized;
}

bool openDumpFile(const char* filename)
{
    if (!sdInitialized) {
        Serial.println(F("SD not initialized"));
        return false;
    }
    
    // Close any previously open file
    if (dumpFile) {
        dumpFile.close();
    }
    
    // Remove existing file if present
    if (SD.exists(filename)) {
        SD.remove(filename);
    }
    
    // Open for writing
    dumpFile = SD.open(filename, FILE_WRITE);
    
    if (dumpFile) {
        totalBytesWritten = 0;
        Serial.print(F("Opened: "));
        Serial.println(filename);
        return true;
    } else {
        Serial.println(F("Failed to open file"));
        return false;
    }
}

uint16_t writeToFile(const uint8_t* buffer, uint16_t length)
{
    if (!dumpFile) {
        return 0;
    }
    
    uint16_t written = dumpFile.write(buffer, length);
    totalBytesWritten += written;
    
    return written;
}

void closeDumpFile(void)
{
    if (dumpFile) {
        dumpFile.flush();
        dumpFile.close();
        Serial.print(F("File closed. Total bytes: "));
        Serial.println(totalBytesWritten);
    }
}

uint32_t getBytesWritten(void)
{
    return totalBytesWritten;
}
