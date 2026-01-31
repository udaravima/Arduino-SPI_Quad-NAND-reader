#include "interface.h"
#include "config.h"
#include "main.h"
#include "spi_bang.h"
#include "sd_writer.h"
#include <Arduino.h>
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
        // Read page from NAND (in chunks due to RAM limit)
        uint16_t pageSize = nandConfig.pageSize;
        uint16_t offset = 0;
        
        while (offset < pageSize)
        {
            uint16_t chunkSize = min((uint16_t)BUFFER_SIZE, (uint16_t)(pageSize - offset));
            
            // Read chunk from NAND
            uint16_t bytesRead = readNandPage(page, dataBuffer, chunkSize);
            
            // Write to SD
            writeToFile(dataBuffer, bytesRead);
            
            offset += bytesRead;
            
            // Break if we read less than expected (shouldn't happen)
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
    Serial.println(F("Copy complete!"));
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
    else if (strcmp(argv[0], "readqspibytes") == 0)
    {
        if (argc == 3)
        {
            uint32_t address = (uint32_t)strtoul(argv[1], NULL, 0);
            int size = atoi(argv[2]);
            if (size <= 0)
            {
                Serial.println(F("Invalid size"));
                return;
            }
            if (size > BUFFER_SIZE)
                size = BUFFER_SIZE;

            setCS(true);
            sendCmdSpi(0x6B);
            sendCmdSpi((address >> 16) & 0xFF);
            sendCmdSpi((address >> 8) & 0xFF);
            sendCmdSpi(address & 0xFF);
            sendDummites(8);
            uint16_t bytesRead = readQSpiBytes(dataBuffer, (uint16_t)size);
            printBufferHex(dataBuffer, bytesRead);
            setCS(false);
        }
        else
        {
            Serial.println(F("Usage: readQSpiBytes <address> <size>"));
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
    }
    else if (strcmp(argv[0], "copytosd") == 0)
    {
        cmdCopyToSD(argc, argv);
    }
    
    // -------------------------------------------------------------------------
    // HELP
    // -------------------------------------------------------------------------
    
    else if (strcmp(argv[0], "help") == 0)
    {
        Serial.println(F("=== NAND Commands ==="));
        Serial.println(F("  readid           - Read chip ID"));
        Serial.println(F("  readqspibytes <addr> <size>"));
        Serial.println(F("  pageread         - Quick page read test"));
        Serial.println(F(""));
        Serial.println(F("=== Size Config ==="));
        Serial.println(F("  setsize <MB>     - Set NAND size (128,256,512,1024)"));
        Serial.println(F("  getsize          - Show current config"));
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
