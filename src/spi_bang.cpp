/*
 * spi_bang.cpp - SPI Bit-Banging for NAND Flash
 * 
 * Pin mapping (configurable in config.h):
 *   CLK   -> PIN_CLK   (PB5/D13)
 *   SIO0  -> PIN_SIO0  (PB3/D11) - MOSI
 *   SIO1  -> PIN_SIO1  (PB4/D12) - MISO
 *   SIO2  -> PIN_SIO2  (PB2/D10) - WP#
 *   SIO3  -> PIN_SIO3  (PB1/D9)  - HOLD#
 *   CS    -> PIN_NAND_CS (PB0/D8)
 */

#include <Arduino.h>
#include "spi_bang.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Global buffer for data storage
uint8_t dataBuffer[BUFFER_SIZE];

// =============================================================================
// CHIP SELECT CONTROL
// =============================================================================

void setCS(bool active)
{
    // Set CLK and CS as outputs
    DDRB |= MASK_CLK | MASK_NAND_CS;
    // CLK LOW for SPI mode 0
    PORTB &= ~MASK_CLK;
    // CS is active LOW
    if (active) {
        PORTB &= ~MASK_NAND_CS;
    } else {
        PORTB |= MASK_NAND_CS;
    }
}

// =============================================================================
// LOW-LEVEL BIT-BANG OPERATIONS
// =============================================================================

/*
 * Send 4 bits of data on SIO0-3 with a clock pulse
 * For standard SPI, only SIO0 is used (bit 0 of data)
 */
void sendBang(uint8_t data)
{
    data &= 0x0f;
    
    // Set all SIO lines as outputs
    DDRB |= MASK_ALL_SIO | MASK_CLK;
    
    // Map data bits to SIO pins
    // data[0] -> SIO0, data[1] -> SIO1, data[2] -> SIO2, data[3] -> SIO3
    uint8_t portVal = PORTB & ~MASK_ALL_SIO;
    if (data & 0x01) portVal |= MASK_SIO0;
    if (data & 0x02) portVal |= MASK_SIO1;
    if (data & 0x04) portVal |= MASK_SIO2;
    if (data & 0x08) portVal |= MASK_SIO3;
    PORTB = portVal;
    
    // Rising edge
    PORTB |= MASK_CLK;
    delayMicroseconds(BDELAY);
    
    // Falling edge
    PORTB &= ~MASK_CLK;
    delayMicroseconds(BDELAY);
}

/*
 * Read 4 bits from SIO0-3 with a clock pulse
 * Returns nibble with SIO0=bit0, SIO1=bit1, SIO2=bit2, SIO3=bit3
 */
uint8_t readBang()
{
    uint8_t data = 0;
    
    // Set SIO lines as inputs
    DDRB &= ~MASK_ALL_SIO;
    DDRB |= MASK_CLK;
    
    // Rising edge
    PORTB |= MASK_CLK;
    delayMicroseconds(BDELAY);
    
    // Sample data on the rising edge (SPI Mode 0)
    uint8_t pinVal = PINB;
    if (pinVal & MASK_SIO0) data |= 0x01;
    if (pinVal & MASK_SIO1) data |= 0x02;
    if (pinVal & MASK_SIO2) data |= 0x04;
    if (pinVal & MASK_SIO3) data |= 0x08;
    
    // Falling edge
    PORTB &= ~MASK_CLK;
    delayMicroseconds(BDELAY);
    
    return data;
}

/*
 * Send dummy clock cycles (for read timing)
 */
void sendDummies(uint8_t count)
{
    // CLK as output, SIO as inputs with pull-ups
    DDRB = (DDRB & ~MASK_ALL_SIO) | MASK_CLK;
    PORTB |= MASK_ALL_SIO; // Enable pull-ups
    
    for (uint8_t i = 0; i < count; i++)
    {
        PORTB |= MASK_CLK;
        delayMicroseconds(BDELAY);
        PORTB &= ~MASK_CLK;
        delayMicroseconds(BDELAY);
    }
}

/*
 * Full-duplex bang: send on rising edge, read on falling edge
 */
uint8_t fBang(uint8_t data)
{
    data &= 0x0f;
    
    // Set SIO as outputs
    DDRB |= MASK_ALL_SIO | MASK_CLK;
    
    // Output data
    uint8_t portVal = PORTB & ~MASK_ALL_SIO;
    if (data & 0x01) portVal |= MASK_SIO0;
    if (data & 0x02) portVal |= MASK_SIO1;
    if (data & 0x04) portVal |= MASK_SIO2;
    if (data & 0x08) portVal |= MASK_SIO3;
    PORTB = portVal;
    
    // Rising edge
    PORTB |= MASK_CLK;
    delayMicroseconds(BDELAY);
    
    // Switch SIO to inputs
    DDRB &= ~MASK_ALL_SIO;
    
    // Falling edge
    PORTB &= ~MASK_CLK;
    delayMicroseconds(BDELAY);
    
    // Read data
    data = 0;
    uint8_t pinVal = PINB;
    if (pinVal & MASK_SIO0) data |= 0x01;
    if (pinVal & MASK_SIO1) data |= 0x02;
    if (pinVal & MASK_SIO2) data |= 0x04;
    if (pinVal & MASK_SIO3) data |= 0x08;
    
    return data;
}

