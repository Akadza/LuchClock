#pragma once
#include <Arduino.h>
#include "../motor/motor_controller.h"
class IEncoder {
public:
 virtual ~IEncoder()=default;
 virtual bool encoderConnected()=0;
 virtual bool encoderValid()=0;
 virtual int16_t encoderRawPosition()=0;
 virtual int16_t encoderCalibratedPosition()=0;
 virtual int encoderMinutes()=0;
 virtual uint8_t address()=0;
 virtual uint8_t raw()=0;
};
IEncoder& encoder();
class IMotor {
public:
 virtual ~IMotor()=default;
 virtual MotorState motorState()=0;
 virtual void motorStop()=0;
 virtual bool motorCommand(const String& action)=0;
 virtual const char* motorStateText()=0;
 virtual unsigned long motorWaitRemainingMs()=0;
 virtual bool motorCanForward()=0;
 virtual bool motorCanReverse()=0;
 virtual unsigned long motorRunElapsedMs()=0;
 virtual unsigned long motorManualPauseRemainingMs()=0;
 virtual bool motorAutoFault()=0;
 virtual bool motorResetAutoFault()=0;
 virtual void motorSetAutoMode(bool enabled)=0;
 virtual const char* motorFaultReason()=0;
};
IMotor& motor();
class ITimeProvider {
public:
 virtual ~ITimeProvider()=default;
 virtual bool timeIsValid()=0;
 virtual bool timeIsFresh()=0;
 virtual unsigned long timeSyncAgeSeconds()=0;
 virtual String timeLocalString()=0;
 virtual String timeLastSyncString()=0;
};
ITimeProvider& clockTime();
