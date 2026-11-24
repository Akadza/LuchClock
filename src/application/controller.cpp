#include "controller.h"
#include "../../config.h"
#include "event_logger.h"
#include "hardware.h"
#include "../encoder/encoder_calibration.h"
static String currentMode = "off";
static bool manualMode = false;
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
 return (currentMode=="manual" || currentMode=="off") && motor().motorState()==MOTOR_STOPPED && !modeTransition && !motor().motorAutoFault() && encoder().encoderValid() && clockTime().timeIsFresh();
}
static bool calibrationAllowed(){uint8_t minute=0;return calibrationReady() && calibrationTimeAllowed(minute) && calibrationNeeded(minute,encoder().encoderRawPosition());}
static bool settingsAllowed(){return currentMode!="auto" && motor().motorState()==MOTOR_STOPPED && !modeTransition;}
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
 s.encoderAddress=encoder().address();
 s.encoderRaw=encoder().raw();
 s.encoderPosition=encoder().encoderCalibratedPosition();
 s.encoderRawPosition=encoder().encoderRawPosition();
 s.encoderMinutes=encoder().encoderMinutes();
 s.calibrationSaved=calibrationSaved();
 uint8_t minute=0;
 s.localClockMs=0;
 const bool localValid=clockTime().localClock(s.localClockMs,minute);
 s.calibrationCurrent=localValid && clockTime().timeIsFresh() && encoder().encoderValid() && s.calibrationSaved && !calibrationNeeded(minute,encoder().encoderRawPosition());
 s.calibrationOffset=calibrationOffset();
 s.calibrationSignedOffset=(int)calibrationSignedOffset();
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
 return {currentMode,manualMode,modeTransition,currentMode=="auto" && autoReady && modeIsReady() && !motor().motorAutoFault(),
 manualMode && modeIsReady() && motor().motorCanForward(),manualMode && modeIsReady() && motor().motorCanReverse(),
 manualMode && (motor().motorState()==MOTOR_FORWARD || motor().motorState()==MOTOR_REVERSE),
 encoder().encoderValid() && motor().motorManualPauseRemainingMs()==0 && motor().motorState()==MOTOR_STOPPED && !modeTransition && !motor().motorAutoFault(),
 calibrationAllowed(),settingsAllowed(),modeWaitRemainingMs(),(manualMode?settingsCurrent().manualLimitSec:settingsCurrent().autoLimitSec)*1000UL,
 currentMode!="manual" && encoder().encoderValid() && !motor().motorAutoFault() && modeIsReady(),
 currentMode!="off", currentMode=="off" && encoder().encoderValid() && motor().motorAutoFault(), currentMode=="off" && motor().motorAutoFault()};
 }
 CommandResult setMode(const String& mode) override {
  if(mode==currentMode)return {200,""};
  if (mode != "off" && !encoder().encoderValid()) {
    return {409,"Энкодер не подключён: доступен только режим Выключен"};
  }

  if (mode == "auto" && motor().motorManualPauseRemainingMs()>0) {
    return {409,"Во время паузы после лимита автоматический режим запрещён"};
  }
  if (mode != "off" && motor().motorAutoFault()) {
    return {409,"Сначала сбросьте аварию в выключенном режиме"};
  }
  if (mode == "off") {
    motor().motorStop();currentMode="off";manualMode=false;autoReady=false;modeTransition=false;
    motor().motorSetAutoMode(false);
    eventAdd("Режим выключен","MODE_OFF","info");
  } else if (mode == "manual") {
    // Запрет переключения во время защитной паузы действует и на API.
    if (!manualMode && (modeTransition || motor().motorState() != MOTOR_STOPPED)) {
      return {409,"Дождитесь завершения защитной паузы и остановки привода"};
    }
    if (currentMode != "manual") {
      motor().motorStop();
      manualMode = true;
      currentMode = "manual";
      motor().motorSetAutoMode(false);
      autoReady = false;
      modeTransition = true;
      modeChangedAt = millis();
      eventAdd("Переключение в ручной режим","MODE_MANUAL","info");
      Serial.println("[MODE] Manual");
    }
  } else if (mode == "auto") {
    if (currentMode != "auto" && (motor().motorState() != MOTOR_STOPPED || modeTransition)) {
      return {409,"Сначала остановите привод и дождитесь завершения паузы"};
    }
    if (currentMode != "auto") {
      motor().motorStop();
      manualMode = false;
      currentMode = "auto";
      motor().motorSetAutoMode(true);
      autoReady = false;
      modeTransition = true;
      modeChangedAt = millis();
      eventAdd("Переключение в автоматический режим","MODE_AUTO","info");
      Serial.println("[MODE] Auto");
    }
  } else {
    return {400,"Invalid mode"};
  }

return {200,""};}
 CommandResult command(const String& action) override {
 if(currentMode!="manual")return {409,"Manual mode required"};
  if (action != "forward" &&
      action != "backward" &&
      action != "stop") {

    return {400,"Invalid action"};
  }

  if (action == "stop" && motor().motorState()!=MOTOR_FORWARD && motor().motorState()!=MOTOR_REVERSE) {
    return {409,"Двигатель уже остановлен"};
  }
  if (action != "stop" && (!modeIsReady() || !encoder().encoderValid())) {
    return {409,"Mode transition or motor cooldown active"};
  }
  if (!motor().motorCommand(action)) {
    return {409,"Motor command rejected (fault or invalid state)"};
  }

  eventAdd(String("Ручная команда: ") + action,"MANUAL_COMMAND");
return {200,""};}
 CommandResult calibrate() override {
 if(!calibrationReady())return {409,"Калибровка недоступна"};
 uint8_t minute=0;if(!calibrationTimeAllowed(minute))return {409,""};
 const int16_t raw=encoder().encoderRawPosition();
 if(raw<0 || raw>=128)return {409,"Калибровка недоступна"};
 if(!calibrationNeeded(minute,raw))return {409,""};
 if(!calibrationSave(minute,raw))return {500,"Не удалось сохранить калибровку"};
 eventAdd("Калибровка энкодера сохранена","CALIBRATION_SAVED","info");return {200,""};
 }
 CommandResult resetFault() override {
 if(currentMode!="off" || !encoder().encoderValid() || !motor().motorResetAutoFault())return {409,"Перейдите в Выключен, проверьте энкодер и наличие аварии"};
 eventAdd("Авария сброшена оператором","FAULT_RESET","info");return {200,""};
 }
 CommandResult changeSettings(const RuntimeSettings& next,bool save) override {
 if(!settingsAllowed())return {409,"Остановите двигатель и дождитесь окончания паузы"};
 if(!settingsValidate(next))return {400,"Некорректные параметры"};
 if(!settingsChange(next,save))return {500,"Не удалось сохранить настройки"};
 eventAdd(save?"Настройки сохранены":"Настройки применены временно",save?"SETTINGS_SAVED":"SETTINGS_APPLIED");return {200,""};
 }
};
static Controller instance;
IController& controller(){return instance;}
void controllerSetup(){
  manualMode = false;
  currentMode = "off";
  motor().motorSetAutoMode(false);
  modeTransition = false;
  autoReady = false;
  eventReset();
  previousAutoFault=false;
  previousManualPause=0;

  eventAdd("Контроллер запущен","BOOT","info");

}
void controllerLoop(){
  const bool encoderNowValid=encoder().encoderValid();
  if (!encoderNowValid && currentMode!="off") {
    // При потере датчика во время движения motorLoop уже фиксирует аварию.
    // Режим OFF сохраняет аварийную блокировку до ручного сброса.
    motor().motorStop();currentMode="off";manualMode=false;autoReady=false;
    modeTransition=false;motor().motorSetAutoMode(false);
    eventAdd("Энкодер недоступен: переход в Выключен","ENCODER_UNAVAILABLE","warning");
  }
  const bool fault=motor().motorAutoFault();
  if(fault && !previousAutoFault){
    String reason=motor().motorFaultReason();
    eventAdd(reason=="no_motion" ? "Двигатель остановлен: энкодер не фиксирует движение" :
      reason=="encoder_lost" ? "Потеря связи с энкодером во время движения" :
      "Критическая ошибка: привод заблокирован", reason=="no_motion"?"NO_MOTION":reason=="encoder_lost"?"ENCODER_LOST":"MOTOR_FAULT","critical");
  }
  if(fault && currentMode!="off"){
    motor().motorStop();currentMode="off";manualMode=false;autoReady=false;
    modeTransition=false;motor().motorSetAutoMode(false);
    eventAdd("Аварийный переход в Выключен","FAULT_MODE_OFF","critical");
  }
  previousAutoFault=fault;
  const unsigned long manualPause=motor().motorManualPauseRemainingMs();
  if(manualPause && !previousManualPause)eventAdd("Достигнут лимит ручной работы: защитная пауза","MANUAL_LIMIT","warning");
  previousManualPause=manualPause;
  if (modeTransition && motor().motorState() != MOTOR_WAITING &&
      (modeWaitRemainingMs() == 0 || motor().motorState() == MOTOR_FAULT)) {
    modeTransition = false;
    if (currentMode=="auto") {
      autoReady = (motor().motorState() == MOTOR_STOPPED);
      Serial.println(autoReady ? "[MODE] Auto ready" : "[MODE] Auto not ready");
    } else {
      Serial.println(F("[MODE] Manual controls unlocked"));
    }
  }
}
