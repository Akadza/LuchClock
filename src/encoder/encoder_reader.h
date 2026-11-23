#pragma once
#include <Arduino.h>

// Shared encoder state: implementation lives in encoder_reader.cpp.
extern uint8_t encoderAddress;
extern uint8_t encoderRaw;
extern int16_t encoderPosition;

void encoderSetup();
void encoderLoop();
bool encoderConnected();
bool encoderValid();
int encoderMinutes();
