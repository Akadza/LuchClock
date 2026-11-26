#include <cstdlib>
#include <iostream>
#include <Arduino.h>
#include <Wire.h>
#include "../../src/application/controller.h"
#include "../../src/application/hardware.h"
#include "../../src/application/event_logger.h"
#include "../../src/encoder/encoder_reader.h"
#include "../../src/motor/motor_controller.h"
#define CHECK(x) do { if (!(x)) { std::cerr << "Failed line " << __LINE__ << ": " << #x << '\n'; std::exit(EXIT_FAILURE); } } while (false)
FakeSerial Serial;
FakeWire Wire;
static uint32_t nowMs=0;
static uint8_t output=LOW;
static uint8_t offset=1;
static bool fresh=true;
static RuntimeSettings settingsValue={};
unsigned long millis(){return nowMs;}
void digitalWrite(uint8_t pin,uint8_t value){CHECK(pin==MOTOR_OUTPUT_PIN);output=value;}
void pinMode(uint8_t,uint8_t){}
const RuntimeSettings& settingsCurrent(){return settingsValue;}
bool settingsPersistedMatches(){return false;}
bool settingsValidate(const RuntimeSettings&){return true;}
bool settingsChange(const RuntimeSettings& s,bool){settingsValue=s;return true;}
uint8_t calibrationOffset(){return offset;}
uint16_t calibrationOffsetFor(uint16_t){return offset;}
void eventAdd(const String&,EventCode,EventLevel){}
void eventReset(){}
class Encoder final:public IEncoder {
public:
 bool encoderConnected() override{return ::encoderConnected();}
 bool encoderValid() override{return ::encoderValid();}
 bool encoderHealthy() override{return ::encoderHealthy();}
 bool encoderHourKnown() override{return ::encoderHourKnown();}
 int encoderClockHour() override{return ::encoderClockHour();}
 uint32_t encoderClockPosition() override{return ::encoderClockPosition();}
 bool encoderSetHour(uint8_t h) override{return ::encoderSetHour(h);}
 void encoderCalibrationChanged() override{::encoderCalibrationChanged();}
 int16_t encoderRawPosition() override{return ::encoderRawPosition();}
 int32_t encoderConfirmedAngle() override{return ::encoderConfirmedAngle();}
 int16_t encoderCalibratedPosition() override{return ::encoderCalibratedPosition();}
 int encoderMinutes() override{return ::encoderMinutes();}
 uint8_t address() override{return ::encoderAddressValue();}
 int16_t raw() override{return ::encoderRawValue();}
 uint16_t resolution() override{return ::encoderResolution();}
 const char* type() override{return ::encoderType();}
 const char* magnetState() override{return ::encoderMagnetState();}
};
IEncoder& encoder(){static Encoder e;return e;}
class Motor final:public IMotor {
public:
 MotorState motorState() override{return ::motorState();}
 void motorStop() override{::motorStop();}
 bool motorCommand(const String& a) override{return ::motorCommand(a);}
 const char* motorStateText() override{return ::motorStateText();}
 unsigned long motorWaitRemainingMs() override{return ::motorWaitRemainingMs();}
 bool motorCanForward() override{return ::motorCanForward();}
 bool motorCanReverse() override{return ::motorCanReverse();}
 unsigned long motorRunElapsedMs() override{return ::motorRunElapsedMs();}
 unsigned long motorManualPauseRemainingMs() override{return ::motorManualPauseRemainingMs();}
 bool motorAutoFault() override{return ::motorAutoFault();}
 bool motorResetAutoFault() override{return ::motorResetAutoFault();}
 void motorSetAutoMode(bool b) override{::motorSetAutoMode(b);}
 const char* motorFaultReason() override{return ::motorFaultReason();}
};
IMotor& motor(){static Motor m;return m;}
class Time final:public ITimeProvider {
public:
 bool timeIsValid() override{return true;}
 bool timeIsFresh() override{return fresh;}
 unsigned long timeSyncAgeSeconds() override{return fresh?0:86400;}
 String timeLocalString() override{return "12:00:00";}
 bool localClock(uint32_t& ms,uint8_t& minute) override{ms=0;minute=0;return true;}
 String timeLastSyncString() override{return "";}
};
ITimeProvider& clockTime(){static Time t;return t;}
class Network final:public INetworkStatus {
public:
 bool connected() override{return true;}
 String ip() override{return "192.0.2.1";}
};
INetworkStatus& networkStatus(){static Network n;return n;}
class Calibration final:public ICalibration {
public:
 bool saved() override{return false;}
 uint16_t offset() override{return ::offset;}
 int16_t signedOffset() override{return ::offset;}
 bool needed(uint8_t,int16_t) override{return true;}
 bool save(uint8_t,int16_t) override{::offset=2;return true;}
 bool timeAllowed(uint8_t& m) override{m=0;return fresh;}
};
ICalibration& calibration(){static Calibration c;return c;}
static void tick(uint32_t ms=25){nowMs+=ms;encoderLoop();motorLoop();controllerLoop();}
static void ticks(unsigned n){while(n--)tick();}
static bool ok(CommandResult r){return r.code==ResultCode::Ok;}
int main(){
 settingsValue.encoderPollMs=25;settingsValue.encoderStableSamples=2;
 settingsValue.manualLimitSec=2;settingsValue.autoLimitSec=900;
 settingsValue.manualPauseSec=2;settingsValue.noMotionWarnSec=10;
 motorSetup();encoderSetup();controllerSetup();
 CHECK(output==LOW && controller().state().mode=="off");
 Wire.value=94; // raw 126 + offset 1 = 127
 ticks(2);CHECK(encoderHealthy() && encoderConfirmedAngle()==65024);
 CHECK(!controller().state().canAuto && !ok(controller().setMode("auto")));
 CHECK(ok(controller().setMode("manual")));ticks(40);
 CHECK(controller().state().canSetClockHour);
 CHECK(ok(controller().setClockHour(11)));
 CHECK(ok(controller().command("forward")) && output==HIGH);
 Wire.value=95; // raw 127 + offset 1 = 0
 tick();CHECK(!encoderHealthy() && encoderClockPosition()==786432-512 && output==HIGH);
 tick();CHECK(encoderHealthy() && encoderClockPosition()==0 && output==HIGH);
 CHECK(ok(controller().command("stop")));ticks(40);
 CHECK(output==LOW && controller().state().canAuto && !calibration().saved());
 CHECK(ok(controller().setMode("auto")));ticks(40);
 CHECK(controller().state().autoReady);
 fresh=false;tick();CHECK(!controller().state().autoReady && output==LOW);
 CHECK(ok(controller().setMode("manual")));ticks(40);
 CHECK(ok(controller().command("forward")) && output==HIGH);
 Wire.connected=false;tick();
 CHECK(output==LOW && motorAutoFault() && controller().state().mode=="off" && !encoderHourKnown());
 CHECK(!ok(controller().resetFault()));
 Wire.connected=true;ticks(210);
 CHECK(encoderHealthy() && !encoderHourKnown() && motorAutoFault());
 CHECK(ok(controller().resetFault()) && output==LOW && !motorAutoFault());
 CHECK(ok(controller().setMode("manual")));ticks(40);
 CHECK(ok(controller().command("forward")));ticks(80);
 CHECK(output==LOW && motorManualPauseRemainingMs()>0);
 ticks(40);CHECK(!ok(controller().command("forward")));
 ticks(40);CHECK(ok(controller().command("forward")));
 CHECK(ok(controller().command("stop")));ticks(40);
 fresh=true;CHECK(ok(controller().setClockHour(3)));
 CHECK(ok(controller().calibrate()) && !encoderHourKnown() && encoderHealthy());
 CHECK(output==LOW && encoderConfirmedAngle()==512);
 // A full budget before calibration must remain available immediately after it.
 Wire.value=127; // raw 0, calibrated 2
 ticks(2);CHECK(encoderHealthy() && encoderConfirmedAngle()==1024);
 CHECK(ok(controller().setClockHour(3)));
 const uint32_t baseline=encoderClockPosition();
 Wire.value=255; // invalid ACE code, PCF still responds
 tick();CHECK(encoderConnected() && encoderValid() && !encoderHealthy());
 Wire.value=127;ticks(2);CHECK(encoderHealthy() && encoderClockPosition()==baseline);
 Wire.value=255;ticks(21);
 CHECK(encoderConnected() && !encoderValid() && !encoderHourKnown() && controller().state().mode=="off");
 Wire.value=127;ticks(2);CHECK(encoderHealthy() && !encoderHourKnown());
 std::cout<<"Reader/motor/controller integration OK\n";
}
