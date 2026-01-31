#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "config.h"

// =============================================================================
// SPI BIT-BANG FUNCTIONS
// =============================================================================

// Chip Select control
void setCS(bool active);

// Single SPI operations
void sendCmdSpi(uint8_t data);
uint8_t readSpiByte(void);
uint16_t readSpiBytes(uint8_t *buffer, uint16_t length);

// Dual SPI operations
uint8_t readDSpiByte(void);
uint16_t readDSpiBytes(uint8_t *buffer, uint16_t length);

// Quad SPI operations
uint8_t readQSpiByte(void);
uint16_t readQSpiBytes(uint8_t *buffer, uint16_t length);

// Low-level bit-bang operations
void sendBang(uint8_t data);
uint8_t readBang(void);
void sendDummites(uint8_t count);
uint8_t fBang(uint8_t data);

// =============================================================================
// NAND FLASH OPERATIONS
// =============================================================================

// Chip identification
void readChipID(void);

// Page operations
// Reads a full page from NAND into buffer
// Returns number of bytes read
uint16_t readNandPage(uint32_t pageAddr, uint8_t *buffer, uint16_t bufSize);

// Block operations  
// Load page into NAND internal buffer
void loadPageToCache(uint32_t pageAddr);

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

// Print buffer as hex dump
void printBufferHex(const uint8_t *buffer, uint16_t length);

// External buffer
extern uint8_t dataBuffer[BUFFER_SIZE];