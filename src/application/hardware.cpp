#include "hardware.h"
#include <ESP8266WiFi.h>
#include <sys/time.h>
#include <time.h>
#include "../encoder/encoder_reader.h"
#include "../motor/motor_controller.h"
#include "../network/time_manager.h"
class EncoderAdapter final: public IEncoder {
public:
 bool encoderConnected() override {return ::encoderConnected();}
 bool encoderValid() override {return ::encoderValid();}
 int16_t encoderRawPosition() override {return ::encoderRawPosition();}
 int16_t encoderCalibratedPosition() override {return ::encoderCalibratedPosition();}
 int encoderMinutes() override {return ::encoderMinutes();}
 uint8_t address() override {return ::encoderAddress;}
 uint8_t raw() override {return ::encoderRaw;}
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
