#include <cstdlib>
#include <vector>
#include <array>
#include <algorithm>
#include <ctime>
#include "../../src/storage/storage.h"
#include "../../src/encoder/encoder_calibration.h"
#include "../../src/network/time_manager.h"
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
static uint64_t virtualMs=0;
static time_t epochBase=0;
static uint64_t epochAt=0;
static void (*syncCallback)()=nullptr;
static bool networkUp=true;
static std::vector<EventCode> eventCodes;
static unsigned highWrites=0;
unsigned long millis(){return nowMs;}
void digitalWrite(uint8_t pin,uint8_t value){CHECK(pin==MOTOR_OUTPUT_PIN);if(value==HIGH && output!=HIGH)++highWrites;output=value;}
void pinMode(uint8_t,uint8_t){}
// Virtual EEPROM: pending bytes and committed bytes are independent.
class MemoryStorage final:public IStorage {
public:
 std::array<uint8_t,128> persisted{},buffer{};
 bool fail=false;
 unsigned writes=0;
 MemoryStorage(){persisted.fill(255);buffer=persisted;}
 void begin(size_t size) override{CHECK(size==buffer.size());buffer=persisted;}
 void read(size_t a,void* data,size_t n) override{CHECK(a+n<=buffer.size());memcpy(data,buffer.data()+a,n);}
 void write(size_t a,const void* data,size_t n) override{CHECK(a+n<=buffer.size());memcpy(buffer.data()+a,data,n);}
 bool commit() override{++writes;if(fail)return false;persisted=buffer;return true;}
};
static MemoryStorage mem;
IStorage& storage(){return mem;}
// Replace only the hardware clock and SNTP callback registration.
extern "C" time_t time(time_t* out) noexcept {
 const time_t value=epochBase+static_cast<time_t>((virtualMs-epochAt)/1000);
 if(out)*out=value;
 return value;
}
void settimeofday_cb(void (*cb)()){syncCallback=cb;}
void configTime(const char*,const char*,const char*){}
void eventAdd(const String&,EventCode code,EventLevel){eventCodes.push_back(code);}
void eventReset(){eventCodes.clear();}
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
 uint8_t raw() override{return ::encoderRawValue();}
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
 bool timeIsValid() override{return ::timeIsValid();}
 bool timeIsFresh() override{return ::timeIsFresh();}
 unsigned long timeSyncAgeSeconds() override{return ::timeSyncAgeSeconds();}
 String timeLocalString() override{return ::timeLocalString();}
 bool localClock(uint32_t& ms,uint8_t& minute) override{
  const auto seconds=static_cast<uint32_t>(time(nullptr)%86400);
  ms=seconds*1000+static_cast<uint32_t>((virtualMs-epochAt)%1000);
  minute=(seconds/60)%60;return true;
 }
 String timeLastSyncString() override{return ::timeLastSyncString();}
};
ITimeProvider& clockTime(){static Time t;return t;}
class Network final:public INetworkStatus {
public:
 bool connected() override{return networkUp;}
 String ip() override{return "192.0.2.1";}
};
INetworkStatus& networkStatus(){static Network n;return n;}
class Calibration final:public ICalibration {
public:
 bool saved() override{return calibrationSaved();}
 uint8_t offset() override{return calibrationOffset();}
 int8_t signedOffset() override{return calibrationSignedOffset();}
 bool needed(uint8_t m,int16_t r) override{return calibrationNeeded(m,r);}
 bool save(uint8_t m,int16_t r) override{return calibrationSave(m,r);}
 bool timeAllowed(uint8_t& m) override{return calibrationTimeAllowed(m);}
};
ICalibration& calibration(){static Calibration c;return c;}
static void tick(uint32_t ms=25){virtualMs+=ms;nowMs+=ms;timeLoop();encoderLoop();motorLoop();controllerLoop();}
static void ticks(unsigned n){while(n--)tick();}
static void hold(unsigned ms){while(ms>=25){tick();ms-=25;}if(ms)tick(ms);}
static bool ok(CommandResult r){return r.code==ResultCode::Ok;}

