#pragma once
#include <Arduino.h>
void calibrationSetup();
bool calibrationSaved();
uint8_t calibrationOffset();
int calibrationMinutes();
bool calibrationSave(uint8_t minute, int16_t rawPosition);
