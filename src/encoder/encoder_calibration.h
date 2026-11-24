#pragma once
#include <Arduino.h>
void calibrationSetup();
bool calibrationSaved();
uint8_t calibrationOffset();
bool calibrationSave(uint8_t minute, int16_t rawPosition);

int8_t calibrationSignedOffset();
bool calibrationTimeAllowed(uint8_t& minute);
