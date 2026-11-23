#include "encoder_calibration.h"
#include "encoder_reader.h"
#include "../system_settings.h"
#include <EEPROM.h>

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
static bool pending=false;
static uint8_t candidateOffset=0;
static uint8_t candidateMinute=0;
static int16_t candidatePosition=-1;

void calibrationSetup(){
  CalibrationRecord r={};
  EEPROM.get(CAL_EEPROM_OFFSET,r);
  storedValid=(r.magic==CAL_MAGIC && r.version==1 && r.offset<128 &&
               r.check==(uint8_t)(r.offset ^ 0xA5));
  storedOffset=storedValid?r.offset:0;
  pending=false;
  Serial.printf("[CAL] %s, offset=%u\n",storedValid?"saved":"not calibrated",storedOffset);
}
bool calibrationSaved(){return storedValid;}
uint8_t calibrationOffset(){return storedOffset;}
int calibrationMinutes(){
  if(!encoderValid())return -1;
  const uint8_t position=(uint8_t)((encoderPosition+storedOffset)%128);
  return position*60/128;
}
bool calibrationPreview(uint8_t minute,int16_t rawPosition,uint8_t& offset){
  if(minute>=60 || rawPosition<0 || rawPosition>=128)return false;
  // Середина интервала позиций, соответствующего данной минуте.
  // Это гарантирует floor(target*60/128)==minute.
  const uint16_t lower=(minute*128U+59U)/60U;
  const uint16_t upper=(((uint16_t)minute+1U)*128U+59U)/60U-1U;
  const uint8_t target=(uint8_t)((lower+upper)/2U);
  candidateOffset=(uint8_t)((target+128U-(uint16_t)rawPosition)%128U);
  candidateMinute=minute;
  candidatePosition=rawPosition;
  pending=true;
  offset=candidateOffset;
  return true;
}
bool calibrationCommit(uint8_t minute,int16_t rawPosition){
  if(!pending || minute!=candidateMinute || rawPosition!=candidatePosition)return false;
  CalibrationRecord r={CAL_MAGIC,1,candidateOffset,(uint8_t)(candidateOffset ^ 0xA5),0};
  EEPROM.put(CAL_EEPROM_OFFSET,r);
  if(!EEPROM.commit())return false;
  storedOffset=candidateOffset;
  storedValid=true;
  pending=false;
  Serial.printf("[CAL] Saved offset=%u\n",storedOffset);
  return true;
}
bool calibrationPending(){return pending;}
uint8_t calibrationPendingOffset(){return candidateOffset;}
void calibrationDiscard(){pending=false;}
