#pragma once
#include <Arduino.h>
#include <time.h>
struct EventEntry { unsigned long uptimeSeconds; time_t timestamp; String message; const char* code; const char* level; };
void eventAdd(const String& message, const char* code="GENERAL", const char* level="info");
void eventReset();
unsigned int eventSize();
const EventEntry& eventAt(unsigned int index);
