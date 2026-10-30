#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <time.h>

#include "../../config.h"
#include "../network/time_manager.h"
#include "web_server.h"
#include "web_ui.h"
#include "../motor/motor_controller.h"
#include "../motor/motor_settings.h"

static ESP8266WebServer server(WEB_PORT);

static bool manualMode = false;
static unsigned long modeChangedAt = 0;
static bool modeTransition = false;
static bool autoReady = false;

static unsigned long modeWaitRemainingMs() {
  if (!modeTransition) return 0;
  unsigned long elapsed = millis() - modeChangedAt;
  return elapsed >= MOTOR_MODE_CHANGE_DELAY_MS ? 0 : MOTOR_MODE_CHANGE_DELAY_MS - elapsed;
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
  json.reserve(420);

  json += F("{\"device\":\"LuchClock\",");
  json += F("\"version\":\"0.3\",");

  json += F("\"wifi\":");
  json += (WiFi.status() == WL_CONNECTED) ? "true" : "false";

  json += F(",\"ip\":\"");
  json += WiFi.localIP().toString();

  json += F("\",\"mode\":\"");
  json += manualMode ? "manual" : "auto";

  json += F("\",\"simulation\":true");

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
  json += String(MOTOR_MODE_CHANGE_DELAY_MS);
  json += F(",\"motor_wait_total_ms\":");
  json += String(MOTOR_STOP_DELAY_MS);
  json += F(",\"mode_wait_ms\":");
  json += String(modeWaitRemainingMs());
  json += F(",\"auto_ready\":");
  json += (!manualMode && autoReady && modeIsReady()) ? "true" : "false";
  json += F(",\"can_forward\":");
  json += (manualMode && modeIsReady() && motorCanForward()) ? "true" : "false";
  json += F(",\"can_reverse\":");
  json += (manualMode && modeIsReady() && motorCanReverse()) ? "true" : "false";
  json += F(",\"can_stop\":true");
  json += F(",\"can_auto\":");
  json += (motorState() == MOTOR_STOPPED && !modeTransition && !manualMode) ||
          (manualMode && motorState() == MOTOR_STOPPED && !modeTransition) ? "true" : "false";

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

  if (mode == "manual") {
    if (!manualMode) {
      motorStop();
      manualMode = true;
      autoReady = false;
      modeTransition = true;
      modeChangedAt = millis();
      webAddEvent("Переключение в ручной режим");
      Serial.println("[MODE] Manual");
    }
  } else if (mode == "auto") {
    if (manualMode && (motorState() != MOTOR_STOPPED || modeTransition)) {
      server.send(409, "text/plain; charset=utf-8", "Сначала остановите привод и дождитесь завершения паузы");
      return;
    }
    if (manualMode) {
      motorStop();
      manualMode = false;
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
  if (!manualMode) {
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

  if (action != "stop" && !modeIsReady()) {
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
    "{\"ok\":true,\"simulation\":true}"
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
      eventTime = "Время неизвестно (+";
      eventTime += String(events[i].uptimeSeconds);
      eventTime += " сек после запуска)";
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

void webSetup() {
  manualMode = false;
  modeTransition = false;
  autoReady = false;
  eventCount = 0;

  webAddEvent("Контроллер запущен");

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/mode", HTTP_POST, handleMode);
  server.on("/api/command", HTTP_POST, handleCommand);
  server.on("/api/events", HTTP_GET, handleEvents);

  server.onNotFound([]() {
    server.send(
      404,
      "text/plain; charset=utf-8",
      "Not found"
    );
  });

  server.begin();

  Serial.println("[WEB] HTTP server v0.3 started");
}

void webLoop() {
  if (modeTransition && motorState() != MOTOR_WAITING &&
      (modeWaitRemainingMs() == 0 || motorState() == MOTOR_FAULT)) {
    modeTransition = false;
    if (!manualMode) {
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