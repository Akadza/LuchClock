#include "encoder_reader.h"
#include "encoder_calibration.h"
#include "movement_filter.h"
#include "clock_tracker.h"
#include "../system_settings.h"
#include <Wire.h>
#include <pgmspace.h>

// Bourns EAW0J-B24-AE0128L, ACE-128.
// Encoder p1..p8 -> PCF8574 P0..P7, common C -> GND.
// The manufacturer code table is NOT ordinary binary or Gray code.
// Position 0..127 -> approximate dial minutes 0..59 (no zero calibration).
static const uint8_t encoderCodes[128] PROGMEM = {
  127, 63, 62, 58, 56, 184, 152, 24, 8, 72, 73, 77, 79, 15, 47, 175,
  191, 159, 31, 29, 28, 92, 76, 12, 4, 36, 164, 166, 167, 135, 151, 215,
  223, 207, 143, 142, 14, 46, 38, 6, 2, 18, 82, 83, 211, 195, 203, 235,
  239, 231, 199, 71, 7, 23, 19, 3, 1, 9, 41, 169, 233, 225, 229, 245,
  247, 243, 227, 163, 131, 139, 137, 129, 128, 132, 148, 212, 244, 240, 242, 250,
  251, 249, 241, 209, 193, 197, 196, 192, 64, 66, 74, 106, 122, 120, 121, 125,
  253, 252, 248, 232, 224, 226, 98, 96, 32, 33, 37, 53, 61, 60, 188, 190,
  254, 126, 124, 116, 112, 113, 49, 48, 16, 144, 146, 154, 158, 30, 94, 95
};
static uint8_t encoderAddress = 0;
static int16_t encoderRaw = -1;
static uint16_t resolution = 0;
static const char* sensorType = "none";
static const char* magnetState = "not_applicable";
static uint16_t lockedResolution = 0;
static int16_t encoderPosition = -1;
static int16_t encoderCandidate = -1;
static uint8_t encoderStableCount = 0;
static position_math::MovementFilter filter(0, DEFAULT_ENCODER_RATE, DEFAULT_ENCODER_BUDGET, DEFAULT_ENCODER_ANOMALY_MS, DEFAULT_ENCODER_REVERSE);
static position_math::ClockTracker tracker(0);
static bool initialized = false;
static uint32_t activeStableSamples = 0;
static bool previousHealthy = false;
static uint16_t calibratedAngle(uint16_t rawAngle) {
  return position_math::applyOffset(rawAngle, static_cast<uint16_t>(uint32_t(calibrationOffsetFor(resolution)) * (resolution ? 65536UL/resolution : 0)));
}
static void loseObservation() {
  if (initialized) Serial.println(F("[ENCODER] Observation lost; hour unknown"));
  filter.invalidate();
  previousHealthy = false;
  tracker.loseContinuity();
  initialized = false;
  encoderPosition = -1;
  encoderStableCount = 0;
  encoderCandidate = -1;
}
static unsigned long encoderLastRead = 0;
static unsigned long encoderLastProbe = 0;

