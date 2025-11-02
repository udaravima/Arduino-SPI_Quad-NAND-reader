#pragma once

#include <Arduino.h>
#include "spi_bang.h"
#include "main.h"

void initInterface();
void processCommand(char* command);
void parseAndExecuteCommand(char* command);
void parseAndExecuteCommand(char *command);