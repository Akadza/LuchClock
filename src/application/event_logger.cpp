#include "event_logger.h"
#include "../../config.h"
#include "../system_settings.h"
#include "hardware.h"
#include <time.h>
static EventEntry events[MAX_LOG_ENTRIES];
static unsigned int eventCount = 0;
static unsigned int eventStart = 0;

void eventAdd(const String& message,const char* code,const char* level) {
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
