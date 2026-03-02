#include "interface.h"
#include "config.h"
#include "main.h"
#include "spi_bang.h"
#include "sd_writer.h"
#include <Arduino.h>
#include <SPI.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define MAX_COMMAND_LENGTH 64
#define MAX_ARGS 6

static char cmdBuffer[MAX_COMMAND_LENGTH];
static int cmdIndex = 0;

// Forward declarations
static void trimInPlace(char *s);
static void cmdCopyToSD(int argc, char *argv[]);

// Trim leading and trailing whitespace in-place
static void trimInPlace(char *s)
{
    char *start = s;
    while (*start && isspace((unsigned char)*start))
        start++;
    if (start != s)
        memmove(s, start, strlen(start) + 1);

    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
    {
        s[len - 1] = '\0';
        len--;
    }
}

// Called by Arduino core when serial data arrives
void serialEvent()
{
    while (Serial.available())
    {
        char c = (char)Serial.read();

        if (c == '\n' || c == '\r')
        {
            if (cmdIndex > 0)
            {
                cmdBuffer[cmdIndex] = '\0';
                trimInPlace(cmdBuffer);
                if (strlen(cmdBuffer) > 0)
                    parseAndExecuteCommand(cmdBuffer);
                cmdIndex = 0;
            }
        }
        else if (cmdIndex < MAX_COMMAND_LENGTH - 1)
        {
            cmdBuffer[cmdIndex++] = c;
        }
    }
}

// Restore bit-bang SPI after SD card hardware SPI usage
static void restoreBitBangSPI()
{
    // Disable hardware SPI peripheral to release pin control
    SPCR = 0;
    // Reconfigure pins for bit-bang
    DDRB |= MASK_CLK;
    setCS(false);
}

// Copy NAND to SD card
static void cmdCopyToSD(int argc, char *argv[])
{
    const char* filename = (argc >= 2) ? argv[1] : "nand.bin";
    
    if (!isSDReady()) {
        Serial.println(F("SD not ready. Run 'initsd' first."));
        return;
    }
    
    if (!openDumpFile(filename)) {
        return;
    }
    
    uint32_t totalPages = getTotalPages();
    uint32_t pagesDone = 0;
    uint32_t lastProgress = 0;
    
    Serial.print(F("Copying "));
    Serial.print(totalPages);
    Serial.println(F(" pages..."));
    
    // Copy page by page
    for (uint32_t page = 0; page < totalPages; page++)
    {
        uint16_t pageSize = nandConfig.pageSize;
        uint16_t colOffset = 0;
        
        // Load this page into NAND cache once
        loadPageToCache(page);
        
        // Read from cache in chunks with increasing column offset
        while (colOffset < pageSize)
        {
            uint16_t chunkSize = min((uint16_t)BUFFER_SIZE, (uint16_t)(pageSize - colOffset));
            
            // Read chunk from NAND cache at current column offset
            uint16_t bytesRead = readFromCache(colOffset, dataBuffer, chunkSize);
            
            // Write to SD (briefly uses hardware SPI)
            writeToFile(dataBuffer, bytesRead);
            
            // Restore bit-bang pin config after SD hardware SPI usage
            restoreBitBangSPI();
            
            colOffset += bytesRead;
            
            if (bytesRead < chunkSize) break;
        }
        
        pagesDone++;
        
        // Progress update every 64 pages (1 block)
        if (pagesDone - lastProgress >= 64)
        {
            uint8_t percent = (uint8_t)((pagesDone * 100UL) / totalPages);
            Serial.print(F("Progress: "));
            Serial.print(percent);
            Serial.print(F("% ("));
            Serial.print(pagesDone);
            Serial.print(F("/"));
            Serial.print(totalPages);
            Serial.println(F(")"));
            lastProgress = pagesDone;
        }
    }
    
    closeDumpFile();
    restoreBitBangSPI();
    Serial.println(F("Copy complete!"));
}

// Dump NAND raw binary data to USB (Serial)
static void cmdDumpToUSB(int argc, char *argv[])
{
    uint32_t totalPages = getTotalPages();
    uint32_t totalBytes = getTotalBytes();
    
    // Announce we are ready and the size expected
    Serial.print(F("READY FOR DUMP:"));
    Serial.println(totalBytes);
    
    // Wait for the host script to send the 'G' (Go) character
    while (!Serial.available()) {
        delay(1);
    }
    char go = Serial.read();
    if (go != 'G') {
        Serial.println(F("DUMP CANCELLED"));
        return;
    }
    
    // The host expects pure binary data now
    for (uint32_t page = 0; page < totalPages; page++)
    {
        uint16_t pageSize = nandConfig.pageSize;
        uint16_t colOffset = 0;
        
        loadPageToCache(page);
        
        while (colOffset < pageSize)
        {
            uint16_t chunkSize = min((uint16_t)BUFFER_SIZE, (uint16_t)(pageSize - colOffset));
            uint16_t bytesRead = readFromCache(colOffset, dataBuffer, chunkSize);
            
            // Raw binary stream to host
            Serial.write(dataBuffer, bytesRead);
            
            colOffset += bytesRead;
            if (bytesRead < chunkSize) break;
        }
    }
    
    // Print completion message to signal end (though host script tracks exact bytes)
    Serial.println();
    Serial.println(F("DUMP COMPLETE"));
}

