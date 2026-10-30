#include <Arduino.h>

#include "config.h"
#include "src/network/wifi_manager.h"
#include "src/network/time_manager.h"
#include "src/web/web_server.h"
#include "src/motor/motor_controller.h"
#include "src/encoder/encoder_reader.h"

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("==================");
  Serial.println("LuchClock v0.3");
  Serial.println("==================");

  motorSetup(); // Инициализируем безопасный LOW до Wi-Fi.
  encoderSetup();
  wifiSetup();
  timeSetup();
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
  encoderLoop();
  webLoop();

  yield();
}