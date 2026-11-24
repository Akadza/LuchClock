#include "controller.h"
#include <ESP8266WiFi.h>
#include "../../config.h"
#include "event_logger.h"
#include "../motor/motor_controller.h"
#include "../encoder/encoder_reader.h"
#include "../encoder/encoder_calibration.h"
#include "../network/time_manager.h"
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
  return !modeTransition && motorState() == MOTOR_STOPPED;
}



static bool calibrationReady(){
 return (currentMode=="manual" || currentMode=="off") && motorState()==MOTOR_STOPPED && !modeTransition && !motorAutoFault() && encoderValid() && timeIsFresh();
}
static bool calibrationAllowed(){uint8_t minute=0;return calibrationReady() && calibrationTimeAllowed(minute);}
static bool settingsAllowed(){return currentMode!="auto" && motorState()==MOTOR_STOPPED && !modeTransition;}
class Controller final:public IController {
public:
 ControllerState snapshot() const override {
 ControllerState s;
 s.mode=state();
 s.device=DEVICE_NAME;
 s.wifi=WiFi.status() == WL_CONNECTED;
 s.ip=WiFi.localIP().toString();
 s.encoderConnected=encoderConnected();
 s.encoderValid=encoderValid();
 s.encoderAddress=encoderAddress;
 s.encoderRaw=encoderRaw;
 s.encoderPosition=encoderCalibratedPosition();
 s.encoderRawPosition=encoderRawPosition();
 s.encoderMinutes=encoderMinutes();
 s.calibrationSaved=calibrationSaved();
 s.calibrationOffset=calibrationOffset();
 s.calibrationSignedOffset=(int)calibrationSignedOffset();
 s.motorRun=motorRunElapsedMs();
 s.manualPause=motorManualPauseRemainingMs();
 s.fault=motorAutoFault();
 s.faultReason=motorFaultReason();
 s.uptime=millis() / 1000UL;
 s.ntpSynced=timeIsValid();
 s.ntpRecent=timeIsFresh();
 s.localTime=timeLocalString();
 s.lastSync=timeLastSyncString();
 s.ntpAge=(long)timeSyncAgeSeconds();
 s.motorState=motorStateText();
 s.motorWait=motorWaitRemainingMs();
 s.modeWaitTotal=DEFAULT_MODE_DELAY_MS;
 s.motorWaitTotal=DEFAULT_MOTOR_STOP_DELAY_MS;
 return s;
 }
 RuntimeSettings settings() const override {return runtimeSettings;}
 bool settingsSaved() const override {return settingsPersistedMatches();}
 ModeState state() const override {
 return {currentMode,manualMode,modeTransition,currentMode=="auto" && autoReady && modeIsReady() && !motorAutoFault(),
 manualMode && modeIsReady() && motorCanForward(),manualMode && modeIsReady() && motorCanReverse(),
 manualMode && (motorState()==MOTOR_FORWARD || motorState()==MOTOR_REVERSE),
 encoderValid() && motorManualPauseRemainingMs()==0 && motorState()==MOTOR_STOPPED && !modeTransition && !motorAutoFault(),
 calibrationAllowed(),settingsAllowed(),modeWaitRemainingMs(),(manualMode?runtimeSettings.manualLimitSec:runtimeSettings.autoLimitSec)*1000UL};
 }
 CommandResult setMode(const String& mode) override {
  if(mode==currentMode)return {200,""};
  if (mode != "off" && !encoderValid()) {
    return {409,"Энкодер не подключён: доступен только режим Выключен"};
  }

  if (mode == "auto" && motorManualPauseRemainingMs()>0) {
    return {409,"Во время паузы после лимита автоматический режим запрещён"};
  }
  if (mode != "off" && motorAutoFault()) {
    return {409,"Сначала сбросьте аварию в выключенном режиме"};
  }
  if (mode == "off") {
    motorStop();currentMode="off";manualMode=false;autoReady=false;modeTransition=false;
    motorSetAutoMode(false);
    eventAdd("Режим выключен");
  } else if (mode == "manual") {
    // Запрет переключения во время защитной паузы действует и на API.
    if (!manualMode && (modeTransition || motorState() != MOTOR_STOPPED)) {
      return {409,"Дождитесь завершения защитной паузы и остановки привода"};
    }
    if (currentMode != "manual") {
      motorStop();
      manualMode = true;
      currentMode = "manual";
      motorSetAutoMode(false);
      autoReady = false;
      modeTransition = true;
      modeChangedAt = millis();
      eventAdd("Переключение в ручной режим");
      Serial.println("[MODE] Manual");
    }
  } else if (mode == "auto") {
    if (currentMode != "auto" && (motorState() != MOTOR_STOPPED || modeTransition)) {
      return {409,"Сначала остановите привод и дождитесь завершения паузы"};
    }
    if (currentMode != "auto") {
      motorStop();
      manualMode = false;
      currentMode = "auto";
      motorSetAutoMode(true);
      autoReady = false;
      modeTransition = true;
      modeChangedAt = millis();
      eventAdd("Переключение в автоматический режим");
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

  if (action == "stop" && motorState()!=MOTOR_FORWARD && motorState()!=MOTOR_REVERSE) {
    return {409,"Двигатель уже остановлен"};
  }
  if (action != "stop" && (!modeIsReady() || !encoderValid())) {
    return {409,"Mode transition or motor cooldown active"};
  }
  if (!motorCommand(action)) {
    return {409,"Motor command rejected (fault or invalid state)"};
  }

  eventAdd(String("Ручная команда: ") + action);
return {200,""};}
 CommandResult calibrate() override {
 if(!calibrationReady())return {409,"Калибровка недоступна"};
 uint8_t minute=0;if(!calibrationTimeAllowed(minute))return {409,""};
 if(!calibrationSave(minute,encoderRawPosition()))return {500,"Не удалось сохранить калибровку"};
 eventAdd("Калибровка энкодера сохранена");return {200,""};
 }
 CommandResult resetFault() override {
 if(currentMode!="off" || !encoderValid() || !motorResetAutoFault())return {409,"Перейдите в Выключен, проверьте энкодер и наличие аварии"};
 eventAdd("Авария сброшена оператором");return {200,""};
 }
 CommandResult changeSettings(const RuntimeSettings& next,bool save) override {
 if(!settingsAllowed())return {409,"Остановите двигатель и дождитесь окончания паузы"};
 if(!settingsValidate(next))return {400,"Некорректные параметры"};
 RuntimeSettings previous=runtimeSettings;runtimeSettings=next;
 if(save && !settingsSave()){runtimeSettings=previous;return {500,"Не удалось сохранить настройки"};}
 eventAdd(save?"Настройки сохранены":"Настройки применены временно");return {200,""};
 }
};
static Controller instance;
IController& controller(){return instance;}
void controllerSetup(){
  manualMode = false;
  currentMode = "off";
  motorSetAutoMode(false);
  modeTransition = false;
  autoReady = false;
  eventReset();
  previousAutoFault=false;
  previousManualPause=0;

  eventAdd("Контроллер запущен");

}
void controllerLoop(){
  const bool encoderNowValid=encoderValid();
  if (!encoderNowValid && currentMode!="off") {
    // При потере датчика во время движения motorLoop уже фиксирует аварию.
    // Режим OFF сохраняет аварийную блокировку до ручного сброса.
    motorStop();currentMode="off";manualMode=false;autoReady=false;
    modeTransition=false;motorSetAutoMode(false);
    eventAdd("Энкодер недоступен: переход в Выключен");
  }
  const bool fault=motorAutoFault();
  if(fault && !previousAutoFault){
    String reason=motorFaultReason();
    eventAdd(reason=="no_motion" ? "Двигатель остановлен: энкодер не фиксирует движение" :
      reason=="encoder_lost" ? "Потеря связи с энкодером во время движения" :
      "Критическая ошибка: привод заблокирован");
  }
  if(fault && currentMode!="off"){
    motorStop();currentMode="off";manualMode=false;autoReady=false;
    modeTransition=false;motorSetAutoMode(false);
    eventAdd("Аварийный переход в Выключен");
  }
  previousAutoFault=fault;
  const unsigned long manualPause=motorManualPauseRemainingMs();
  if(manualPause && !previousManualPause)eventAdd("Достигнут лимит ручной работы: защитная пауза");
  previousManualPause=manualPause;
  if (modeTransition && motorState() != MOTOR_WAITING &&
      (modeWaitRemainingMs() == 0 || motorState() == MOTOR_FAULT)) {
    modeTransition = false;
    if (currentMode=="auto") {
      autoReady = (motorState() == MOTOR_STOPPED);
      Serial.println(autoReady ? "[MODE] Auto ready" : "[MODE] Auto not ready");
    } else {
      Serial.println(F("[MODE] Manual controls unlocked"));
    }
  }
}
