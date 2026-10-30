#pragma once

#include <Arduino.h>

void webSetup();
void webLoop();

void webAddEvent(const String& message);