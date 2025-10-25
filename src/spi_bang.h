#pragma once

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define CLK PB5
#define CS PB0
#define BDELAY 10 // microS

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