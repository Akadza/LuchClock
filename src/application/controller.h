#pragma once
#include <Arduino.h>
#include "../system_settings.h"
struct CommandResult { int status; const char* message; };
struct ModeState {
 String mode; bool manual, transition, autoReady, canForward, canReverse, canStop, canAuto, calibrationAllowed, settingsAllowed;
 unsigned long waitMs, motorLimitMs;
};
struct ControllerState {
 ModeState mode;
 String device;
 bool wifi;
 String ip;
 bool encoderConnected;
 bool encoderValid;
 int encoderAddress;
 int encoderRaw;
 int encoderPosition;
 int encoderRawPosition;
 int encoderMinutes;
 bool calibrationSaved;
 int calibrationOffset;
 int calibrationSignedOffset;
 unsigned long motorRun;
 unsigned long manualPause;
 bool fault;
 String faultReason;
 unsigned long uptime;
 bool ntpSynced;
 bool ntpRecent;
 String localTime;
 String lastSync;
 long ntpAge;
 String motorState;
 unsigned long motorWait;
 unsigned long modeWaitTotal;
 unsigned long motorWaitTotal;
};
class IController {
public:
 virtual ModeState state() const=0;
 virtual ControllerState snapshot() const=0;
 virtual RuntimeSettings settings() const=0;
 virtual bool settingsSaved() const=0;
 virtual CommandResult setMode(const String& mode)=0;
 virtual CommandResult command(const String& action)=0;
 virtual CommandResult calibrate()=0;
 virtual CommandResult resetFault()=0;
 virtual CommandResult changeSettings(const RuntimeSettings& next,bool save)=0;
};
IController& controller();
void controllerSetup();
void controllerLoop();
