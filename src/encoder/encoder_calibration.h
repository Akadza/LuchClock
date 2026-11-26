#pragma once
#include <Arduino.h>
void calibrationSetup();
bool calibrationSaved();
uint8_t calibrationOffset();
bool calibrationNeeded(uint8_t minute, int16_t rawPosition);
bool calibrationSave(uint8_t minute, int16_t rawPosition);

int8_t calibrationSignedOffset();
bool calibrationTimeAllowed(uint8_t& minute);

uint16_t calibrationOffsetFor(uint16_t resolution);
bool calibrationSavedFor(uint16_t resolution);
bool calibrationNeededFor(uint16_t resolution,uint8_t minute,int16_t raw);
bool calibrationSaveFor(uint16_t resolution,uint8_t minute,int16_t raw);
int16_t calibrationSignedOffsetFor(uint16_t resolution);
