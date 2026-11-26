#pragma once
#include <Arduino.h>

// Shared encoder state: implementation lives in encoder_reader.cpp.
uint8_t encoderAddressValue();
int16_t encoderRawValue();
uint16_t encoderResolution();
const char* encoderType();
const char* encoderMagnetState();
int16_t encoderRawPosition(); // Только диагностика и калибровка.
int32_t encoderConfirmedAngle(); // 0..65535, -1 при недостоверности.
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
