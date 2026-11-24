#include <Arduino.h>

#include "config.h"
#include "src/system_settings.h"
#include "src/network/wifi_manager.h"
#include "src/network/time_manager.h"
#include "src/web/web_server.h"
#include "src/motor/motor_controller.h"
#include "src/encoder/encoder_reader.h"
#include "src/encoder/encoder_calibration.h"

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("==================");
  Serial.println("LuchClock v1.10");
  Serial.println("==================");

  motorSetup(); // Сначала гарантируем LOW на D5.
  settingsSetup(); // Загрузка параметров из EEPROM.
  calibrationSetup(); // Загрузка сохранённого смещения энкодера.
  encoderSetup();
  wifiSetup();
  timeSetup();
  webSetup();

  Serial.print("[SYSTEM] Device: ");Serial.println(DEVICE_NAME);
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

  encoderLoop();
  motorLoop();
  webLoop();

  yield();
}