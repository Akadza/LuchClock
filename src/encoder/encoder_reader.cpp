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
static uint8_t encoderRaw = 255;
static int16_t encoderPosition = -1;
static uint8_t encoderCandidate = 255;
static uint8_t encoderStableCount = 0;
static position_math::MovementFilter filter(0, DEFAULT_ENCODER_RATE, DEFAULT_ENCODER_BUDGET, DEFAULT_ENCODER_ANOMALY_MS, DEFAULT_ENCODER_REVERSE);
static position_math::ClockTracker tracker(0);
static bool initialized = false;
static void loseObservation() {
  filter.invalidate();
  tracker.loseContinuity();
  initialized = false;
  encoderPosition = -1;
  encoderStableCount = 0;
  encoderCandidate = 255;
}
static unsigned long encoderLastRead = 0;
static unsigned long encoderLastProbe = 0;

static bool encoderProbe(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}
static void encoderFindDevice() {
  encoderAddress = 0;
  loseObservation();
  for (uint8_t addr = 0x20; addr <= 0x27; ++addr) {
    if (encoderProbe(addr)) { encoderAddress = addr; break; }
  }
  if (!encoderAddress) for (uint8_t addr = 0x38; addr <= 0x3f; ++addr) {
    if (encoderProbe(addr)) { encoderAddress = addr; break; }
  }
  if (encoderAddress) {
    Wire.beginTransmission(encoderAddress);
    Wire.write(0xff); // PCF8574 inputs (quasi-bidirectional pull-ups).
    if (Wire.endTransmission() != 0) encoderAddress = 0;
  }
  Serial.printf("[ENCODER] PCF8574 address: 0x%02X\n", encoderAddress);
}
void encoderSetup() {
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
  if (Wire.requestFrom((uint8_t)encoderAddress, (uint8_t)1) != 1) {
    encoderAddress = 0;
    loseObservation();
    return;
  }
  const uint8_t value = (uint8_t)Wire.read();
  encoderRaw = value;
  int16_t raw = -1;
  for (int i = 0; i < 128; ++i) {
    if (pgm_read_byte(&encoderCodes[i]) == value) { raw = i; break; }
  }
  uint16_t angle = 0;
  if (!position_math::normalizePosition(raw, 128, angle)) {
    encoderStableCount = 0;
    if (initialized) {
      filter.missing(elapsed);
      if (!filter.valid()) loseObservation();
    }
    return;
  }
  if (!initialized) {
    if (value != encoderCandidate) {
      encoderCandidate = value;
      encoderStableCount = 0;
    }
    if (++encoderStableCount < settingsCurrent().encoderStableSamples) return;
    filter = position_math::MovementFilter(angle, DEFAULT_ENCODER_RATE, DEFAULT_ENCODER_BUDGET, DEFAULT_ENCODER_ANOMALY_MS, DEFAULT_ENCODER_REVERSE);
    tracker.beginObservation(angle);
    encoderPosition = raw;
    initialized = true;
    return;
  }
  if (filter.update(angle, elapsed)) {
    if (!tracker.advance(filter.angle(), filter.lastDelta())) {
      loseObservation();
      return;
    }
    encoderPosition = filter.angle() / 512;
  }
  if (!filter.valid()) loseObservation();
}

bool encoderConnected() { return encoderAddress != 0; }
bool encoderValid() { return encoderAddress && encoderPosition >= 0; }
bool encoderHealthy() { return encoderValid() && filter.healthy(); }
int16_t encoderRawPosition(){return encoderValid()?encoderPosition:-1;}
int16_t encoderCalibratedPosition(){
  return encoderValid()?(encoderPosition+calibrationOffset())%128:-1;
}
int encoderMinutes(){
  const int16_t position=encoderCalibratedPosition();
  return position>=0?position*60/128:-1;
}

uint8_t encoderAddressValue(){return encoderAddress;}
uint8_t encoderRawValue(){return encoderRaw;}
