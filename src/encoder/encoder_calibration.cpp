#include "encoder_calibration.h"
#include "../system_settings.h"
#include "../storage/storage.h"
#include <time.h>
#include "../application/hardware.h"

struct CalibrationRecord {
  uint32_t magic;
  uint8_t version;
  uint8_t offset;
  uint8_t check;
  uint8_t reserved;
};
static const uint32_t CAL_MAGIC=0x4C43414CUL;
static const int CAL_EEPROM_OFFSET=80; // Отдельно от настроек (68 байт в v3).
static uint8_t storedOffset=0;
static bool storedValid=false;
static_assert(sizeof(CalibrationRecord)+CAL_EEPROM_OFFSET<=SETTINGS_EEPROM_BYTES,"Calibration exceeds EEPROM");

void calibrationSetup(){
  CalibrationRecord r={};
  storage().get(CAL_EEPROM_OFFSET,r);
  storedValid=(r.magic==CAL_MAGIC && r.version==1 && r.offset<128 &&
               r.check==(uint8_t)(r.offset ^ 0xA5));
  storedOffset=storedValid?r.offset:0;
  Serial.printf("[CAL] %s, offset=%u\n",storedValid?"saved":"not calibrated",storedOffset);
}
bool calibrationSaved(){return storedValid;}
uint8_t calibrationOffset(){return storedOffset;}
static uint8_t targetOffset(uint8_t minute,int16_t rawPosition){
  const uint16_t lower=(minute*128U+59U)/60U;
  const uint16_t upper=((minute+1U)*128U+59U)/60U-1U;
  const uint8_t target=(lower+upper)/2U;
  return (target+128U-rawPosition)%128U;
}
bool calibrationNeeded(uint8_t minute,int16_t rawPosition){
  if(minute>=60 || rawPosition<0 || rawPosition>=128)return false;
  return !storedValid || storedOffset!=targetOffset(minute,rawPosition);
}
bool calibrationSave(uint8_t minute,int16_t rawPosition){
  if(minute>=60 || rawPosition<0 || rawPosition>=128)return false;
  const uint8_t offset=targetOffset(minute,rawPosition);
  if(storedValid && storedOffset==offset)return true;
  CalibrationRecord previous={};
  storage().get(CAL_EEPROM_OFFSET,previous);
  CalibrationRecord r={CAL_MAGIC,1,offset,(uint8_t)(offset ^ 0xA5),0};
  storage().put(CAL_EEPROM_OFFSET,r);
  if(!storage().commit()){
    storage().put(CAL_EEPROM_OFFSET,previous);
    return false;
  }
  storedOffset=offset;
  storedValid=true;
  Serial.printf("[CAL] Saved offset=%u\n",storedOffset);
  return true;
}

int8_t calibrationSignedOffset(){return storedOffset>=64?(int16_t)storedOffset-128:storedOffset;}
bool calibrationTimeAllowed(uint8_t& minute){
  if(!clockTime().timeIsFresh())return false;
  uint32_t ms=0;
  if(!clockTime().localClock(ms,minute))return false;
  if((ms/1000UL)%60UL>=60UL-settingsCurrent().calibrationGuardSec)return false;
  return true;
}
