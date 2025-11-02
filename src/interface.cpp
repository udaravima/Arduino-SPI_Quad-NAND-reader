#include "interface.h"
#include "api.h"
#include "interface.h"
#include "main.h"   // provides ThePageRead()
#include "spi_bang.h"
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

// Trim leading and trailing whitespace in-place
static void trimInPlace(char *s)
{
    // Trim leading
    char *start = s;
    while (*start && isspace((unsigned char)*start))
        start++;
    if (start != s)
        memmove(s, start, strlen(start) + 1);

    // Trim trailing
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
    {
        s[len - 1] = '\0';
        len--;
    }
}

// Called by Arduino core when serial data arrives (if supported)
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

// Parse a command (modifies the buffer) and execute it.
void parseAndExecuteCommand(char *command)
{
    char *argv[MAX_ARGS];
    int argc = 0;

    // Tokenize on spaces (simple). Multiple spaces are treated as separators.
    char *token = strtok(command, " \t");
    while (token != NULL && argc < MAX_ARGS)
    {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }

    if (argc == 0)
        return;

    // Make command lowercase for case-insensitive matching
    for (char *p = argv[0]; *p; ++p)
        *p = (char)tolower((unsigned char)*p);

    if (strcmp(argv[0], "readid") == 0)
    {
        readChipID();
    }
    else if (strcmp(argv[0], "readqspibytes") == 0)
    {
        if (argc == 3)
        {
            uint32_t address = (uint32_t)strtoul(argv[1], NULL, 0); // base 0 allows 0x
            int size = atoi(argv[2]);
            if (size <= 0)
            {
                Serial.println("Invalid size");
                return;
            }
            if (size > BUFFER_SIZE)
                size = BUFFER_SIZE;

            setCS(true);
            sendCmdSpi(0x6B); // Quad read command
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
            Serial.println("Usage: readQSpiBytes <address> <size>");
        }
    }
    else if (strcmp(argv[0], "pageread") == 0)
    {
        ThePageRead();
    }
    else if (strcmp(argv[0], "help") == 0)
    {
        Serial.println("Available commands:");
        Serial.println("  readID - Read chip identification");
        Serial.println("  readQSpiBytes <address> <size> - Read bytes using Quad SPI");
        Serial.println("  pageRead - Read a page from memory");
        Serial.println("  help - Show this help message");
    }
    else
    {
        Serial.println("Unknown command. Type 'help' for available commands.");
    }
}

void initInterface()
{
    Serial.println("Interface Initialized...");
    Serial.println("Type 'help' for available commands.");
}
