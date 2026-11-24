#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>

#include "../../config.h"
#include "../network/time_manager.h"
#include "web_server.h"
#include "../application/controller.h"
#include "../application/event_logger.h"
#include "web_ui.h"
#include "../motor/motor_controller.h"
#include "../system_settings.h"
#include "../encoder/encoder_reader.h"
#include "../encoder/encoder_calibration.h"

static ESP8266WebServer server(WEB_PORT);

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

void handleRoot() {
  server.send_P(
    200,
    "text/html; charset=utf-8",
    WEB_UI
  );
}


void handleStatus() {
  const ModeState state=controller().state();
  String json;
  json.reserve(560);

  json += F("{\"device\":\"");json+=jsonEscape(DEVICE_NAME);json+=F("\",");
  json += F("\"version\":\"1.13\",");

  json += F("\"wifi\":");
  json += (WiFi.status() == WL_CONNECTED) ? "true" : "false";

  json += F(",\"ip\":\"");
  json += WiFi.localIP().toString();

  json += F("\",\"mode\":\"");
  json += state.mode;

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
  json += F(",\"calibration_allowed\":");json += state.calibrationAllowed?"true":"false";
  json += F(",\"encoder_minutes\":");
  json += String(encoderMinutes());
  json += F(",\"calibration_saved\":");json += calibrationSaved()?"true":"false";
  json += F(",\"calibration_offset\":");json += String(calibrationOffset());
  json += F(",\"calibration_signed_offset\":");json += String((int)calibrationSignedOffset());

  json += F(",\"motor_run_ms\":");json += String(motorRunElapsedMs());
  json += F(",\"motor_limit_ms\":");json += String(state.motorLimitMs);
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
  json += state.transition ? "true" : "false";
  json += F(",\"mode_wait_total_ms\":");
  json += String(DEFAULT_MODE_DELAY_MS);
  json += F(",\"motor_wait_total_ms\":");
  json += String(DEFAULT_MOTOR_STOP_DELAY_MS);
  json += F(",\"mode_wait_ms\":");
  json += String(state.waitMs);
  json += F(",\"auto_ready\":");
  json += state.autoReady ? "true" : "false";
  json += F(",\"can_forward\":");
  json += state.canForward ? "true" : "false";
  json += F(",\"can_reverse\":");
  json += state.canReverse ? "true" : "false";
  json += F(",\"can_stop\":");
  json += state.canStop ? "true" : "false";
  json += F(",\"can_auto\":");
  json += state.canAuto ? "true" : "false";

  json += F("}");

  server.sendHeader("Cache-Control", "no-store");

  server.send(
    200,
    "application/json; charset=utf-8",
    json
  );
}

static void sendResult(CommandResult r){
 if(r.status==200)server.send(200,"application/json","{\"ok\":true}");
 else server.send(r.status,"text/plain; charset=utf-8",r.message);
}
void handleMode(){if(!server.hasArg("value")){sendResult({400,"Missing mode"});return;}sendResult(controller().setMode(server.arg("value")));}
void handleCommand(){if(!server.hasArg("action")){sendResult({400,"Missing action"});return;}sendResult(controller().command(server.arg("action")));}
void handleEvents() {
  String json;
  json.reserve(100 + eventSize() * 150);

  json += F("{\"events\":[");

  for (int i = (int)eventSize() - 1; i >= 0; i--) {
    if (i != (int)eventSize() - 1) {
      json += ',';
    }

    String eventTime = formatEventTime(eventAt(i).timestamp);

    if (eventTime.length() == 0) {
      eventTime = "После запуска: +";
      eventTime += String(eventAt(i).uptimeSeconds);
      eventTime += " с";
    }

    json += F("{\"time\":\"");
    json += jsonEscape(eventTime);

    json += F("\",\"message\":\"");
    json += jsonEscape(eventAt(i).message);

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
     !readSetting("encoderStableSamples",next.encoderStableSamples)){
    server.send(400,"text/plain; charset=utf-8","Некорректные параметры");
    return;
  }
  const String operation=server.arg("operation");
  if(operation!="apply" && operation!="save"){
    server.send(400,"text/plain","Invalid operation");return;
  }
  sendResult(controller().changeSettings(next,operation=="save"));
}
void handleCalibration(){sendResult(controller().calibrate());}
void handleFaultReset(){sendResult(controller().resetFault());}
void webSetup() {
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

  Serial.println("[WEB] HTTP server v1.13 started");
}

void webLoop(){server.handleClient();}
