#include "event_logger.h"
#include "../system_settings.h"
#include "../network/time_manager.h"
static EventEntry events[MAX_LOG_ENTRIES];
static unsigned int eventCount = 0;

void eventAdd(const String& message) {
  if (eventCount >= MAX_LOG_ENTRIES) {
    for (unsigned int i = 1; i < eventCount; i++) {
      events[i - 1] = events[i];
    }

    eventCount--;
  }

  EventEntry& entry = events[eventCount];

  entry.uptimeSeconds = millis() / 1000UL;
  entry.timestamp = timeIsValid() ? time(nullptr) : 0;
  entry.message = message;

  eventCount++;
}

void eventReset(){eventCount=0;}
unsigned int eventSize(){return eventCount;}
const EventEntry& eventAt(unsigned int index){return events[index];}
