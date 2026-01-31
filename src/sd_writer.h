/*
 * sd_writer.h - SD Card Writer for NAND dump
 */

#pragma once

#include <Arduino.h>
#include <stdint.h>

// =============================================================================
// SD CARD FUNCTIONS
// =============================================================================

// Initialize SD card
// Returns true on success
bool initSD(void);

// Check if SD card is initialized
bool isSDReady(void);

// Open a file for writing (creates new or overwrites)
// Returns true on success
bool openDumpFile(const char* filename);

// Write a buffer to the currently open file
// Returns number of bytes written
uint16_t writeToFile(const uint8_t* buffer, uint16_t length);

// Close the current file
void closeDumpFile(void);

// Get bytes written so far
uint32_t getBytesWritten(void);
