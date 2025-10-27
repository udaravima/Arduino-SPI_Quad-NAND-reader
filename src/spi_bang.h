#pragma once

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define CLK PB5
#define CS PB0
#define BDELAY 10       // microS
#define BUFFER_SIZE 256 // Buffer size for data storage

uint8_t fbang(uint8_t data);
void setCS(bool val);
void sendBang(uint8_t data);
uint8_t readBang();
void sendDummites(uint8_t count);
void sendCmdSpi(uint8_t data);
uint8_t readQSpiByte();
uint8_t readSpiByte();
uint8_t readDSpiByte();
void readChipID();

// Buffer management functions
extern uint8_t dataBuffer[BUFFER_SIZE];
uint16_t readQSpiBytes(uint8_t *buffer, uint16_t length);
uint16_t readDSpiBytes(uint8_t *buffer, uint16_t length);
uint16_t readSpiBytes(uint8_t *buffer, uint16_t length);
void printBufferHex(const uint8_t *buffer, uint16_t length);