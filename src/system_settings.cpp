#include "system_settings.h"
#include "storage/storage.h"
#include <stddef.h>
static RuntimeSettings runtimeSettings;
const RuntimeSettings& settingsCurrent(){return runtimeSettings;}
bool settingsChange(const RuntimeSettings& next,bool save){
 if(!settingsValidate(next))return false;
 const RuntimeSettings previous=runtimeSettings;
 runtimeSettings=next;
 if(save && !settingsSave()){runtimeSettings=previous;return false;}
 return true;
}
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
  s.ntpRequestSec=DEFAULT_NTP_REQUEST_SEC;
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
    s.encoderStableSamples>=MIN_ENCODER_STABLE_SAMPLES && s.encoderStableSamples<=MAX_ENCODER_STABLE_SAMPLES &&
    s.ntpRequestSec>=MIN_NTP_REQUEST_SEC && s.ntpRequestSec<=MAX_NTP_REQUEST_SEC;
}
struct V2Values {
  uint32_t manualLimitSec,autoLimitSec,manualPauseSec,calibrationGuardSec,noMotionWarnSec,autoStepMin;
  uint32_t wifiReconnectMs,wifiDiagnosticMs,ntpStaleSec,encoderPollMs,encoderProbeMs,encoderStableSamples;
};
struct V2Stored {uint32_t magic,version;V2Values values;uint32_t checksum;};
static uint32_t v2Checksum(const V2Stored& s){
  const uint8_t* p=reinterpret_cast<const uint8_t*>(&s);
  uint32_t hash=2166136261UL;
  for(size_t i=0;i<offsetof(V2Stored,checksum);++i)hash=(hash^p[i])*16777619UL;
  return hash;
}
struct LegacyValues {
  uint32_t manualLimitSec, autoLimitSec, manualPauseSec;
  uint32_t calibrationGuardSec, noMotionWarnSec, autoStepMin;
};
struct LegacyStored {
  uint32_t magic, version;
  LegacyValues values;
  uint32_t checksum;
};
static uint32_t legacyChecksum(const LegacyStored& s){
  const uint8_t* p=reinterpret_cast<const uint8_t*>(&s);
  uint32_t hash=2166136261UL;
  for(size_t i=0;i<offsetof(LegacyStored,checksum);++i)hash=(hash^p[i])*16777619UL;
  return hash;
}
void settingsSetup() {
  settingsDefaults(runtimeSettings);
  storage().begin(SETTINGS_EEPROM_BYTES);
  StoredSettings stored={};
  storage().get(0,stored);
  if (stored.magic==SETTINGS_MAGIC && stored.version==SETTINGS_FORMAT_VERSION &&
      stored.checksum==checksum(stored) && settingsValidate(stored.values)) {
    runtimeSettings=stored.values;
    Serial.println(F("[SETTINGS] Saved values loaded"));
  } else {
    V2Stored old2={};storage().get(0,old2);
    if(old2.magic==SETTINGS_MAGIC && old2.version==2UL && old2.checksum==v2Checksum(old2)){
      RuntimeSettings migrated=runtimeSettings;
      memcpy(&migrated,&old2.values,sizeof(V2Values));
      if(settingsValidate(migrated)){
        runtimeSettings=migrated;
        Serial.println(F("[SETTINGS] Migrated v2 settings in RAM"));
        return;
      }
    }
    LegacyStored legacy={};
    storage().get(0,legacy);
    if(legacy.magic==SETTINGS_MAGIC && legacy.version==1UL &&
       legacy.checksum==legacyChecksum(legacy)){
      RuntimeSettings migrated=runtimeSettings;
      migrated.manualLimitSec=legacy.values.manualLimitSec;
      migrated.autoLimitSec=legacy.values.autoLimitSec;
      migrated.manualPauseSec=legacy.values.manualPauseSec;
      migrated.calibrationGuardSec=legacy.values.calibrationGuardSec;
      migrated.noMotionWarnSec=legacy.values.noMotionWarnSec;
      migrated.autoStepMin=legacy.values.autoStepMin;
      if(settingsValidate(migrated)){
        runtimeSettings=migrated;
        Serial.println(F("[SETTINGS] Migrated v1 settings in RAM; save to persist v2"));
      }else Serial.println(F("[SETTINGS] Defaults loaded (legacy invalid)"));
    }else Serial.println(F("[SETTINGS] Defaults loaded"));
  }
}
bool settingsSave() {
  StoredSettings stored={};
  stored.magic=SETTINGS_MAGIC;
  stored.version=SETTINGS_FORMAT_VERSION;
  stored.values=runtimeSettings;
  stored.checksum=checksum(stored);
  storage().put(0,stored);
  return storage().commit();
}

bool settingsPersistedMatches(){
  StoredSettings stored={};
  storage().get(0,stored);
  return stored.magic==SETTINGS_MAGIC && stored.version==SETTINGS_FORMAT_VERSION &&
    stored.checksum==checksum(stored) &&
    memcmp(&stored.values,&runtimeSettings,sizeof(RuntimeSettings))==0;
}
