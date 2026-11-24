#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>

#include "../../config.h"
#include "web_server.h"
#include "../application/controller.h"
#include "../application/event_logger.h"
#include "web_ui.h"
#include "../system_settings.h"

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
  const ControllerState snapshot=controller().snapshot();
  const ModeState& state=snapshot.mode;
  String json;
  json.reserve(1600);

  json += F("{\"device\":\"");json+=jsonEscape(snapshot.device);json+=F("\",");
  json += F("\"version\":\"" FIRMWARE_VERSION "\",\"api_version\":");

  json += String(API_VERSION);
  json += F(",\"wifi\":");
  json += (snapshot.wifi) ? "true" : "false";

  json += F(",\"ip\":\"");
  json += snapshot.ip;

  json += F("\",\"mode\":\"");
  json += state.mode;

  json += F("\",\"simulation\":false");

  json += F(",\"encoder_connected\":");
  json += snapshot.encoderConnected ? "true" : "false";
  json += F(",\"encoder_valid\":");
  json += snapshot.encoderValid ? "true" : "false";
  json += F(",\"encoder_address\":");
  json += String(snapshot.encoderAddress);
  json += F(",\"encoder_raw\":");
  json += String(snapshot.encoderRaw);
  json += F(",\"encoder_position\":");
  json += String(snapshot.encoderPosition);
  json += F(",\"encoder_raw_position\":");json += String(snapshot.encoderRawPosition);
  json += F(",\"calibration_allowed\":");json += state.calibrationAllowed?"true":"false";
  json += F(",\"encoder_minutes\":");
  json += String(snapshot.encoderMinutes);
  json += F(",\"calibration_saved\":");json += snapshot.calibrationSaved?"true":"false";
  json += F(",\"calibration_offset\":");json += String(snapshot.calibrationOffset);
  json += F(",\"calibration_signed_offset\":");json += String(snapshot.calibrationSignedOffset);

  json += F(",\"motor_run_ms\":");json += String(snapshot.motorRun);
  json += F(",\"motor_limit_ms\":");json += String(state.motorLimitMs);
  json += F(",\"manual_pause_ms\":");json += String(snapshot.manualPause);
  json += F(",\"auto_fault\":");json += snapshot.fault?"true":"false";
  json += F(",\"fault_reason\":\"");json += snapshot.faultReason;json += F("\"");
  json += F(",\"uptime_seconds\":");
  json += String(snapshot.uptime);

  json += F(",\"ntp_synced\":");
  json += snapshot.ntpSynced ? "true" : "false";

  json += F(",\"ntp_recent\":");
  json += snapshot.ntpRecent ? "true" : "false";

  json += F(",\"time_local\":\"");
  json += snapshot.localTime;

  json += F("\",\"last_ntp_sync\":\"");
  json += snapshot.lastSync;

  json += F("\",\"ntp_age_seconds\":");
  json += String(snapshot.ntpAge);

  json += F(",\"motor_state\":\"");
  json += snapshot.motorState;
  json += F("\",\"motor_wait_ms\":");
  json += String(snapshot.motorWait);
  json += F(",\"mode_transition\":");
  json += state.transition ? "true" : "false";
  json += F(",\"mode_wait_total_ms\":");
  json += String(snapshot.modeWaitTotal);
  json += F(",\"motor_wait_total_ms\":");
  json += String(snapshot.motorWaitTotal);
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

  json += F(",\"can_manual\":");json += state.canManual?"true":"false";
  json += F(",\"can_off\":");json += state.canOff?"true":"false";
  json += F(",\"can_reset_fault\":");json += state.canResetFault?"true":"false";
  json += F(",\"show_fault_reset\":");json += state.showFaultReset?"true":"false";
  json += F(",\"settings_allowed\":");json += state.settingsAllowed?"true":"false";
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
 else {String json=F("{\"ok\":false,\"code\":\"");json+=r.status==400?"INVALID_REQUEST":r.status==409?"OPERATION_BLOCKED":"INTERNAL_ERROR";json+=F("\",\"message\":\"");json+=jsonEscape(r.message);json+=F("\"}");server.send(r.status,"application/json; charset=utf-8",json);}
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

    json += F("\",\"code\":\"");json+=eventAt(i).code;
    json += F("\",\"level\":\"");json+=eventAt(i).level;
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
  const RuntimeSettings settings=controller().settings();
  String json="{", sep="";
  json+=sep+String(char(34))+"manualLimitSec"+char(34)+":"+String(settings.manualLimitSec);sep=",";
  json+=sep+String(char(34))+"autoLimitSec"+char(34)+":"+String(settings.autoLimitSec);sep=",";
  json+=sep+String(char(34))+"manualPauseSec"+char(34)+":"+String(settings.manualPauseSec);sep=",";
  json+=sep+String(char(34))+"calibrationGuardSec"+char(34)+":"+String(settings.calibrationGuardSec);sep=",";
  json+=sep+String(char(34))+"noMotionWarnSec"+char(34)+":"+String(settings.noMotionWarnSec);sep=",";
  json+=sep+String(char(34))+"autoStepMin"+char(34)+":"+String(settings.autoStepMin);sep=",";
  json+=sep+String(char(34))+"wifiReconnectMs"+char(34)+":"+String(settings.wifiReconnectMs);sep=",";
  json+=sep+String(char(34))+"wifiDiagnosticMs"+char(34)+":"+String(settings.wifiDiagnosticMs);sep=",";
  json+=sep+String(char(34))+"ntpStaleSec"+char(34)+":"+String(settings.ntpStaleSec);sep=",";
  json+=sep+String(char(34))+"encoderPollMs"+char(34)+":"+String(settings.encoderPollMs);sep=",";
  json+=sep+String(char(34))+"encoderProbeMs"+char(34)+":"+String(settings.encoderProbeMs);sep=",";
  json+=sep+String(char(34))+"encoderStableSamples"+char(34)+":"+String(settings.encoderStableSamples);sep=",";
  json+=","+String(char(34))+"_saved"+char(34)+":";
  json+=controller().settingsSaved()?"true":"false";
  json+="}";
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json",json);
}
void handleSettingsPost(){
  RuntimeSettings next=controller().settings();
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
    sendResult({400,"Некорректные параметры"});
    return;
  }
  const String operation=server.arg("operation");
  if(operation!="apply" && operation!="save"){
    sendResult({400,"Invalid operation"});return;
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

  Serial.println("[WEB] HTTP server v" FIRMWARE_VERSION " started");
}

void webLoop(){server.handleClient();}
