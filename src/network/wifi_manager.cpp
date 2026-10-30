
#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "../../config.h"
#include "wifi_manager.h"

static unsigned long lastReconnectAttempt = 0;
static unsigned long lastDiagnostic = 0;
static bool wasConnected = false;

const char* wifiStatusText(wl_status_t status) {
  switch (status) {
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_NO_SSID_AVAIL:
      return "SSID NOT FOUND";
    case WL_CONNECT_FAILED:
      return "CONNECT FAILED (password/auth?)";
    case WL_CONNECTION_LOST:
      return "CONNECTION LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    case WL_IDLE_STATUS:
      return "IDLE";
    default:
      return "UNKNOWN";
  }
}

void printWifiDiagnostics() {
  wl_status_t status = WiFi.status();

  Serial.println();
  Serial.println("===== WIFI DIAGNOSTICS =====");

  Serial.print("SSID: ");
  Serial.println(WIFI_SSID);

  Serial.print("Status: ");
  Serial.print(wifiStatusText(status));
  Serial.print(" (");
  Serial.print((int)status);
  Serial.println(")");

  Serial.print("WiFi mode: ");
  Serial.println((int)WiFi.getMode());

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  if (status == WL_CONNECTED) {
    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
  }

  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");

  Serial.println("============================");
}

void wifiSetup() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(DEVICE_HOSTNAME);
  WiFi.setAutoReconnect(true);

  Serial.println("[WiFi] Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  lastReconnectAttempt = millis();
  lastDiagnostic = millis();

  printWifiDiagnostics();
}

void wifiLoop() {
  bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected != wasConnected) {
    wasConnected = connected;

    if (connected) {
      Serial.println("[WiFi] Connected!");
      Serial.print("[WiFi] IP: ");
      Serial.println(WiFi.localIP());
      printWifiDiagnostics();
    } else {
      Serial.println("[WiFi] Disconnected!");
      printWifiDiagnostics();
    }
  }

  if (!connected &&
      millis() - lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL) {

    lastReconnectAttempt = millis();

    Serial.println("[WiFi] Reconnecting...");
    WiFi.reconnect();
  }

  // Подробная диагностика каждые 15 секунд,
  // только пока соединение отсутствует.
  if (!connected &&
      millis() - lastDiagnostic >= 15000UL) {

    lastDiagnostic = millis();
    printWifiDiagnostics();
  }
}
