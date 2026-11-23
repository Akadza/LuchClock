#pragma once
#include <Arduino.h>
void calibrationSetup();
bool calibrationSaved();
uint8_t calibrationOffset();
int calibrationMinutes();
bool calibrationPreview(uint32_t ntpMinuteEpoch, int16_t rawPosition, uint8_t& offset);
bool calibrationCommit(uint32_t ntpMinuteEpoch, int16_t rawPosition);
bool calibrationPending();
uint8_t calibrationPendingOffset();
void calibrationDiscard();