// =============================================================================
// SINGLE SPI OPERATIONS (1-bit mode using SIO0/SIO1)
// =============================================================================

/*
 * Send a command byte using standard SPI (MSB first on SIO0)
 */
void sendCmdSpi(uint8_t data)
{
    DDRB |= MASK_SIO0 | MASK_CLK;
    
    for (int8_t b = 7; b >= 0; b--)
    {
        // Set SIO0 based on bit value
        if (data & (1 << b)) {
            PORTB |= MASK_SIO0;
        } else {
            PORTB &= ~MASK_SIO0;
        }
        
        // Clock pulse
        PORTB |= MASK_CLK;
        delayMicroseconds(BDELAY);
        PORTB &= ~MASK_CLK;
        delayMicroseconds(BDELAY);
    }
}

/*
 * Read a byte using standard SPI (MSB first from SIO1)
 */
uint8_t readSpiByte()
{
    uint8_t data = 0;
    
    DDRB = (DDRB & ~MASK_SIO1) | MASK_CLK;  // SIO1 input, CLK output
    
    for (int8_t i = 7; i >= 0; i--)
    {
        // Rising edge
        PORTB |= MASK_CLK;
        delayMicroseconds(BDELAY);
        
        // Sample SIO1 on the rising edge (SPI Mode 0)
        if (PINB & MASK_SIO1) {
            data |= (1 << i);
        }
        
        // Falling edge
        PORTB &= ~MASK_CLK;
        delayMicroseconds(BDELAY);
    }
    
    return data;
}

uint16_t readSpiBytes(uint8_t *buffer, uint16_t length)
{
    uint16_t bytesRead = 0;
    while (bytesRead < length && bytesRead < BUFFER_SIZE)
    {
        buffer[bytesRead++] = readSpiByte();
    }
    return bytesRead;
}

// =============================================================================
// DUAL SPI OPERATIONS (2-bit mode using SIO0 and SIO1)
// =============================================================================

uint8_t readDSpiByte()
{
    uint8_t data = 0;
    
    DDRB = (DDRB & ~(MASK_SIO0 | MASK_SIO1)) | MASK_CLK;
    
    for (int8_t i = 3; i >= 0; i--)
    {
        // Rising edge
        PORTB |= MASK_CLK;
        delayMicroseconds(BDELAY);
        
        // Sample SIO0 and SIO1 on the rising edge (SPI Mode 0)
        uint8_t nibble = 0;
        uint8_t pinVal = PINB;
        if (pinVal & MASK_SIO0) nibble |= 0x01;
        if (pinVal & MASK_SIO1) nibble |= 0x02;
        
        data |= (nibble << (i * 2));
        
        // Falling edge
        PORTB &= ~MASK_CLK;
        delayMicroseconds(BDELAY);
    }
    
    return data;
}

uint16_t readDSpiBytes(uint8_t *buffer, uint16_t length)
{
    uint16_t bytesRead = 0;
    while (bytesRead < length && bytesRead < BUFFER_SIZE)
    {
        buffer[bytesRead++] = readDSpiByte();
    }
    return bytesRead;
}

// =============================================================================
// QUAD SPI OPERATIONS (4-bit mode using SIO0-3)
// =============================================================================

uint8_t readQSpiByte()
{
    uint8_t data = 0;
    
    // Set all SIO as inputs
    DDRB = (DDRB & ~MASK_ALL_SIO) | MASK_CLK;
    
    // Read high nibble
    data = readBang() << 4;
    // Read low nibble
    data |= readBang();
    
    return data;
}

uint16_t readQSpiBytes(uint8_t *buffer, uint16_t length)
{
    uint16_t bytesRead = 0;
    while (bytesRead < length && bytesRead < BUFFER_SIZE)
    {
        buffer[bytesRead++] = readQSpiByte();
    }
    return bytesRead;
}

// =============================================================================
// NAND FLASH OPERATIONS
// =============================================================================

/*
 * Get status/feature register value
 * regAddr: A0h=Block Lock, B0h=Config, C0h=Status, D0h=Driver Strength
 */
uint8_t getStatusRegister(uint8_t regAddr)
{
    uint8_t status;
    
    setCS(true);
    sendCmdSpi(0x0F);    // Get Features command
    sendCmdSpi(regAddr); // Register address
    status = readSpiByte();
    setCS(false);
    
    return status;
}

