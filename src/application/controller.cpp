#include "controller.h"
#include "../../config.h"
#include "event_logger.h"
#include "hardware.h"
enum class ClockMode { Off, Manual, Auto };
static ClockMode currentMode=ClockMode::Off;
static const char* modeText(){switch(currentMode){case ClockMode::Manual:return "manual";case ClockMode::Auto:return "auto";default:return "off";}}
static unsigned long modeChangedAt = 0;
static bool modeTransition = false;
static bool autoReady = false;
static bool previousAutoFault = false;
static unsigned long previousManualPause = 0;

static unsigned long modeWaitRemainingMs() {
  if (!modeTransition) return 0;
  unsigned long elapsed = millis() - modeChangedAt;
  return elapsed >= DEFAULT_MODE_DELAY_MS ? 0 : DEFAULT_MODE_DELAY_MS - elapsed;
}
static bool modeIsReady() {
  return !modeTransition && motor().motorState() == MOTOR_STOPPED;
}



static bool calibrationReady(){
 return (currentMode==ClockMode::Manual || currentMode==ClockMode::Off) && motor().motorState()==MOTOR_STOPPED && !modeTransition && !motor().motorAutoFault() && encoder().encoderValid() && clockTime().timeIsFresh();
}
static bool calibrationAllowed(){uint8_t minute=0;return calibrationReady() && calibration().timeAllowed(minute) && calibration().needed(minute,encoder().encoderRawPosition());}
static bool settingsAllowed(){return currentMode!=ClockMode::Auto && motor().motorState()==MOTOR_STOPPED && !modeTransition;}
class Controller final:public IController {
public:
 ControllerState snapshot() const override {
 ControllerState s;
 s.mode=state();
 s.device=DEVICE_NAME;
 s.wifi=networkStatus().connected();
 s.ip=networkStatus().ip();
 s.encoderConnected=encoder().encoderConnected();
 s.encoderValid=encoder().encoderValid();
 s.encoderHealthy=encoder().encoderHealthy();
 s.clockHourKnown=encoder().encoderHourKnown();
 s.clockHour=encoder().encoderClockHour();
 s.clockPosition=encoder().encoderClockPosition();
 s.encoderAddress=encoder().address();
 s.encoderRaw=encoder().raw();
 s.encoderPosition=encoder().encoderCalibratedPosition();
 s.encoderRawPosition=encoder().encoderRawPosition();
 s.encoderMinutes=encoder().encoderMinutes();
 s.calibrationSaved=calibration().saved();
 uint8_t minute=0;
 s.localClockMs=0;
 const bool localValid=clockTime().localClock(s.localClockMs,minute);
 s.calibrationCurrent=localValid && clockTime().timeIsFresh() && encoder().encoderValid() && s.calibrationSaved && !calibration().needed(minute,encoder().encoderRawPosition());
 s.calibrationOffset=calibration().offset();
 s.calibrationSignedOffset=(int)calibration().signedOffset();
 s.motorRun=motor().motorRunElapsedMs();
 s.manualPause=motor().motorManualPauseRemainingMs();
 s.fault=motor().motorAutoFault();
 s.faultReason=motor().motorFaultReason();
 s.uptime=millis() / 1000UL;
 s.ntpSynced=clockTime().timeIsValid();
 s.ntpRecent=clockTime().timeIsFresh();
 s.localTime=clockTime().timeLocalString();
 s.lastSync=clockTime().timeLastSyncString();
 s.ntpAge=(long)clockTime().timeSyncAgeSeconds();
 s.motorState=motor().motorStateText();
 s.motorWait=motor().motorWaitRemainingMs();
 s.modeWaitTotal=DEFAULT_MODE_DELAY_MS;
 s.motorWaitTotal=DEFAULT_MOTOR_STOP_DELAY_MS;
 return s;
 }
 RuntimeSettings settings() const override {return settingsCurrent();}
 bool settingsSaved() const override {return settingsPersistedMatches();}
 ModeState state() const override {
 return {modeText(),(currentMode==ClockMode::Manual),modeTransition,currentMode==ClockMode::Auto && autoReady && modeIsReady() && encoder().encoderHourKnown() && !motor().motorAutoFault(),
 (currentMode==ClockMode::Manual) && modeIsReady() && motor().motorCanForward(),(currentMode==ClockMode::Manual) && modeIsReady() && motor().motorCanReverse(),
 (currentMode==ClockMode::Manual) && (motor().motorState()==MOTOR_FORWARD || motor().motorState()==MOTOR_REVERSE),
 encoder().encoderHealthy() && encoder().encoderHourKnown() && motor().motorManualPauseRemainingMs()==0 && motor().motorState()==MOTOR_STOPPED && !modeTransition && !motor().motorAutoFault(),
 calibrationAllowed(),settingsAllowed(),modeWaitRemainingMs(),((currentMode==ClockMode::Manual)?settingsCurrent().manualLimitSec:settingsCurrent().autoLimitSec)*1000UL,
 currentMode!=ClockMode::Manual && encoder().encoderHealthy() && !motor().motorAutoFault() && modeIsReady(),
 currentMode!=ClockMode::Off, currentMode==ClockMode::Off && encoder().encoderValid() && motor().motorAutoFault(), currentMode==ClockMode::Off && motor().motorAutoFault()};
 }
 CommandResult setMode(const String& mode) override {
  if(mode==modeText())return {ResultCode::Ok,""};
  if (mode != "off" && !encoder().encoderValid()) {
    return {ResultCode::Blocked,"Энкодер не подключён: доступен только режим Выключен"};
  }

  if (mode == "auto" && !encoder().encoderHourKnown()) {
    return {ResultCode::Blocked,"Сначала задайте час в ручном режиме"};
  }
  if (mode == "auto" && motor().motorManualPauseRemainingMs()>0) {
    return {ResultCode::Blocked,"Во время паузы после лимита автоматический режим запрещён"};
  }
  if (mode != "off" && motor().motorAutoFault()) {
    return {ResultCode::Blocked,"Сначала сбросьте аварию в выключенном режиме"};
  }
  if (mode == "off") {
    motor().motorStop();currentMode=ClockMode::Off;autoReady=false;modeTransition=false;
    motor().motorSetAutoMode(false);
    eventAdd("Режим выключен",EventCode::MODE_OFF,EventLevel::Info);
  } else if (mode == "manual") {
    // Запрет переключения во время защитной паузы действует и на API.
    if (!(currentMode==ClockMode::Manual) && (modeTransition || motor().motorState() != MOTOR_STOPPED)) {
      return {ResultCode::Blocked,"Дождитесь завершения защитной паузы и остановки привода"};
    }
    if (currentMode!=ClockMode::Manual) {
      motor().motorStop();
      currentMode=ClockMode::Manual;
      motor().motorSetAutoMode(false);
      autoReady = false;
      modeTransition = true;
      modeChangedAt = millis();
      eventAdd("Переключение в ручной режим",EventCode::MODE_MANUAL,EventLevel::Info);
      Serial.println("[MODE] Manual");
    }
  } else if (mode == "auto") {
    if (currentMode!=ClockMode::Auto && (motor().motorState() != MOTOR_STOPPED || modeTransition)) {
      return {ResultCode::Blocked,"Сначала остановите привод и дождитесь завершения паузы"};
    }
    if (currentMode!=ClockMode::Auto) {
      motor().motorStop();
      currentMode=ClockMode::Auto;
      motor().motorSetAutoMode(true);
      autoReady = false;
      modeTransition = true;
      modeChangedAt = millis();
      eventAdd("Переключение в автоматический режим",EventCode::MODE_AUTO,EventLevel::Info);
      Serial.println("[MODE] Auto");
    }
  } else {
    return {ResultCode::Invalid,"Invalid mode"};
  }

return {ResultCode::Ok,""};}
 CommandResult command(const String& action) override {
 if(currentMode!=ClockMode::Manual)return {ResultCode::Blocked,"Manual mode required"};
  if (action != "forward" &&
      action != "backward" &&
      action != "stop") {

    return {ResultCode::Invalid,"Invalid action"};
  }

  if (action == "stop" && motor().motorState()!=MOTOR_FORWARD && motor().motorState()!=MOTOR_REVERSE) {
    return {ResultCode::Blocked,"Двигатель уже остановлен"};
  }
  if (action != "stop" && (!modeIsReady() || !encoder().encoderValid())) {
    return {ResultCode::Blocked,"Mode transition or motor cooldown active"};
  }
  if (!motor().motorCommand(action)) {
    return {ResultCode::Blocked,"Motor command rejected (fault or invalid state)"};
  }

  eventAdd(String("Ручная команда: ") + action,EventCode::MANUAL_COMMAND);
return {ResultCode::Ok,""};}
 CommandResult calibrate() override {
 if(!calibrationReady())return {ResultCode::Blocked,"Калибровка недоступна"};
 uint8_t minute=0;if(!calibration().timeAllowed(minute))return {ResultCode::Blocked,""};
 const int16_t raw=encoder().encoderRawPosition();
 if(raw<0 || raw>=128)return {ResultCode::Blocked,"Калибровка недоступна"};
 if(!calibration().needed(minute,raw))return {ResultCode::Blocked,""};
 if(!calibration().save(minute,raw))return {ResultCode::Failed,"Не удалось сохранить калибровку"};
 encoder().encoderCalibrationChanged();
 eventAdd("Калибровка энкодера сохранена",EventCode::CALIBRATION_SAVED,EventLevel::Info);return {ResultCode::Ok,""};
 }
 CommandResult setClockHour(uint8_t hour) override {
  if(hour>=12)return {ResultCode::Invalid,"Час должен быть от 0 до 11"};
  if(currentMode!=ClockMode::Manual || !modeIsReady() || motor().motorAutoFault() ||
     !encoder().encoderHealthy())return {ResultCode::Blocked,"Перейдите в ручной режим и остановите двигатель"};
  if(!encoder().encoderSetHour(hour))return {ResultCode::Failed,"Не удалось задать час"};
  eventAdd(String("Час задан оператором: ")+String(hour),EventCode::GENERAL,EventLevel::Info);
  return {ResultCode::Ok,""};
 }
 CommandResult resetFault() override {
 if(currentMode!=ClockMode::Off || !encoder().encoderValid() || !motor().motorResetAutoFault())return {ResultCode::Blocked,"Перейдите в Выключен, проверьте энкодер и наличие аварии"};
 eventAdd("Авария сброшена оператором",EventCode::FAULT_RESET,EventLevel::Info);return {ResultCode::Ok,""};
 }
 CommandResult changeSettings(const RuntimeSettings& next,bool save) override {
 if(!settingsAllowed())return {ResultCode::Blocked,"Остановите двигатель и дождитесь окончания паузы"};
 if(!settingsValidate(next))return {ResultCode::Invalid,"Некорректные параметры"};
 if(!settingsChange(next,save))return {ResultCode::Failed,"Не удалось сохранить настройки"};
 eventAdd(save?"Настройки сохранены":"Настройки применены временно",save?EventCode::SETTINGS_SAVED:EventCode::SETTINGS_APPLIED);return {ResultCode::Ok,""};
 }
};
static Controller instance;
IController& controller(){return instance;}
void controllerSetup(){
  currentMode=ClockMode::Off;
  motor().motorSetAutoMode(false);
  modeTransition = false;
  autoReady = false;
  eventReset();
  previousAutoFault=false;
  previousManualPause=0;

  eventAdd("Контроллер запущен",EventCode::BOOT,EventLevel::Info);

}
void controllerLoop(){
  const bool encoderNowValid=encoder().encoderValid();
  if (!encoderNowValid && currentMode!=ClockMode::Off) {
    // При потере датчика во время движения motorLoop уже фиксирует аварию.
    // Режим OFF сохраняет аварийную блокировку до ручного сброса.
    motor().motorStop();currentMode=ClockMode::Off;autoReady=false;
    modeTransition=false;motor().motorSetAutoMode(false);
    eventAdd("Энкодер недоступен: переход в Выключен",EventCode::ENCODER_UNAVAILABLE,EventLevel::Warning);
  }
  const bool fault=motor().motorAutoFault();
  if(fault && !previousAutoFault){
    String reason=motor().motorFaultReason();
    eventAdd(reason=="no_motion" ? "Двигатель остановлен: энкодер не фиксирует движение" :
      reason=="encoder_lost" ? "Потеря связи с энкодером во время движения" :
      "Критическая ошибка: привод заблокирован", reason=="no_motion"?EventCode::NO_MOTION:reason=="encoder_lost"?EventCode::ENCODER_LOST:EventCode::MOTOR_FAULT,EventLevel::Critical);
  }
  if(fault && currentMode!=ClockMode::Off){
    motor().motorStop();currentMode=ClockMode::Off;autoReady=false;
    modeTransition=false;motor().motorSetAutoMode(false);
    eventAdd("Аварийный переход в Выключен",EventCode::FAULT_MODE_OFF,EventLevel::Critical);
  }
  previousAutoFault=fault;
  const unsigned long manualPause=motor().motorManualPauseRemainingMs();
  if(manualPause && !previousManualPause)eventAdd("Достигнут лимит ручной работы: защитная пауза",EventCode::MANUAL_LIMIT,EventLevel::Warning);
  previousManualPause=manualPause;
  if (modeTransition && motor().motorState() != MOTOR_WAITING &&
      (modeWaitRemainingMs() == 0 || motor().motorState() == MOTOR_FAULT)) {
    modeTransition = false;
    if (currentMode==ClockMode::Auto) {
      autoReady = (motor().motorState() == MOTOR_STOPPED);
      Serial.println(autoReady ? "[MODE] Auto ready" : "[MODE] Auto not ready");
    } else {
      Serial.println(F("[MODE] Manual controls unlocked"));
    }
  }
}
