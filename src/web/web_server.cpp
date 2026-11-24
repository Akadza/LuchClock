#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>

#include "../../config.h"
#include "../network/time_manager.h"
#include "web_server.h"
#include "web_ui.h"
#include "../motor/motor_controller.h"
#include "../system_settings.h"
#include "../encoder/encoder_reader.h"
#include "../encoder/encoder_calibration.h"

static ESP8266WebServer server(WEB_PORT);

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


struct EventEntry {
  unsigned long uptimeSeconds;
  time_t timestamp;
  String message;
};

static EventEntry events[MAX_LOG_ENTRIES];
static unsigned int eventCount = 0;

String jsonEscape(const String& source) {
  String result;
  result.reserve(source.length() + 8);

  for (unsigned int i = 0; i < source.length(); i++) {
    char c = source[i];

    if (c == '"' || c == '\\') {
      result += '\\';
      result += c;
    } else if (c == '\n') {
      result += F("\\n");
    } else if (c == '\r') {
      result += F("\\r");
    } else if (c == '\t') {
      result += F("\\t");
    } else if ((unsigned char)c >= 0x20) {
      result += c;
    }
  }

  return result;
}

String formatEventTime(time_t timestamp) {
  if (timestamp < 1700000000) {
    return "";
  }

  struct tm localTime;

  if (localtime_r(&timestamp, &localTime) == nullptr) {
    return "";
  }

  char buffer[32];

  strftime(
    buffer,
    sizeof(buffer),
    "%d.%m.%Y %H:%M:%S",
    &localTime
  );

  return String(buffer);
}

void webAddEvent(const String& message) {
  if (eventCount >= MAX_LOG_ENTRIES) {
    for (unsigned int i = 1; i < eventCount; i++) {
      events[i - 1] = events[i];
    }

    eventCount--;
  }

  EventEntry& entry = events[eventCount];

  entry.uptimeSeconds = millis() / 1000UL;
  entry.timestamp = timeIsValid() ? time(nullptr) : 0;
  entry.message = message;

  eventCount++;
}

void handleRoot() {
  server.send_P(
    200,
    "text/html; charset=utf-8",
    WEB_UI
  );
}

static bool calibrationAllowed();
void handleStatus() {
  String json;
  json.reserve(560);

  json += F("{\"device\":\"");json+=jsonEscape(DEVICE_NAME);json+=F("\",");
  json += F("\"version\":\"1.11\",");

  json += F("\"wifi\":");
  json += (WiFi.status() == WL_CONNECTED) ? "true" : "false";

  json += F(",\"ip\":\"");
  json += WiFi.localIP().toString();

  json += F("\",\"mode\":\"");
  json += currentMode;

  json += F("\",\"simulation\":false");

  json += F(",\"encoder_connected\":");
  json += encoderConnected() ? "true" : "false";
  json += F(",\"encoder_valid\":");
  json += encoderValid() ? "true" : "false";
  json += F(",\"encoder_address\":");
  json += String(encoderAddress);
  json += F(",\"encoder_raw\":");
  json += String(encoderRaw);
  json += F(",\"encoder_position\":");
  json += String(encoderCalibratedPosition());
  json += F(",\"encoder_raw_position\":");json += String(encoderRawPosition());
  json += F(",\"calibration_allowed\":");json += calibrationAllowed()?"true":"false";
  json += F(",\"encoder_minutes\":");
  json += String(encoderMinutes());
  json += F(",\"calibration_saved\":");json += calibrationSaved()?"true":"false";
  json += F(",\"calibration_offset\":");json += String(calibrationOffset());

  json += F(",\"motor_run_ms\":");json += String(motorRunElapsedMs());
  json += F(",\"motor_limit_ms\":");json += String((manualMode?runtimeSettings.manualLimitSec:runtimeSettings.autoLimitSec)*1000UL);
  json += F(",\"manual_pause_ms\":");json += String(motorManualPauseRemainingMs());
  json += F(",\"auto_fault\":");json += motorAutoFault()?"true":"false";
  json += F(",\"fault_reason\":\"");json += motorFaultReason();json += F("\"");
  json += F(",\"uptime_seconds\":");
  json += String(millis() / 1000UL);

  json += F(",\"ntp_synced\":");
  json += timeIsValid() ? "true" : "false";

  json += F(",\"ntp_recent\":");
  json += timeIsFresh() ? "true" : "false";

  json += F(",\"time_local\":\"");
  json += timeLocalString();

  json += F("\",\"last_ntp_sync\":\"");
  json += timeLastSyncString();

  json += F("\",\"ntp_age_seconds\":");
  json += String(timeSyncAgeSeconds());

  json += F(",\"motor_state\":\"");
  json += motorStateText();
  json += F("\",\"motor_wait_ms\":");
  json += String(motorWaitRemainingMs());
  json += F(",\"mode_transition\":");
  json += modeTransition ? "true" : "false";
  json += F(",\"mode_wait_total_ms\":");
  json += String(DEFAULT_MODE_DELAY_MS);
  json += F(",\"motor_wait_total_ms\":");
  json += String(DEFAULT_MOTOR_STOP_DELAY_MS);
  json += F(",\"mode_wait_ms\":");
  json += String(modeWaitRemainingMs());
  json += F(",\"auto_ready\":");
  json += (currentMode=="auto" && autoReady && modeIsReady() && !motorAutoFault()) ? "true" : "false";
  json += F(",\"can_forward\":");
  json += (manualMode && modeIsReady() && motorCanForward()) ? "true" : "false";
  json += F(",\"can_reverse\":");
  json += (manualMode && modeIsReady() && motorCanReverse()) ? "true" : "false";
  json += F(",\"can_stop\":");
  json += (manualMode && (motorState()==MOTOR_FORWARD || motorState()==MOTOR_REVERSE)) ? "true" : "false";
  json += F(",\"can_auto\":");
  json += (encoderValid() && motorManualPauseRemainingMs()==0 && motorState() == MOTOR_STOPPED && !modeTransition && !motorAutoFault()) ? "true" : "false";

  json += F("}");

  server.sendHeader("Cache-Control", "no-store");

  server.send(
    200,
    "application/json; charset=utf-8",
    json
  );
}

