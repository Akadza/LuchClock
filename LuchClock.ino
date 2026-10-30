#include <Arduino.h>

#include "config.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "web_server.h"
#include "motor_controller.h"

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("==================");
  Serial.println("LuchClock v0.3");
  Serial.println("==================");

  wifiSetup();
  timeSetup();
  motorSetup();
  webSetup();

  Serial.println("[SYSTEM] Startup complete");
}

void loop() {
  wifiLoop();

  TimeEvent event = timeLoop();

  switch (event) {
    case TIME_EVENT_FIRST_SYNC:
      webAddEvent("Получено точное время NTP");
      break;

    case TIME_EVENT_STALE:
      webAddEvent("Предупреждение: NTP давно не синхронизировался");
      break;

    case TIME_EVENT_RESTORED:
      webAddEvent("Синхронизация NTP восстановлена");
      break;

    default:
      break;
  }

  motorLoop();
  webLoop();

  yield();
}