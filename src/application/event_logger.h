#pragma once
#include <Arduino.h>
#include <time.h>
struct EventEntry { unsigned long uptimeSeconds; time_t timestamp; String message; };
void eventAdd(const String& message);
void eventReset();
unsigned int eventSize();
const EventEntry& eventAt(unsigned int index);