void handleMode() {
  if (!server.hasArg("value")) {
    server.send(400, "text/plain", "Missing mode");
    return;
  }

  String mode = server.arg("value");
  if (mode == currentMode) {server.send(200,"application/json","{\\\"ok\\\":true}");return;}
  if (mode != "off" && !encoderValid()) {
    server.send(409,"text/plain; charset=utf-8","Энкодер не подключён: доступен только режим Выключен");return;
  }

  if (mode == "auto" && motorManualPauseRemainingMs()>0) {
    server.send(409,"text/plain; charset=utf-8","Во время паузы после лимита автоматический режим запрещён");return;
  }
  if (mode != "off" && motorAutoFault()) {
    server.send(409,"text/plain; charset=utf-8","Сначала сбросьте аварию в выключенном режиме");return;
  }
  if (mode == "off") {
    motorStop();currentMode="off";manualMode=false;autoReady=false;modeTransition=false;
    motorSetAutoMode(false);
    webAddEvent("Режим выключен");
  } else if (mode == "manual") {
    // Запрет переключения во время защитной паузы действует и на API.
    if (!manualMode && (modeTransition || motorState() != MOTOR_STOPPED)) {
      server.send(409, "text/plain; charset=utf-8", "Дождитесь завершения защитной паузы и остановки привода");
      return;
    }
    if (currentMode != "manual") {
      motorStop();
      manualMode = true;
      currentMode = "manual";
      motorSetAutoMode(false);
      autoReady = false;
      modeTransition = true;
      modeChangedAt = millis();
      webAddEvent("Переключение в ручной режим");
      Serial.println("[MODE] Manual");
    }
  } else if (mode == "auto") {
    if (currentMode != "auto" && (motorState() != MOTOR_STOPPED || modeTransition)) {
      server.send(409, "text/plain; charset=utf-8", "Сначала остановите привод и дождитесь завершения паузы");
      return;
    }
    if (currentMode != "auto") {
      motorStop();
      manualMode = false;
      currentMode = "auto";
      motorSetAutoMode(true);
      autoReady = false;
      modeTransition = true;
      modeChangedAt = millis();
      webAddEvent("Переключение в автоматический режим");
      Serial.println("[MODE] Auto");
    }
  } else {
    server.send(400, "text/plain", "Invalid mode");
    return;
  }

  server.send(
    200,
    "application/json",
    "{\"ok\":true}"
  );
}

void handleCommand() {
  if (currentMode != "manual") {
    server.send(
      409,
      "text/plain",
      "Manual mode required"
    );
    return;
  }

  if (!server.hasArg("action")) {
    server.send(
      400,
      "text/plain",
      "Missing action"
    );
    return;
  }

  String action = server.arg("action");

  if (action != "forward" &&
      action != "backward" &&
      action != "stop") {

    server.send(
      400,
      "text/plain",
      "Invalid action"
    );
    return;
  }

  if (action == "stop" && motorState()!=MOTOR_FORWARD && motorState()!=MOTOR_REVERSE) {
    server.send(409,"text/plain; charset=utf-8","Двигатель уже остановлен");return;
  }
  if (action != "stop" && (!modeIsReady() || !encoderValid())) {
    server.send(409, "text/plain", "Mode transition or motor cooldown active");
    return;
  }
  if (!motorCommand(action)) {
    server.send(409, "text/plain", "Motor command rejected (fault or invalid state)");
    return;
  }

  webAddEvent(String("Ручная команда: ") + action);
  server.send(
    200,
    "application/json",
    "{\"ok\":true,\"simulation\":false}"
  );
}