/*
 * Set feature register value
 */
void setFeatureRegister(uint8_t regAddr, uint8_t value)
{
    setCS(true);
    sendCmdSpi(0x1F);    // Set Features command
    sendCmdSpi(regAddr); // Register address
    sendCmdSpi(value);   // Value to write
    setCS(false);
}

/*
 * Wait for NAND to be ready by polling OIP bit in status register (C0h)
 * OIP = bit 0: 1=busy, 0=ready
 * Returns status register value when ready
 */
uint8_t waitForReady(void)
{
    uint8_t status;
    uint16_t timeout = 10000; // Max iterations
    
    do {
        status = getStatusRegister(0xC0);
        if (!(status & 0x01)) {
            return status;
        }
        delayMicroseconds(10);
    } while (--timeout > 0);
    
    Serial.println(F("WARN: NAND timeout!"));
    return status;
}

/*
 * Enable Quad SPI mode by setting QE bit in config register (B0h)
 * QE = bit 0 of register B0h
 */
void enableQuadMode(void)
{
    uint8_t config = getStatusRegister(0xB0);
    if (!(config & 0x01)) {
        config |= 0x01; // Set QE bit
        setFeatureRegister(0xB0, config);
        Serial.println(F("Quad mode enabled"));
    } else {
        Serial.println(F("Quad mode already enabled"));
    }
}

void readChipID()
{
    uint16_t id = 0;
    
    setCS(true);
    sendCmdSpi(0x9F);  // Read ID command
    sendDummies(8);    // 1 dummy byte (8 clocks)
    readSpiBytes((uint8_t *)&dataBuffer, 2);
    setCS(false);
    
    id = (dataBuffer[0] << 8) | dataBuffer[1];
    
    char buff[32];
    sprintf(buff, "Chip ID: 0x%04X", id);
    Serial.println(buff);
}

/*
 * Load a page from NAND into its internal cache buffer
 * This is step 1 of a page read operation
 * Polls OIP bit until operation completes
 */
void loadPageToCache(uint32_t pageAddr)
{
    setCS(true);
    
    // Command: Page Read to Cache (13h)
    sendCmdSpi(0x13);
    
    // 24-bit row address: 8 dummy bits + 16-bit block/page address
    sendCmdSpi((pageAddr >> 16) & 0xFF); // Dummy bits (0 for < 65536 pages)
    sendCmdSpi((pageAddr >> 8) & 0xFF);  // Row address high
    sendCmdSpi(pageAddr & 0xFF);         // Row address low
    
    setCS(false);
    
    // Poll OIP bit until page is loaded
    waitForReady();
}

/*
 * Read data from NAND internal cache buffer at a given column offset
 * using Fast Read Quad Output (6Bh)
 * Must call loadPageToCache() first!
 *
 * Column address format (16 bits):
 *   [15:13] = dummy bits
 *   [12]    = plane select (0 for 1Gbit devices)
 *   [11:0]  = 12-bit column address (0-2111)
 *
 * Returns number of bytes actually read
 */
uint16_t readFromCache(uint16_t colOffset, uint8_t *buffer, uint16_t bufSize)
{
    setCS(true);
    
    // Command: Fast Read Quad Output (6Bh) - 1-1-4 schema
    sendCmdSpi(0x6B);
    
    // Column address (2 bytes)
    sendCmdSpi((colOffset >> 8) & 0x0F); // Upper nibble (plane=0, col[11:8])
    sendCmdSpi(colOffset & 0xFF);        // col[7:0]
    
    // 1 dummy byte (8 clock cycles)
    sendDummies(8);
    
    // Read data using Quad SPI
    uint16_t bytesRead = readQSpiBytes(buffer, bufSize);
    
    setCS(false);
    
    return bytesRead;
}

/*
 * Read a full page from NAND into buffer
 * Handles the two-step process: load page to cache, then read from cache
 * Returns number of bytes read
 */
uint16_t readNandPage(uint32_t pageAddr, uint16_t colOffset, uint8_t *buffer, uint16_t bufSize)
{
    // Step 1: Load page to cache
    loadPageToCache(pageAddr);
    
    // Step 2: Read from cache at specified column offset
    return readFromCache(colOffset, buffer, bufSize);
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

void printBufferHex(const uint8_t *buffer, uint16_t length)
{
    char buff[16];
    for (uint16_t i = 0; i < length; i++)
    {
        if (i % 16 == 0)
        {
            sprintf(buff, "\n%04X: ", i);
            Serial.print(buff);
        }
        sprintf(buff, "%02X ", buffer[i]);
        Serial.print(buff);
    }
    Serial.println();
}