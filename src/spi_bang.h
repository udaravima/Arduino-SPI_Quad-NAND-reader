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
void sendDummies(uint8_t count);
uint8_t fBang(uint8_t data);

// =============================================================================
// NAND FLASH OPERATIONS
// =============================================================================

// Feature register access
uint8_t getStatusRegister(uint8_t regAddr);
void setFeatureRegister(uint8_t regAddr, uint8_t value);

// Wait for NAND ready (polls OIP bit)
uint8_t waitForReady(void);

// Enable Quad SPI mode (sets QE bit in B0h)
void enableQuadMode(void);

// Chip identification
void readChipID(void);

// Page operations
void loadPageToCache(uint32_t pageAddr);
uint16_t readFromCache(uint16_t colOffset, uint8_t *buffer, uint16_t bufSize);
uint16_t readNandPage(uint32_t pageAddr, uint16_t colOffset, uint8_t *buffer, uint16_t bufSize);

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

// Print buffer as hex dump
void printBufferHex(const uint8_t *buffer, uint16_t length);

// External buffer
extern uint8_t dataBuffer[BUFFER_SIZE];