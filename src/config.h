/*
 * config.h - Centralized pin and NAND configuration
 * 
 * Pin mapping for ATmega328P (Arduino Uno):
 * - CLK, SIO0, SIO1 shared with SD card SPI
 * - SIO2, SIO3 dedicated for NAND Quad mode
 * - Separate CS for NAND and SD card
 */

#pragma once

#include <Arduino.h>

// =============================================================================
// PIN DEFINITIONS (ATmega328P / Arduino Uno)
// =============================================================================

// NAND Flash Pins (directly mapped to PORTB bits for bit-banging)
#define PIN_CLK      PB5   // D13 - Clock (shared with SD)
#define PIN_SIO0     PB3   // D11 - MOSI (shared with SD)
#define PIN_SIO1     PB4   // D12 - MISO (shared with SD)
#define PIN_SIO2     PB2   // D10 - WP# (Quad mode)
#define PIN_SIO3     PB1   // D9  - HOLD# (Quad mode)
#define PIN_NAND_CS  PB0   // D8  - NAND Chip Select

// SD Card CS (Arduino pin number for SD library)
#define PIN_SD_CS    7     // D7

// =============================================================================
// BIT MASKS FOR PORT MANIPULATION
// =============================================================================

// Output mask: CLK + all SIO lines
#define MASK_CLK        (1 << PIN_CLK)
#define MASK_SIO0       (1 << PIN_SIO0)
#define MASK_SIO1       (1 << PIN_SIO1)
#define MASK_SIO2       (1 << PIN_SIO2)
#define MASK_SIO3       (1 << PIN_SIO3)
#define MASK_NAND_CS    (1 << PIN_NAND_CS)

// Combined masks
#define MASK_ALL_SIO    (MASK_SIO0 | MASK_SIO1 | MASK_SIO2 | MASK_SIO3)
#define MASK_ALL_OUT    (MASK_CLK | MASK_ALL_SIO | MASK_NAND_CS)

// =============================================================================
// TIMING
// =============================================================================

#define BDELAY 10  // Bit-bang delay in microseconds

// =============================================================================
// BUFFER SIZE
// =============================================================================

#define BUFFER_SIZE 256  // RAM buffer for data transfer

// =============================================================================
// NAND CONFIGURATION (User selectable via serial command)
// =============================================================================

// NAND size structure
typedef struct {
    uint16_t pageSize;       // Page size in bytes (typically 2048)
    uint8_t  pagesPerBlock;  // Pages per block (typically 64)
    uint16_t totalBlocks;    // Total blocks in device
    uint16_t oobSize;        // OOB/spare area size per page
} NandConfig_t;

// Preset configurations for common NAND sizes
// Usage: Call setNandSize() with size in MB
#define NAND_PAGE_SIZE     2048
#define NAND_OOB_SIZE      64
#define NAND_PAGES_PER_BLK 64

// Default: 128MB NAND
extern NandConfig_t nandConfig;

// Initialize with size in MB (128, 256, 512, 1024)
void setNandSize(uint16_t sizeMB);
uint32_t getTotalPages(void);
uint32_t getTotalBytes(void);
