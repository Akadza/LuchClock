#pragma once
#include <Arduino.h>
void calibrationSetup();
bool calibrationSaved();
uint8_t calibrationOffset();
int calibrationMinutes();
bool calibrationPreview(uint8_t minute, int16_t rawPosition, uint8_t& offset);
bool calibrationCommit(uint8_t minute, int16_t rawPosition);
bool calibrationPending();
uint8_t calibrationPendingOffset();
void calibrationDiscard();