// Parse and execute command
void parseAndExecuteCommand(char *command)
{
    char *argv[MAX_ARGS];
    int argc = 0;

    char *token = strtok(command, " \t");
    while (token != NULL && argc < MAX_ARGS)
    {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }

    if (argc == 0)
        return;

    // Convert command to lowercase
    for (char *p = argv[0]; *p; ++p)
        *p = (char)tolower((unsigned char)*p);

    // -------------------------------------------------------------------------
    // NAND COMMANDS
    // -------------------------------------------------------------------------
    
    if (strcmp(argv[0], "readid") == 0)
    {
        readChipID();
    }
    else if (strcmp(argv[0], "readcache") == 0)
    {
        // Read from the currently-loaded NAND cache (must loadPageToCache first)
        if (argc == 3)
        {
            uint16_t colAddr = (uint16_t)strtoul(argv[1], NULL, 0);
            int size = atoi(argv[2]);
            if (size <= 0)
            {
                Serial.println(F("Invalid size"));
                return;
            }
            if (size > BUFFER_SIZE)
                size = BUFFER_SIZE;

            uint16_t bytesRead = readFromCache(colAddr, dataBuffer, (uint16_t)size);
            printBufferHex(dataBuffer, bytesRead);
        }
        else
        {
            Serial.println(F("Usage: readcache <colAddr> <size>"));
            Serial.println(F("  (reads from currently-loaded page cache)"));
        }
    }
    else if (strcmp(argv[0], "readpage") == 0)
    {
        // Full two-step page read: load page to cache, then read from cache
        if (argc >= 2)
        {
            uint32_t pageAddr = (uint32_t)strtoul(argv[1], NULL, 0);
            uint16_t colOffset = 0;
            int size = BUFFER_SIZE;
            
            if (argc >= 3)
                colOffset = (uint16_t)strtoul(argv[2], NULL, 0);
            if (argc >= 4) {
                size = atoi(argv[3]);
                if (size <= 0) {
                    Serial.println(F("Invalid size"));
                    return;
                }
                if (size > BUFFER_SIZE)
                    size = BUFFER_SIZE;
            }
            
            uint16_t bytesRead = readNandPage(pageAddr, colOffset, dataBuffer, (uint16_t)size);
            Serial.print(F("Page "));
            Serial.print(pageAddr);
            Serial.print(F(", col "));
            Serial.print(colOffset);
            Serial.print(F(", "));
            Serial.print(bytesRead);
            Serial.println(F(" bytes:"));
            printBufferHex(dataBuffer, bytesRead);
        }
        else
        {
            Serial.println(F("Usage: readpage <page> [colOffset] [size]"));
        }
    }
    else if (strcmp(argv[0], "pageread") == 0)
    {
        ThePageRead();
    }
    
    // -------------------------------------------------------------------------
    // NAND SIZE CONFIGURATION
    // -------------------------------------------------------------------------
    
    else if (strcmp(argv[0], "setsize") == 0)
    {
        if (argc == 2)
        {
            uint16_t sizeMB = (uint16_t)atoi(argv[1]);
            if (sizeMB == 0) {
                Serial.println(F("Invalid size. Use MB value (e.g., 128, 256, 512, 1024)"));
                return;
            }
            setNandSize(sizeMB);
            Serial.print(F("NAND size set to "));
            Serial.print(sizeMB);
            Serial.print(F("MB ("));
            Serial.print(getTotalPages());
            Serial.println(F(" pages)"));
        }
        else
        {
            Serial.println(F("Usage: setsize <MB>"));
            Serial.println(F("Example: setsize 128"));
        }
    }
    else if (strcmp(argv[0], "getsize") == 0)
    {
        Serial.print(F("Page size: "));
        Serial.println(nandConfig.pageSize);
        Serial.print(F("Pages/block: "));
        Serial.println(nandConfig.pagesPerBlock);
        Serial.print(F("Total blocks: "));
        Serial.println(nandConfig.totalBlocks);
        Serial.print(F("Total pages: "));
        Serial.println(getTotalPages());
        Serial.print(F("Total bytes: "));
        Serial.println(getTotalBytes());
    }
    
    // -------------------------------------------------------------------------
    // SD CARD COMMANDS
    // -------------------------------------------------------------------------
    
    else if (strcmp(argv[0], "initsd") == 0)
    {
        initSD();
        restoreBitBangSPI();
    }
    else if (strcmp(argv[0], "copytosd") == 0)
    {
        cmdCopyToSD(argc, argv);
    }
    
    // -------------------------------------------------------------------------
    // USB DUMP COMMAND
    // -------------------------------------------------------------------------
    
    else if (strcmp(argv[0], "dumptousb") == 0)
    {
        cmdDumpToUSB(argc, argv);
    }
    
    // -------------------------------------------------------------------------
    // HELP
    // -------------------------------------------------------------------------
    
    else if (strcmp(argv[0], "help") == 0)
    {
        Serial.println(F("=== NAND Commands ==="));
        Serial.println(F("  readid           - Read chip ID"));
        Serial.println(F("  readpage <page> [col] [size]"));
        Serial.println(F("                   - Read from NAND page"));
        Serial.println(F("  readcache <col> <size>"));
        Serial.println(F("                   - Read from loaded cache"));
        Serial.println(F("  pageread         - Quick page 0 read test"));
        Serial.println(F(""));
        Serial.println(F("=== Size Config ==="));
        Serial.println(F("  setsize <MB>     - Set NAND size (128,256,512,1024)"));
        Serial.println(F("  getsize          - Show current config"));
        Serial.println(F(""));
        Serial.println(F("=== Export ==="));
        Serial.println(F("  dumptousb        - Raw binary dump over Serial"));
        Serial.println(F(""));
        Serial.println(F("=== SD Card ==="));
        Serial.println(F("  initsd           - Initialize SD card"));
        Serial.println(F("  copytosd [file]  - Copy NAND to SD"));
    }
    else
    {
        Serial.println(F("Unknown command. Type 'help'."));
    }
}

void initInterface()
{
    Serial.println(F("Interface ready. Type 'help' for commands."));
}