void handleEvents() {
  String json;
  json.reserve(100 + eventCount * 150);

  json += F("{\"events\":[");

  for (int i = (int)eventCount - 1; i >= 0; i--) {
    if (i != (int)eventCount - 1) {
      json += ',';
    }

    String eventTime = formatEventTime(events[i].timestamp);

    if (eventTime.length() == 0) {
      eventTime = "После запуска: +";
      eventTime += String(events[i].uptimeSeconds);
      eventTime += " с";
    }

    json += F("{\"time\":\"");
    json += jsonEscape(eventTime);

    json += F("\",\"message\":\"");
    json += jsonEscape(events[i].message);

    json += F("\"}");
  }

  json += F("]}");

  server.sendHeader("Cache-Control", "no-store");

  server.send(
    200,
    "application/json; charset=utf-8",
    json
  );
}


static bool readSetting(const char* name,uint32_t& value) {
  if(!server.hasArg(name))return false;
  const String s=server.arg(name);
  if(s.length()==0 || s.length()>10)return false;
  for(unsigned int i=0;i<s.length();++i)if(s[i]<'0'||s[i]>'9')return false;
  const unsigned long v=strtoul(s.c_str(),nullptr,10);
  if(String(v)!=s)return false;
  value=v;return true;
}
void handleSettingsGet(){
  String json="{", sep="";
  json+=sep+String(char(34))+"manualLimitSec"+char(34)+":"+String(runtimeSettings.manualLimitSec);sep=",";
  json+=sep+String(char(34))+"autoLimitSec"+char(34)+":"+String(runtimeSettings.autoLimitSec);sep=",";
  json+=sep+String(char(34))+"manualPauseSec"+char(34)+":"+String(runtimeSettings.manualPauseSec);sep=",";
  json+=sep+String(char(34))+"calibrationGuardSec"+char(34)+":"+String(runtimeSettings.calibrationGuardSec);sep=",";
  json+=sep+String(char(34))+"noMotionWarnSec"+char(34)+":"+String(runtimeSettings.noMotionWarnSec);sep=",";
  json+=sep+String(char(34))+"autoStepMin"+char(34)+":"+String(runtimeSettings.autoStepMin);sep=",";
  json+=sep+String(char(34))+"wifiReconnectMs"+char(34)+":"+String(runtimeSettings.wifiReconnectMs);sep=",";
  json+=sep+String(char(34))+"wifiDiagnosticMs"+char(34)+":"+String(runtimeSettings.wifiDiagnosticMs);sep=",";
  json+=sep+String(char(34))+"ntpStaleSec"+char(34)+":"+String(runtimeSettings.ntpStaleSec);sep=",";
  json+=sep+String(char(34))+"encoderPollMs"+char(34)+":"+String(runtimeSettings.encoderPollMs);sep=",";
  json+=sep+String(char(34))+"encoderProbeMs"+char(34)+":"+String(runtimeSettings.encoderProbeMs);sep=",";
  json+=sep+String(char(34))+"encoderStableSamples"+char(34)+":"+String(runtimeSettings.encoderStableSamples);sep=",";
  json+=","+String(char(34))+"_saved"+char(34)+":";
  json+=settingsPersistedMatches()?"true":"false";
  json+="}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",json);
}
void handleSettingsPost(){
  // Защитные настройки меняем только при остановленном двигателе.
  if(currentMode=="auto" || motorState()!=MOTOR_STOPPED || modeTransition){
    server.send(409,"text/plain; charset=utf-8","Остановите двигатель и дождитесь окончания паузы");
    return;
  }
  RuntimeSettings next=runtimeSettings;
  if(!readSetting("manualLimitSec",next.manualLimitSec) ||
     !readSetting("autoLimitSec",next.autoLimitSec) ||
     !readSetting("manualPauseSec",next.manualPauseSec) ||
     !readSetting("calibrationGuardSec",next.calibrationGuardSec) ||
     !readSetting("noMotionWarnSec",next.noMotionWarnSec) ||
     !readSetting("autoStepMin",next.autoStepMin) ||
     !readSetting("wifiReconnectMs",next.wifiReconnectMs) ||
     !readSetting("wifiDiagnosticMs",next.wifiDiagnosticMs) ||
     !readSetting("ntpStaleSec",next.ntpStaleSec) ||
     !readSetting("encoderPollMs",next.encoderPollMs) ||
     !readSetting("encoderProbeMs",next.encoderProbeMs) ||
     !readSetting("encoderStableSamples",next.encoderStableSamples) ||
     !settingsValidate(next)){
    server.send(400,"text/plain; charset=utf-8","Некорректные параметры");
    return;
  }
  const String operation=server.arg("operation");
  if(operation!="apply" && operation!="save"){
    server.send(400,"text/plain","Invalid operation");return;
  }
  RuntimeSettings previous=runtimeSettings;
  runtimeSettings=next;
  if(operation=="save" && !settingsSave()){
    runtimeSettings=previous;
    server.send(500,"text/plain; charset=utf-8","Не удалось сохранить настройки");
    return;
  }
  webAddEvent(operation=="save"?"Настройки сохранены":"Настройки применены временно");
  server.send(200,"application/json",String("{")+char(34)+"ok"+char(34)+":true}");
}
static bool calibrationAllowed(){
  return currentMode=="manual" && motorState()==MOTOR_STOPPED &&
    !modeTransition && !motorAutoFault() && encoderValid() && timeIsFresh();
}
static bool calibrationTimeAllowed(uint8_t& minute){
  time_t now=time(nullptr);
  struct tm local={};
  if(!localtime_r(&now,&local))return false;
  const uint32_t guard=runtimeSettings.calibrationGuardSec;
  if((uint32_t)local.tm_sec<guard || (uint32_t)local.tm_sec>=60UL-guard)return false;
  minute=(uint8_t)local.tm_min;
  return true;
}
void handleCalibration(){
  if(!calibrationAllowed()){
    server.send(409,"text/plain; charset=utf-8","Нужен ручной режим, остановленный двигатель, исправный энкодер и актуальный NTP");return;
  }
  uint8_t minute=0;
  if(!calibrationTimeAllowed(minute)){
    const unsigned guard=runtimeSettings.calibrationGuardSec;
    server.send(409,"text/plain; charset=utf-8",String("Калибровка разрешена с ")+guard+" по "+(59U-guard)+" секунду");return;
  }
  if(!calibrationSave(minute,encoderRawPosition())){
    server.send(500,"text/plain; charset=utf-8","Не удалось сохранить калибровку");return;
  }
  webAddEvent("Калибровка энкодера сохранена");
  server.send(200,"application/json","{\"ok\":true}");
}
void handleFaultReset(){
  if(currentMode!="off" || !encoderValid() || !motorResetAutoFault()){
    server.send(409,"text/plain; charset=utf-8","Перейдите в Выключен, проверьте энкодер и наличие аварии");
    return;
  }
  webAddEvent("Авария сброшена оператором");
  server.send(200,"application/json",String("{")+char(34)+"ok"+char(34)+":true}");
}

