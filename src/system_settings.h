#pragma once
#include <Arduino.h>
// Все изменяемые тайминги LuchClock находятся здесь.
// Значения по умолчанию и допустимые пределы — единый источник.
#define MOTOR_OUTPUT_PIN 14
#define DEFAULT_MANUAL_LIMIT_SEC 900UL       // 15 мин
#define DEFAULT_AUTO_LIMIT_SEC 900UL         // 15 мин
#define DEFAULT_MANUAL_PAUSE_SEC 15UL        // 15 с после ручного лимита
#define DEFAULT_CALIBRATION_GUARD_SEC 10UL   // запрет секунд 50..59 перед сменой минуты
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
#define DEFAULT_WIFI_RECONNECT_MS 10000UL
#define DEFAULT_WIFI_DIAGNOSTIC_MS 15000UL
#define DEFAULT_NTP_STALE_SEC 86400UL
// Preliminary bench-only physical filter limits; verify on real mechanism.
#define DEFAULT_ENCODER_RATE 120UL
#define DEFAULT_ENCODER_BUDGET 1024U
#define DEFAULT_ENCODER_ANOMALY_MS 500UL
#define DEFAULT_ENCODER_REVERSE 512U
#define DEFAULT_ENCODER_POLL_MS 25UL
#define DEFAULT_ENCODER_PROBE_MS 5000UL
#define DEFAULT_ENCODER_STABLE_SAMPLES 2UL
#define MIN_WIFI_RECONNECT_MS 1000UL
#define MAX_WIFI_RECONNECT_MS 300000UL
#define MIN_WIFI_DIAGNOSTIC_MS 1000UL
#define MAX_WIFI_DIAGNOSTIC_MS 300000UL
#define MIN_NTP_STALE_SEC 60UL
#define MAX_NTP_STALE_SEC 86400UL
#define MIN_ENCODER_POLL_MS 5UL
#define MAX_ENCODER_POLL_MS 100UL
#define MIN_ENCODER_PROBE_MS 250UL
#define MAX_ENCODER_PROBE_MS 10000UL
#define MIN_ENCODER_STABLE_SAMPLES 2UL
#define MAX_ENCODER_STABLE_SAMPLES 5UL
#define DEFAULT_NTP_REQUEST_SEC 3600UL  // Период повторного запроса NTP
#define MIN_NTP_REQUEST_SEC 60UL
#define MAX_NTP_REQUEST_SEC 86400UL
#define SETTINGS_FORMAT_VERSION 3UL

struct RuntimeSettings {
  uint32_t manualLimitSec;
  uint32_t autoLimitSec;
  uint32_t manualPauseSec;
  uint32_t calibrationGuardSec;
  uint32_t noMotionWarnSec;
  uint32_t autoStepMin;
  uint32_t wifiReconnectMs;
  uint32_t wifiDiagnosticMs;
  uint32_t ntpStaleSec;
  uint32_t encoderPollMs;
  uint32_t encoderProbeMs;
  uint32_t encoderStableSamples;
  uint32_t ntpRequestSec;
};
const RuntimeSettings& settingsCurrent();
bool settingsChange(const RuntimeSettings& next,bool save);
void settingsSetup();
bool settingsSave();
bool settingsPersistedMatches();
bool settingsValidate(const RuntimeSettings& value);
void settingsDefaults(RuntimeSettings& value);

// Публичные настройки: ключ, подпись, пределы в внутренних единицах, масштаб UI.
#define PUBLIC_SETTINGS(X) \
 X(manualLimitSec,"Лимит ручного режима, сек",MIN_MOTOR_LIMIT_SEC,MAX_MOTOR_LIMIT_SEC,1) \
 X(autoLimitSec,"Лимит авторежима, сек",MIN_MOTOR_LIMIT_SEC,MAX_MOTOR_LIMIT_SEC,1) \
 X(manualPauseSec,"Пауза после ручного лимита, сек",MIN_MANUAL_PAUSE_SEC,MAX_MANUAL_PAUSE_SEC,1) \
 X(calibrationGuardSec,"Запрет калибровки перед сменой минуты, сек",0,MAX_CALIBRATION_GUARD_SEC,1) \
 X(noMotionWarnSec,"Аварийная остановка без движения энкодера, сек",MIN_NO_MOTION_WARN_SEC,MAX_NO_MOTION_WARN_SEC,1) \
 X(autoStepMin,"Интервал автоматической коррекции, мин",MIN_AUTO_STEP_MIN,MAX_AUTO_STEP_MIN,1) \
 X(wifiReconnectMs,"Повтор подключения Wi-Fi, сек",MIN_WIFI_RECONNECT_MS,MAX_WIFI_RECONNECT_MS,1000) \
 X(ntpStaleSec,"Допустимое время без синхронизации NTP, ч",MIN_NTP_STALE_SEC,MAX_NTP_STALE_SEC,3600) \
 X(encoderPollMs,"Опрос энкодера, мс",MIN_ENCODER_POLL_MS,MAX_ENCODER_POLL_MS,1) \
 X(encoderStableSamples,"Подтверждающие чтения энкодера",MIN_ENCODER_STABLE_SAMPLES,MAX_ENCODER_STABLE_SAMPLES,1)
