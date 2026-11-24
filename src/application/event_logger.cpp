#include "event_logger.h"
#include "../../config.h"
#include "../system_settings.h"
#include "hardware.h"
#include <time.h>
static EventEntry events[MAX_LOG_ENTRIES];
static unsigned int eventCount = 0;
static unsigned int eventStart = 0;

void eventAdd(const String& message,EventCode code,EventLevel level) {
  if(eventCount==MAX_LOG_ENTRIES){eventStart=(eventStart+1)%MAX_LOG_ENTRIES;--eventCount;}
  EventEntry& entry=events[(eventStart+eventCount)%MAX_LOG_ENTRIES];

  entry.uptimeSeconds = millis() / 1000UL;
  entry.timestamp = clockTime().timeIsValid() ? time(nullptr) : 0;
  entry.message = message;
  entry.code=code;entry.level=level;

  eventCount++;
}

void eventReset(){eventCount=0;eventStart=0;}
unsigned int eventSize(){return eventCount;}
const EventEntry& eventAt(unsigned int index){return events[(eventStart+index)%MAX_LOG_ENTRIES];}

const char* eventCodeText(EventCode code){switch(code){case EventCode::GENERAL:return "GENERAL";case EventCode::BOOT:return "BOOT";case EventCode::NTP_SYNC:return "NTP_SYNC";case EventCode::NTP_STALE:return "NTP_STALE";case EventCode::NTP_RESTORED:return "NTP_RESTORED";case EventCode::MODE_OFF:return "MODE_OFF";case EventCode::MODE_MANUAL:return "MODE_MANUAL";case EventCode::MODE_AUTO:return "MODE_AUTO";case EventCode::MANUAL_COMMAND:return "MANUAL_COMMAND";case EventCode::CALIBRATION_SAVED:return "CALIBRATION_SAVED";case EventCode::FAULT_RESET:return "FAULT_RESET";case EventCode::SETTINGS_SAVED:return "SETTINGS_SAVED";case EventCode::SETTINGS_APPLIED:return "SETTINGS_APPLIED";case EventCode::ENCODER_UNAVAILABLE:return "ENCODER_UNAVAILABLE";case EventCode::NO_MOTION:return "NO_MOTION";case EventCode::ENCODER_LOST:return "ENCODER_LOST";case EventCode::MOTOR_FAULT:return "MOTOR_FAULT";case EventCode::FAULT_MODE_OFF:return "FAULT_MODE_OFF";case EventCode::MANUAL_LIMIT:return "MANUAL_LIMIT";}return "GENERAL";}
const char* eventLevelText(EventLevel level){switch(level){case EventLevel::Warning:return "warning";case EventLevel::Critical:return "critical";default:return "info";}}
