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

void handleStatus() {
  String json;
  json.reserve(560);

  json += F("{\"device\":\"LuchClock\",");
  json += F("\"version\":\"0.3\",");

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
  json += String(encoderValid() ? encoderPosition : -1);
  json += F(",\"encoder_minutes\":");
  json += String(encoderMinutes());

  json += F(",\"motor_run_ms\":");json += String(motorRunElapsedMs());
  json += F(",\"motor_limit_ms\":");json += String((manualMode?runtimeSettings.manualLimitSec:runtimeSettings.autoLimitSec)*1000UL);
  json += F(",\"manual_pause_ms\":");json += String(motorManualPauseRemainingMs());
  json += F(",\"auto_fault\":");json += motorAutoFault()?"true":"false";
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
  json += F(",\"can_stop\":true");
  json += F(",\"can_auto\":");
  json += (motorState() == MOTOR_STOPPED && !modeTransition && !motorAutoFault()) ? "true" : "false";

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
  String json="{";
  json+="\\"manualLimitSec\\":"+String(runtimeSettings.manualLimitSec);
  json+=",\\"autoLimitSec\\":"+String(runtimeSettings.autoLimitSec);
  json+=",\\"manualPauseSec\\":"+String(runtimeSettings.manualPauseSec);
  json+=",\\"calibrationGuardSec\\":"+String(runtimeSettings.calibrationGuardSec);
  json+=",\\"noMotionWarnSec\\":"+String(runtimeSettings.noMotionWarnSec);
  json+=",\\"autoStepMin\\":"+String(runtimeSettings.autoStepMin);
  json+="}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",json);
}
void handleSettingsPost(){
  // Защитные настройки меняем только при остановленном двигателе.
  if(motorState()!=MOTOR_STOPPED || modeTransition){
    server.send(409,"text/plain; charset=utf-8","Остановите двигатель и дождитесь окончания паузы");
    return;
  }
  RuntimeSettings next=runtimeSettings;
  if(!readSetting("manualLimitSec",next.manualLimitSec) ||
     !readSetting("autoLimitSec",next.autoLimitSec) ||
     !readSetting("manualPauseSec",next.manualPauseSec) ||
     !readSetting("calibrationGuardSec",next.calibrationGuardSec) ||
     !readSetting("noMotionWarnSec",next.noMotionWarnSec) ||
     !readSetting("autoStepMin",next.autoStepMin) || !settingsValidate(next)){
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
  server.send(200,"application/json","{\\"ok\\":true}");
}
void handleFaultReset(){
  if(currentMode!="off" || !encoderValid() || !motorResetAutoFault()){
    server.send(409,"text/plain; charset=utf-8","Перейдите в Выключен, проверьте энкодер и наличие аварии");
    return;
  }
  webAddEvent("Авария сброшена оператором");
  server.send(200,"application/json","{\\"ok\\":true}");
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

  server.onNotFound([]() {
    server.send(
      404,
      "text/plain; charset=utf-8",
      "Not found"
    );
  });

  server.begin();

  Serial.println("[WEB] HTTP server v1.3 started");
}

void webLoop() {
  const bool fault=motorAutoFault();
  if(fault && !previousAutoFault)webAddEvent("КРИТИЧЕСКАЯ ОШИБКА: привод заблокирован (лимит или энкодер)");
  previousAutoFault=fault;
  const unsigned long manualPause=motorManualPauseRemainingMs();
  if(manualPause && !previousManualPause)webAddEvent("ПРЕДУПРЕЖДЕНИЕ: лимит ручной работы, пауза");
  previousManualPause=manualPause;
  if (modeTransition && motorState() != MOTOR_WAITING &&
      (modeWaitRemainingMs() == 0 || motorState() == MOTOR_FAULT)) {
    modeTransition = false;
    if (currentMode=="auto") {
      autoReady = (motorState() == MOTOR_STOPPED);
      webAddEvent(autoReady
        ? "Автоматический режим готов (движение пока не реализовано)"
        : "Автоматический режим: привод не готов");
    } else {
      webAddEvent("Ручное управление разблокировано");
    }
  }
  server.handleClient();
}