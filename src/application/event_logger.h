#pragma once
#include <Arduino.h>
#include <time.h>
enum class EventCode { GENERAL, BOOT, NTP_SYNC, NTP_STALE, NTP_RESTORED, MODE_OFF, MODE_MANUAL, MODE_AUTO, MANUAL_COMMAND, CALIBRATION_SAVED, FAULT_RESET, SETTINGS_SAVED, SETTINGS_APPLIED, ENCODER_UNAVAILABLE, NO_MOTION, ENCODER_LOST, MOTOR_FAULT, FAULT_MODE_OFF, MANUAL_LIMIT };
enum class EventLevel { Info, Warning, Critical };
const char* eventCodeText(EventCode code);
const char* eventLevelText(EventLevel level);
struct EventEntry { unsigned long uptimeSeconds; time_t timestamp; String message; EventCode code; EventLevel level; };
void eventAdd(const String& message, EventCode code=EventCode::GENERAL, EventLevel level=EventLevel::Info);
void eventReset();
unsigned int eventSize();
const EventEntry& eventAt(unsigned int index);
