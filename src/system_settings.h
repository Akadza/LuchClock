#pragma once
#include <Arduino.h>
// Все изменяемые тайминги LuchClock находятся здесь.
// Значения по умолчанию и допустимые пределы — единый источник.
#define MOTOR_OUTPUT_PIN 14
#define DEFAULT_MANUAL_LIMIT_SEC 900UL       // 15 мин
#define DEFAULT_AUTO_LIMIT_SEC 900UL         // 15 мин
#define DEFAULT_MANUAL_PAUSE_SEC 15UL        // 15 с после ручного лимита
#define DEFAULT_CALIBRATION_GUARD_SEC 10UL   // запрет секунд 00..09 и 50..59
#define DEFAULT_NO_MOTION_WARN_SEC 300UL     // 5 мин
#define DEFAULT_AUTO_STEP_MIN 2UL            // каждые 2 мин
#define DEFAULT_MOTOR_STOP_DELAY_MS 1000UL
#define DEFAULT_MODE_DELAY_MS 1000UL
#define MIN_MOTOR_LIMIT_SEC 1UL
#define MAX_MOTOR_LIMIT_SEC 900UL            // нельзя превышать 15 мин через сайт
#define MIN_MANUAL_PAUSE_SEC 1UL
#define MAX_MANUAL_PAUSE_SEC 300UL
#define MAX_CALIBRATION_GUARD_SEC 29UL
#define MIN_NO_MOTION_WARN_SEC 1UL
#define MAX_NO_MOTION_WARN_SEC 900UL
#define MIN_AUTO_STEP_MIN 1UL
#define MAX_AUTO_STEP_MIN 10UL
#define SETTINGS_EEPROM_BYTES 128
#define SETTINGS_MAGIC 0x4C434B31UL
#define SETTINGS_FORMAT_VERSION 1UL

struct RuntimeSettings {
  uint32_t manualLimitSec;
  uint32_t autoLimitSec;
  uint32_t manualPauseSec;
  uint32_t calibrationGuardSec;
  uint32_t noMotionWarnSec;
  uint32_t autoStepMin;
};
extern RuntimeSettings runtimeSettings;
void settingsSetup();
bool settingsSave();
bool settingsValidate(const RuntimeSettings& value);
void settingsDefaults(RuntimeSettings& value);
