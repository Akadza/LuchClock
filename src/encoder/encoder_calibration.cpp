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
struct AsCalibrationRecord { uint32_t magic; uint16_t offset; uint16_t check; };
static const size_t AS_OFFSET=88;
static const uint32_t AS_MAGIC=0x41533536UL;
static uint16_t asOffset=0;
static bool asValid=false;
static_assert(AS_OFFSET+sizeof(AsCalibrationRecord)<=SETTINGS_EEPROM_BYTES,"AS calibration exceeds EEPROM");
static_assert(sizeof(CalibrationRecord)+CAL_EEPROM_OFFSET<=SETTINGS_EEPROM_BYTES,"Calibration exceeds EEPROM");

void calibrationSetup(){
  CalibrationRecord r={};
  storage().get(CAL_EEPROM_OFFSET,r);
  storedValid=(r.magic==CAL_MAGIC && r.version==1 && r.offset<128 &&
               r.check==(uint8_t)(r.offset ^ 0xA5));
  storedOffset=storedValid?r.offset:0;
  AsCalibrationRecord as={};storage().get(AS_OFFSET,as);
  asValid=as.magic==AS_MAGIC && as.offset<4096 && as.check==uint16_t(as.offset^0xA55A);
  asOffset=asValid?as.offset:0;
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

uint16_t calibrationOffsetFor(uint16_t res){return res==4096?asOffset:res==128?storedOffset:0;}
bool calibrationSavedFor(uint16_t res){return res==4096?asValid:res==128&&storedValid;}
static uint16_t asTarget(uint8_t minute,int16_t raw){
 const uint32_t lo=(uint32_t(minute)*4096+59)/60,hi=(uint32_t(minute+1)*4096+59)/60-1;
 return (lo+(hi-lo)/2+4096-raw)%4096;
}
bool calibrationNeededFor(uint16_t res,uint8_t minute,int16_t raw){
 if(res==128)return calibrationNeeded(minute,raw);
 return res==4096&&minute<60&&raw>=0&&raw<4096&&(!asValid||asOffset!=asTarget(minute,raw));
}
bool calibrationSaveFor(uint16_t res,uint8_t minute,int16_t raw){
 if(res==128)return calibrationSave(minute,raw);
 if(res!=4096||minute>=60||raw<0||raw>=4096)return false;
 const uint16_t offset=asTarget(minute,raw);
 if(asValid&&asOffset==offset)return true;
 AsCalibrationRecord prev={};storage().get(AS_OFFSET,prev);
 AsCalibrationRecord next={AS_MAGIC,offset,uint16_t(offset^0xA55A)};storage().put(AS_OFFSET,next);
 if(!storage().commit()){storage().put(AS_OFFSET,prev);return false;}
 asOffset=offset;asValid=true;return true;
}
int16_t calibrationSignedOffsetFor(uint16_t res){const int16_t v=calibrationOffsetFor(res);return res&&v>=res/2?v-int16_t(res):v;}
