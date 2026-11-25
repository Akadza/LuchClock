#pragma once

#include <Arduino.h>
#include <time.h>

enum TimeEvent {
  TIME_EVENT_NONE,
  TIME_EVENT_FIRST_SYNC,
  TIME_EVENT_STALE,
  TIME_EVENT_RESTORED,
  TIME_EVENT_THRESHOLD_CHANGED
};

void timeSetup();
TimeEvent timeLoop();

bool timeIsValid();
bool timeIsFresh();

unsigned long timeSyncAgeSeconds();

String timeLocalString();
String timeLastSyncString();