// Independent known ACE-128 fixture, indexed by physical shaft position.
static const uint8_t codes[128]={
127,63,62,58,56,184,152,24,8,72,73,77,79,15,47,175,
191,159,31,29,28,92,76,12,4,36,164,166,167,135,151,215,
223,207,143,142,14,46,38,6,2,18,82,83,211,195,203,235,
239,231,199,71,7,23,19,3,1,9,41,169,233,225,229,245,
247,243,227,163,131,139,137,129,128,132,148,212,244,240,242,250,
251,249,241,209,193,197,196,192,64,66,74,106,122,120,121,125,
253,252,248,232,224,226,98,96,32,33,37,53,61,60,188,190,
254,126,124,116,112,113,49,48,16,144,146,154,158,30,94,95};
static unsigned rawPos=0;
static void position(unsigned p){rawPos=p%128;Wire.value=codes[rawPos];}
static void syncAt(unsigned h,unsigned m,unsigned s){
 epochBase=1764115200 - (1764115200 % 86400) + h*3600+m*60+s; // fixed synthetic date; TZ=UTC in CI
 epochAt=virtualMs;CHECK(syncCallback);syncCallback();tick();
}
static void checkInvariant(){
 const auto s=controller().snapshot();
 CHECK(s.encoderHealthy==encoderHealthy());
 CHECK(s.clockHourKnown==encoderHourKnown());
 CHECK(s.mode.mode==controller().state().mode);
 CHECK(!s.encoderHealthy || s.encoderValid);
 CHECK(!s.clockHourKnown || (s.encoderValid && s.clockPosition<786432 &&
       s.clockPosition%65536==static_cast<uint32_t>(encoderConfirmedAngle())));
 CHECK(output==LOW || (s.mode.mode=="manual" && s.encoderValid && !s.fault));
 CHECK(!s.fault || output==LOW);
 CHECK(!s.mode.canForward || (s.encoderHealthy && !s.fault));
 CHECK(!s.mode.autoReady || (s.clockHourKnown && s.encoderHealthy && s.ntpRecent && !s.fault));
}
static void startManual(unsigned h=5){
 CHECK(encoderHealthy());
 if(motorAutoFault())CHECK(ok(controller().resetFault()));
 CHECK(ok(controller().setMode("manual")));hold(1000);
 CHECK(ok(controller().setClockHour(h)));checkInvariant();
}
static void boot(unsigned p){
 CHECK(ok(controller().setMode("off")));motorSetup();
 Wire.connected=true;position(p);settingsSetup();calibrationSetup();encoderSetup();timeSetup();controllerSetup();
 hold(100);CHECK(encoderHealthy() && !encoderHourKnown() && output==LOW);
}
static void expectLost(bool moving){
 CHECK(!encoderHourKnown() && controller().state().mode=="off" && output==LOW);
 CHECK(motorAutoFault()==moving);
 if(moving)CHECK(String(motorFaultReason())=="encoder_lost");
}
static void scenario(const char* name){std::cout<<"SCENARIO "<<name<<std::endl;}
int main(){
 setenv("TZ","UTC",1);tzset();
 scenario("S01 boot without NTP, manual allowed, AUTO forbidden");
 boot(0);CHECK(!timeIsFresh() && !controller().state().canAuto);
 startManual();CHECK(!ok(controller().setMode("auto")));CHECK(ok(controller().command("forward")));
 CHECK(ok(controller().command("stop")));hold(1000);
 scenario("S02 synthetic NTP 11:59:50 and independent shaft hour");
 syncAt(11,59,50);CHECK(timeIsFresh() && clockTime().timeLocalString()=="11:59:50");
 CHECK(controller().state().canAuto && !calibrationSaved());
 CHECK(ok(controller().setMode("auto")));hold(1000);CHECK(controller().state().autoReady && output==LOW);
 hold(10000);CHECK(clockTime().timeLocalString()=="12:00:01" && encoderClockHour()==5);
 scenario("S03 twelve-hour physical run at 5.625 seconds per ACE step");
 CHECK(ok(controller().setMode("manual")));hold(1000);CHECK(ok(controller().setClockHour(0)));
 uint32_t expected=encoderClockPosition();
 for(unsigned i=0;i<1536;++i){
   hold(5625);position(rawPos+1);ticks(2);
   expected=(expected+512)%786432;
   CHECK(encoderHealthy() && encoderClockPosition()==expected);checkInvariant();
 }
 CHECK(expected==0 && encoderClockHour()==0);
 scenario("S04 normal speeds 5 and 6 seconds with motor active");
 RuntimeSettings cfg=settingsCurrent();cfg.manualLimitSec=900;cfg.noMotionWarnSec=15;
 CHECK(ok(controller().changeSettings(cfg,false)));CHECK(ok(controller().command("forward")));
 for(unsigned i=0;i<20;++i){hold(i%2?5000:6000);position(rawPos+1);ticks(2);CHECK(output==HIGH);checkInvariant();}
 CHECK(ok(controller().command("stop")));hold(1000);
 scenario("S05 one/two invalid or far valid samples; preserve hour and reject launches");
 for(unsigned count=1;count<=2;++count){
  for(uint8_t bad:{uint8_t(255),codes[(rawPos+20)%128]}){
   const auto c=encoderClockPosition();Wire.value=bad;ticks(count);
   CHECK(encoderValid() && !encoderHealthy() && encoderClockPosition()==c);
   CHECK(!ok(controller().command("forward")) && !ok(controller().setMode("auto")));
   position(rawPos);ticks(2);CHECK(encoderHealthy() && encoderClockPosition()==c);checkInvariant();
  }
 }
 scenario("S06 reverse jitter across 12-hour boundary, repeated returns");
 boot(127);syncAt(12,0,0);startManual(11);hold(6000);position(0);ticks(2);
 CHECK(encoderClockPosition()==0);
 for(int i=0;i<40;++i){position(127);tick();CHECK(encoderClockPosition()==0 && !encoderHealthy());
 position(0);tick();CHECK(encoderHealthy() && encoderClockPosition()==0);checkInvariant();}
 scenario("S07 long invalid while moving, exact timeout and latch");
 CHECK(ok(controller().command("forward")));Wire.value=255;tick();ticks(19);
 CHECK(encoderValid() && encoderHourKnown() && output==HIGH);
 tick();expectLost(true);CHECK(encoderConnected() && !encoderValid());
 CHECK(!ok(controller().resetFault()));position(0);ticks(2);
 CHECK(encoderHealthy() && motorAutoFault() && !encoderHourKnown());
 const auto starts=highWrites;CHECK(ok(controller().resetFault()));CHECK(highWrites==starts && output==LOW);
 scenario("S08 changing far/invalid/reverse/forward candidates never extend deadline");
 startManual();hold(10000);CHECK(ok(controller().command("forward")));
 for(unsigned i=0;i<20;++i){Wire.value=(i%4==0?codes[1]:i%4==1?codes[127]:i%4==2?255:codes[40]);tick();}
 CHECK(encoderValid() && output==HIGH);
 Wire.value=255;tick();expectLost(true);position(0);ticks(2);CHECK(ok(controller().resetFault()));
 scenario("S09 I2C failure stops immediately; restoration does not clear latch");
 startManual();CHECK(ok(controller().command("forward")));Wire.connected=false;tick();expectLost(true);
 CHECK(!encoderConnected());Wire.connected=true;hold(4975);CHECK(!encoderConnected());hold(100);
 CHECK(encoderHealthy() && motorAutoFault() && !encoderHourKnown());CHECK(ok(controller().resetFault()));
 scenario("S10 replay reported field log: large reverse, far forward, invalid 216");
 boot(113); // reported initial raw position inferred from the logged offset
 startManual();Wire.value=122;hold(600);expectLost(false);
 CHECK(encoderHealthy() && !encoderHourKnown()); // new baseline
 startManual();Wire.value=120;ticks(2);CHECK(encoderHealthy());
 Wire.value=248;hold(600);expectLost(false);
 startManual();Wire.value=216;hold(600);expectLost(false);
 position(7);ticks(2);CHECK(encoderHealthy() && !encoderHourKnown());
 scenario("S11 motion stops: confirmed steps reset no-motion timer; jitter does not");
 startManual();cfg=settingsCurrent();cfg.noMotionWarnSec=15;cfg.manualLimitSec=60;
 CHECK(ok(controller().changeSettings(cfg,false)));CHECK(ok(controller().command("forward")));
 for(int i=0;i<4;++i){hold(5600);position(rawPos+1);ticks(2);CHECK(output==HIGH);}
 hold(14975);CHECK(output==HIGH);tick();CHECK(output==LOW && String(motorFaultReason())=="no_motion");
 CHECK(ok(controller().resetFault()));
 scenario("S12 manual limit, pause snapshot, OFF and no spontaneous restart");
 startManual();cfg=settingsCurrent();cfg.manualLimitSec=2;cfg.manualPauseSec=5;cfg.noMotionWarnSec=60;
 CHECK(ok(controller().changeSettings(cfg,false)));CHECK(ok(controller().command("forward")));hold(1975);CHECK(output==HIGH);tick();
 CHECK(output==LOW && motorManualPauseRemainingMs()==5000);
 hold(1000);CHECK(!ok(controller().command("forward")) && !ok(controller().setMode("auto")));
 cfg.manualPauseSec=1;CHECK(ok(controller().changeSettings(cfg,false)));CHECK(motorManualPauseRemainingMs()==4000);
 CHECK(ok(controller().setMode("off")));hold(4000);CHECK(output==LOW && !motorManualPauseRemainingMs());
 scenario("S13 real time manager: stale threshold, clock jump, resync and Wi-Fi loss");
 startManual();cfg=settingsCurrent();cfg.ntpStaleSec=60;CHECK(ok(controller().changeSettings(cfg,false)));
 syncAt(8,30,0);CHECK(ok(controller().setMode("auto")));hold(1000);
 networkUp=false;hold(58950);CHECK(timeIsFresh());hold(50);CHECK(!timeIsFresh() && !controller().state().autoReady && output==LOW);
 CHECK(encoderHourKnown());CHECK(ok(controller().setMode("manual")));hold(1000);
 CHECK(ok(controller().command("forward")));CHECK(ok(controller().command("stop")));hold(1000);
 networkUp=true;syncAt(7,0,0);CHECK(timeIsFresh() && timeSyncAgeSeconds()==0 && encoderHourKnown() && output==LOW);
 scenario("S14 real calibration, guard boundary, EEPROM commit rollback, restart");
 boot(10);syncAt(9,15,50);CHECK(!ok(controller().calibrate()));
 syncAt(9,15,0);startManual();const auto oldOffset=calibrationOffset();
 mem.fail=true;CHECK(!ok(controller().calibrate()) && calibrationOffset()==oldOffset && encoderHourKnown());
 mem.fail=false;CHECK(ok(controller().calibrate()) && calibrationSaved() && !encoderHourKnown());
 CHECK(encoderMinutes()==15);const unsigned commits=mem.writes;
 CHECK(!ok(controller().calibrate()) && mem.writes==commits);
 const auto saved=calibrationOffset();cfg=settingsCurrent();cfg.ntpStaleSec=7200;
 CHECK(ok(controller().changeSettings(cfg,true)));cfg.ntpStaleSec=3600;CHECK(ok(controller().changeSettings(cfg,false)));
 boot(10);CHECK(calibrationSaved() && calibrationOffset()==saved && settingsCurrent().ntpStaleSec==7200 && !encoderHourKnown());
 scenario("S15 stable samples 2..5 and poll limits");
 for(unsigned n=2;n<=5;++n){
  cfg=settingsCurrent();cfg.encoderStableSamples=n;cfg.encoderPollMs=100;CHECK(ok(controller().changeSettings(cfg,false)));
  hold(1000);startManual();hold(6000);const auto c=encoderClockPosition();
  position(rawPos+1);hold(n*100);CHECK(encoderHealthy() && encoderClockPosition()==(c+512)%786432);checkInvariant();
  CHECK(ok(controller().setMode("off")));
 }
 scenario("S16 delayed poll loses continuity, no motion restart");
 cfg=settingsCurrent();cfg.encoderPollMs=25;cfg.encoderStableSamples=2;CHECK(ok(controller().changeSettings(cfg,false)));hold(1000);startManual();
 CHECK(ok(controller().command("forward")));tick(500);expectLost(true);
 std::cout<<"All 16 virtual system scenarios passed"<<std::endl;
}
