/*
 * config.cpp - NAND configuration implementation
 */

#include "config.h"

// Default NAND configuration (128MB)
NandConfig_t nandConfig = {
    .pageSize = NAND_PAGE_SIZE,
    .pagesPerBlock = NAND_PAGES_PER_BLK,
    .totalBlocks = 1024,  // 128MB default
    .oobSize = NAND_OOB_SIZE
};

void setNandSize(uint16_t sizeMB) {
    nandConfig.pageSize = NAND_PAGE_SIZE;
    nandConfig.pagesPerBlock = NAND_PAGES_PER_BLK;
    nandConfig.oobSize = NAND_OOB_SIZE;
    
    // Calculate blocks based on size
    // Each block = 64 pages * 2048 bytes = 128KB
    // Blocks for given MB = sizeMB * 1024KB / 128KB = sizeMB * 8
    nandConfig.totalBlocks = sizeMB * 8;
}

uint32_t getTotalPages(void) {
    return (uint32_t)nandConfig.totalBlocks * nandConfig.pagesPerBlock;
}

uint32_t getTotalBytes(void) {
    return getTotalPages() * nandConfig.pageSize;
}
