#pragma once
#include <Arduino.h>
#include "../system_settings.h"
struct CommandResult { int status; const char* message; };
struct ModeState {
 String mode; bool manual, transition, autoReady, canForward, canReverse, canStop, canAuto, calibrationAllowed, settingsAllowed;
 unsigned long waitMs, motorLimitMs;
};
class IController {
public:
 virtual ModeState state() const=0;
 virtual CommandResult setMode(const String& mode)=0;
 virtual CommandResult command(const String& action)=0;
 virtual CommandResult calibrate()=0;
 virtual CommandResult resetFault()=0;
 virtual CommandResult changeSettings(const RuntimeSettings& next,bool save)=0;
};
IController& controller();
void controllerSetup();
void controllerLoop();
