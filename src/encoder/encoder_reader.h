#pragma once
#include <Arduino.h>

// Shared encoder state: implementation lives in encoder_reader.cpp.
uint8_t encoderAddressValue();
uint8_t encoderRawValue();
int16_t encoderRawPosition(); // Диагностика и контроль физического движения.
int16_t encoderCalibratedPosition(); // Рабочая позиция стрелки.

void encoderSetup();
void encoderLoop();
bool encoderConnected();
bool encoderValid();
bool encoderHealthy();
bool encoderHourKnown();
int encoderClockHour();
uint32_t encoderClockPosition();
bool encoderSetHour(uint8_t hour);
void encoderCalibrationChanged();
int encoderMinutes();
