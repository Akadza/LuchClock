#pragma once
#include <Arduino.h>
#include "../motor/motor_controller.h"
class IEncoder {
public:
 virtual ~IEncoder()=default;
 virtual bool encoderConnected()=0;
 virtual bool encoderValid()=0;
 virtual bool encoderHealthy()=0;
 virtual bool encoderHourKnown()=0;
 virtual int encoderClockHour()=0;
 virtual uint32_t encoderClockPosition()=0;
 virtual bool encoderSetHour(uint8_t hour)=0;
 virtual void encoderCalibrationChanged()=0;
 virtual int16_t encoderRawPosition()=0;
 virtual int32_t encoderConfirmedAngle()=0;
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
 virtual bool localClock(uint32_t& ms,uint8_t& minute)=0;
 virtual String timeLastSyncString()=0;
};
ITimeProvider& clockTime();

class INetworkStatus {
public:
 virtual ~INetworkStatus()=default;
 virtual bool connected()=0;
 virtual String ip()=0;
};
INetworkStatus& networkStatus();

class ICalibration {
public:
 virtual ~ICalibration()=default;
 virtual bool saved()=0;
 virtual uint8_t offset()=0;
 virtual int8_t signedOffset()=0;
 virtual bool needed(uint8_t minute,int16_t raw)=0;
 virtual bool save(uint8_t minute,int16_t raw)=0;
 virtual bool timeAllowed(uint8_t& minute)=0;
};
ICalibration& calibration();