static bool encoderProbe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}
// AS5600 status and RAW ANGLE. No configuration/OTP writes.
static bool asRead(uint8_t reg,uint8_t* bytes,uint8_t n){
  Wire.beginTransmission(0x36);Wire.write(reg);
  if(Wire.endTransmission(false)!=0 || Wire.requestFrom(uint8_t(0x36),n)!=n)return false;
  for(uint8_t i=0;i<n;++i){const int v=Wire.read();if(v<0)return false;bytes[i]=uint8_t(v);}
  return true;
}
static void encoderFindDevice() {
  encoderAddress=0;resolution=0;encoderRaw=-1;loseObservation();
  sensorType="none";magnetState="not_applicable";
  uint8_t pcf=0;unsigned count=0;
  for(uint8_t a=0x20;a<=0x27;++a)if(encoderProbe(a)){pcf=a;++count;}
  for(uint8_t a=0x38;a<=0x3f;++a)if(encoderProbe(a)){pcf=a;++count;}
  const bool as=encoderProbe(0x36);
  if(count+(as?1:0)>1){sensorType="conflict";Serial.println(F("[ENCODER] Multiple sensors; OFF"));return;}
  const uint16_t found=as?4096:(pcf?128:0);
  if(!found)return;
  if(lockedResolution && found!=lockedResolution){sensorType="conflict";Serial.println(F("[ENCODER] Sensor changed; reboot required"));return;}
  if(as){
    uint8_t status=0;
    if(!asRead(0x0B,&status,1)){sensorType="as5600";magnetState="read_error";return;}
    encoderAddress=0x36;resolution=4096;sensorType="as5600";magnetState="checking";
  }else{
    Wire.beginTransmission(pcf);Wire.write(0xff);
    if(Wire.endTransmission()!=0)return;
    encoderAddress=pcf;resolution=128;sensorType="ace128";
  }
  lockedResolution=resolution;
  Serial.printf("[ENCODER] %s address: 0x%02X\n",sensorType,encoderAddress);
}
void encoderSetup() {
  lockedResolution=0;
  Wire.begin(4, 5); // NodeMCU D2=SDA, D1=SCL.
  Wire.setClock(100000);
  encoderFindDevice();
}
void encoderLoop() {
  const unsigned long now = millis();
  if (!encoderAddress) {
    if (now - encoderLastProbe >= DEFAULT_ENCODER_PROBE_MS) {
      encoderLastProbe = now;
      encoderFindDevice();
    }
    return;
  }
  if (now - encoderLastRead < settingsCurrent().encoderPollMs) return;
  const uint32_t elapsed = now - encoderLastRead;
  encoderLastRead = now;
  if (initialized && (elapsed >= DEFAULT_ENCODER_ANOMALY_MS || activeStableSamples != settingsCurrent().encoderStableSamples)) {
    Serial.println(F("[ENCODER] Observation interrupted"));
    loseObservation();
    return;
  }
  int16_t raw=-1;
  bool readOk=true;
  if(resolution==4096){
    uint8_t status=0, angleBytes[2]={};
    readOk=asRead(0x0B,&status,1)&&asRead(0x0C,angleBytes,2);
    if(readOk){
      encoderRaw=int16_t((uint16_t(angleBytes[0]&0x0f)<<8)|angleBytes[1]);
      magnetState=!(status&0x20)?"missing":(status&0x10)?"weak":(status&0x08)?"strong":"ok";
      if((status&0x38)==0x20)raw=encoderRaw;
    }else magnetState="read_error";
  }else{
    readOk=Wire.requestFrom((uint8_t)encoderAddress,(uint8_t)1)==1;
    if(readOk){
      const int v=Wire.read();encoderRaw=v;
      for(int i=0;i<128;++i)if(pgm_read_byte(&encoderCodes[i])==v){raw=i;break;}
    }
  }
  if (!readOk) {
    Serial.println(F("[ENCODER] I2C read failed"));
    encoderAddress = 0;
    encoderLastProbe = now;
    loseObservation();
    return;
  }
  const int16_t value=encoderRaw;
  uint16_t angle = 0;
  if (!position_math::normalizePosition(raw, resolution, angle)) {
    encoderStableCount = 0;
    if (initialized) {
      filter.missing(elapsed);
      if (previousHealthy) Serial.printf("[ENCODER] Invalid sensor sample=%u; holding confirmed angle=%u\n", unsigned(value), unsigned(filter.angle()));
      previousHealthy = false;
      if (!filter.valid()) loseObservation();
    }
    return;
  }
  angle = calibratedAngle(angle);
  if (!initialized) {
    if (value != encoderCandidate) {
      encoderCandidate = value;
      encoderStableCount = 0;
    }
    if (++encoderStableCount < settingsCurrent().encoderStableSamples) return;
    filter = position_math::MovementFilter(angle, DEFAULT_ENCODER_RATE, resolution==4096?128:DEFAULT_ENCODER_BUDGET, DEFAULT_ENCODER_ANOMALY_MS, resolution==4096?64:DEFAULT_ENCODER_REVERSE, settingsCurrent().encoderStableSamples, resolution==4096?48:512, resolution==4096?32:0);
    tracker.beginObservation(angle);
    encoderPosition = raw;
    activeStableSamples = settingsCurrent().encoderStableSamples;
    initialized = true;
    previousHealthy = true;
    Serial.printf("[ENCODER] Observation initialized; raw=%u confirmed=%u; hour unknown\n", unsigned(value), unsigned(angle));
    return;
  }
  if (filter.update(angle, elapsed)) {
    if (!tracker.advance(filter.angle(), filter.lastDelta())) {
      loseObservation();
      return;
    }
    encoderPosition = (filter.angle() / (65536UL/resolution) + resolution - calibrationOffsetFor(resolution)) % resolution;
  }
  if (previousHealthy != filter.healthy()) {
    Serial.printf("[ENCODER] health=%u raw=%u angle=%u confirmed=%u\n", unsigned(filter.healthy()), unsigned(value), unsigned(angle), unsigned(filter.angle()));
    previousHealthy = filter.healthy();
  }
  if (!filter.valid()) loseObservation();
}

bool encoderConnected() { return encoderAddress != 0; }
bool encoderValid() { return encoderAddress && encoderPosition >= 0; }
bool encoderHealthy() { return encoderValid() && filter.healthy(); }
bool encoderHourKnown() { return encoderValid() && tracker.known(); }
int encoderClockHour() { return encoderHourKnown() ? int(tracker.position() / 65536UL) : -1; }
uint32_t encoderClockPosition() { return encoderHourKnown() ? tracker.position() : 0; }
bool encoderSetHour(uint8_t hour) { return encoderHealthy() && tracker.setHour(hour); }
void encoderCalibrationChanged() {
  if (encoderValid()) {
    const uint16_t angle = calibratedAngle(static_cast<uint16_t>(uint32_t(encoderPosition) * (65536UL/resolution)));
    if (filter.rebase(angle)) tracker.beginObservation(angle);
    else loseObservation();
  }
  else tracker.loseContinuity();
}
int32_t encoderConfirmedAngle(){return encoderValid()?static_cast<int32_t>(filter.angle()):-1;}
int16_t encoderRawPosition(){return encoderValid()?encoderPosition:-1;}
int16_t encoderCalibratedPosition(){
  return encoderValid()?filter.angle()/(65536UL/resolution):-1;
}
int encoderMinutes(){
  const int16_t position=encoderCalibratedPosition();
  return position>=0?int32_t(position)*60/resolution:-1;
}

uint8_t encoderAddressValue(){return encoderAddress;}
int16_t encoderRawValue(){return encoderRaw;}
uint16_t encoderResolution(){return resolution;}
const char* encoderType(){return sensorType;}
const char* encoderMagnetState(){return magnetState;}