void webSetup() {
  manualMode = false;
  currentMode = "off";
  motorSetAutoMode(false);
  modeTransition = false;
  autoReady = false;
  eventCount = 0;
  previousAutoFault=false;
  previousManualPause=0;

  webAddEvent("Контроллер запущен");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/mode", HTTP_POST, handleMode);
  server.on("/api/command", HTTP_POST, handleCommand);
  server.on("/api/events", HTTP_GET, handleEvents);
  server.on("/api/settings", HTTP_GET, handleSettingsGet);
  server.on("/api/settings", HTTP_POST, handleSettingsPost);
  server.on("/api/reset-fault", HTTP_POST, handleFaultReset);
  server.on("/api/calibration", HTTP_POST, handleCalibration);

  server.onNotFound([]() {
    server.send(
      404,
      "text/plain; charset=utf-8",
      "Not found"
    );
  });

  server.begin();

  Serial.println("[WEB] HTTP server v1.11 started");
}

void webLoop() {
  const bool encoderNowValid=encoderValid();
  if (!encoderNowValid && currentMode!="off") {
    // При потере датчика во время движения motorLoop уже фиксирует аварию.
    // Режим OFF сохраняет аварийную блокировку до ручного сброса.
    motorStop();currentMode="off";manualMode=false;autoReady=false;
    modeTransition=false;motorSetAutoMode(false);
    webAddEvent("Энкодер недоступен: переход в Выключен");
  }
  const bool fault=motorAutoFault();
  if(fault && !previousAutoFault){
    String reason=motorFaultReason();
    webAddEvent(reason=="no_motion" ? "Двигатель остановлен: энкодер не фиксирует движение" :
      reason=="encoder_lost" ? "Потеря связи с энкодером во время движения" :
      "Критическая ошибка: привод заблокирован");
  }
  if(fault && currentMode!="off"){
    motorStop();currentMode="off";manualMode=false;autoReady=false;
    modeTransition=false;motorSetAutoMode(false);
    webAddEvent("Аварийный переход в Выключен");
  }
  previousAutoFault=fault;
  const unsigned long manualPause=motorManualPauseRemainingMs();
  if(manualPause && !previousManualPause)webAddEvent("Достигнут лимит ручной работы: защитная пауза");
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
  server.handleClient();
}