#pragma once
#include <Arduino.h>

// Shared encoder state: implementation lives in encoder_reader.cpp.
extern uint8_t encoderAddress;
extern uint8_t encoderRaw;
int16_t encoderRawPosition(); // Диагностика и контроль физического движения.
int16_t encoderCalibratedPosition(); // Рабочая позиция стрелки.

void encoderSetup();
void encoderLoop();
bool encoderConnected();
bool encoderValid();
int encoderMinutes();
