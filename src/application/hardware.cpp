#include "hardware.h"
#include <ESP8266WiFi.h>
#include <sys/time.h>
#include <time.h>
#include "../encoder/encoder_reader.h"
#include "../encoder/encoder_calibration.h"
#include "../motor/motor_controller.h"
#include "../network/time_manager.h"
class EncoderAdapter final: public IEncoder {
public:
 bool encoderConnected() override {return ::encoderConnected();}
 bool encoderValid() override {return ::encoderValid();}
 bool encoderHealthy() override {return ::encoderHealthy();}
 bool encoderHourKnown() override {return ::encoderHourKnown();}
 int encoderClockHour() override {return ::encoderClockHour();}
 uint32_t encoderClockPosition() override {return ::encoderClockPosition();}
 bool encoderSetHour(uint8_t hour) override {return ::encoderSetHour(hour);}
 void encoderCalibrationChanged() override {::encoderCalibrationChanged();}
 int32_t encoderConfirmedAngle() override {return ::encoderConfirmedAngle();}
 int16_t encoderRawPosition() override {return ::encoderRawPosition();}
 int16_t encoderCalibratedPosition() override {return ::encoderCalibratedPosition();}
 int encoderMinutes() override {return ::encoderMinutes();}
 uint8_t address() override {return ::encoderAddressValue();}
 uint8_t raw() override {return ::encoderRawValue();}
};
IEncoder& encoder(){static EncoderAdapter instance;return instance;}
class MotorAdapter final: public IMotor {
public:
 MotorState motorState() override {return ::motorState();}
 void motorStop() override {::motorStop();}
 bool motorCommand(const String& action) override {return ::motorCommand(action);}
 const char* motorStateText() override {return ::motorStateText();}
 unsigned long motorWaitRemainingMs() override {return ::motorWaitRemainingMs();}
 bool motorCanForward() override {return ::motorCanForward();}
 bool motorCanReverse() override {return ::motorCanReverse();}
 unsigned long motorRunElapsedMs() override {return ::motorRunElapsedMs();}
 unsigned long motorManualPauseRemainingMs() override {return ::motorManualPauseRemainingMs();}
 bool motorAutoFault() override {return ::motorAutoFault();}
 bool motorResetAutoFault() override {return ::motorResetAutoFault();}
 void motorSetAutoMode(bool enabled) override {::motorSetAutoMode(enabled);}
 const char* motorFaultReason() override {return ::motorFaultReason();}
};
IMotor& motor(){static MotorAdapter instance;return instance;}
class TimeProviderAdapter final: public ITimeProvider {
public:
 bool timeIsValid() override {return ::timeIsValid();}
 bool timeIsFresh() override {return ::timeIsFresh();}
 unsigned long timeSyncAgeSeconds() override {return ::timeSyncAgeSeconds();}
 bool localClock(uint32_t& ms,uint8_t& minute) override {
  struct timeval tv={};
  if(gettimeofday(&tv,nullptr)!=0)return false;
  time_t seconds=tv.tv_sec;struct tm local={};
  if(!localtime_r(&seconds,&local))return false;
  minute=local.tm_min;
  ms=((uint32_t)local.tm_hour*3600UL+local.tm_min*60UL+local.tm_sec)*1000UL+tv.tv_usec/1000;
  return true;
 }
 String timeLocalString() override {return ::timeLocalString();}
 String timeLastSyncString() override {return ::timeLastSyncString();}
};
ITimeProvider& clockTime(){static TimeProviderAdapter instance;return instance;}

class NetworkStatusAdapter final:public INetworkStatus {
public:
 bool connected() override {return WiFi.status()==WL_CONNECTED;}
 String ip() override {return WiFi.localIP().toString();}
};
INetworkStatus& networkStatus(){static NetworkStatusAdapter instance;return instance;}

class CalibrationAdapter final:public ICalibration {
public:
 bool saved() override{return calibrationSaved();}
 uint8_t offset() override{return calibrationOffset();}
 int8_t signedOffset() override{return calibrationSignedOffset();}
 bool needed(uint8_t minute,int16_t raw) override{return calibrationNeeded(minute,raw);}
 bool save(uint8_t minute,int16_t raw) override{return calibrationSave(minute,raw);}
 bool timeAllowed(uint8_t& minute) override{return calibrationTimeAllowed(minute);}
};
ICalibration& calibration(){static CalibrationAdapter instance;return instance;}
