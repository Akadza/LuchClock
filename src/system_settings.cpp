#include "system_settings.h"
#include <EEPROM.h>
#include <stddef.h>
RuntimeSettings runtimeSettings;
struct StoredSettings {
  uint32_t magic;
  uint32_t version;
  RuntimeSettings values;
  uint32_t checksum;
};
static uint32_t checksum(const StoredSettings& s) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(&s);
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < offsetof(StoredSettings, checksum); ++i) {
    hash = (hash ^ p[i]) * 16777619UL;
  }
  return hash;
}
void settingsDefaults(RuntimeSettings& s) {
  s.manualLimitSec=DEFAULT_MANUAL_LIMIT_SEC;
  s.autoLimitSec=DEFAULT_AUTO_LIMIT_SEC;
  s.manualPauseSec=DEFAULT_MANUAL_PAUSE_SEC;
  s.calibrationGuardSec=DEFAULT_CALIBRATION_GUARD_SEC;
  s.noMotionWarnSec=DEFAULT_NO_MOTION_WARN_SEC;
  s.autoStepMin=DEFAULT_AUTO_STEP_MIN;
  s.wifiReconnectMs=DEFAULT_WIFI_RECONNECT_MS;
  s.wifiDiagnosticMs=DEFAULT_WIFI_DIAGNOSTIC_MS;
  s.ntpStaleSec=DEFAULT_NTP_STALE_SEC;
  s.encoderPollMs=DEFAULT_ENCODER_POLL_MS;
  s.encoderProbeMs=DEFAULT_ENCODER_PROBE_MS;
  s.encoderStableSamples=DEFAULT_ENCODER_STABLE_SAMPLES;
}
bool settingsValidate(const RuntimeSettings& s) {
  return s.manualLimitSec>=MIN_MOTOR_LIMIT_SEC && s.manualLimitSec<=MAX_MOTOR_LIMIT_SEC &&
    s.autoLimitSec>=MIN_MOTOR_LIMIT_SEC && s.autoLimitSec<=MAX_MOTOR_LIMIT_SEC &&
    s.manualPauseSec>=MIN_MANUAL_PAUSE_SEC && s.manualPauseSec<=MAX_MANUAL_PAUSE_SEC &&
    s.calibrationGuardSec<=MAX_CALIBRATION_GUARD_SEC &&
    s.noMotionWarnSec>=MIN_NO_MOTION_WARN_SEC && s.noMotionWarnSec<=MAX_NO_MOTION_WARN_SEC &&
    s.autoStepMin>=MIN_AUTO_STEP_MIN && s.autoStepMin<=MAX_AUTO_STEP_MIN &&
    s.wifiReconnectMs>=MIN_WIFI_RECONNECT_MS && s.wifiReconnectMs<=MAX_WIFI_RECONNECT_MS &&
    s.wifiDiagnosticMs>=MIN_WIFI_DIAGNOSTIC_MS && s.wifiDiagnosticMs<=MAX_WIFI_DIAGNOSTIC_MS &&
    s.ntpStaleSec>=MIN_NTP_STALE_SEC && s.ntpStaleSec<=MAX_NTP_STALE_SEC &&
    s.encoderPollMs>=MIN_ENCODER_POLL_MS && s.encoderPollMs<=MAX_ENCODER_POLL_MS &&
    s.encoderProbeMs>=MIN_ENCODER_PROBE_MS && s.encoderProbeMs<=MAX_ENCODER_PROBE_MS &&
    s.encoderStableSamples>=MIN_ENCODER_STABLE_SAMPLES && s.encoderStableSamples<=MAX_ENCODER_STABLE_SAMPLES;
}
void settingsSetup() {
  settingsDefaults(runtimeSettings);
  EEPROM.begin(SETTINGS_EEPROM_BYTES);
  StoredSettings stored={};
  EEPROM.get(0,stored);
  if (stored.magic==SETTINGS_MAGIC && stored.version==SETTINGS_FORMAT_VERSION &&
      stored.checksum==checksum(stored) && settingsValidate(stored.values)) {
    runtimeSettings=stored.values;
    Serial.println(F("[SETTINGS] Saved values loaded"));
  } else Serial.println(F("[SETTINGS] Defaults loaded"));
}
bool settingsSave() {
  StoredSettings stored={};
  stored.magic=SETTINGS_MAGIC;
  stored.version=SETTINGS_FORMAT_VERSION;
  stored.values=runtimeSettings;
  stored.checksum=checksum(stored);
  EEPROM.put(0,stored);
  return EEPROM.commit();
}
