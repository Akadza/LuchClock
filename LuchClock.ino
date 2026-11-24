#include <Arduino.h>

#include "config.h"
#include "src/system_settings.h"
#include "src/network/wifi_manager.h"
#include "src/network/time_manager.h"
#include "src/web/web_server.h"
#include "src/application/controller.h"
#include "src/application/event_logger.h"
#include "src/motor/motor_controller.h"
#include "src/encoder/encoder_reader.h"
#include "src/encoder/encoder_calibration.h"

void setup() {
  motorSetup(); // Safe output before serial and delays.
  Serial.begin(115200);
  delay(100);

  Serial.println();
  Serial.println("==================");
  Serial.println("LuchClock v" FIRMWARE_VERSION);
  Serial.println("==================");

  settingsSetup(); // Загрузка параметров из EEPROM.
  calibrationSetup(); // Загрузка сохранённого смещения энкодера.
  encoderSetup();
  wifiSetup();
  timeSetup();
  controllerSetup();
  webSetup();

  Serial.print("[SYSTEM] Device: ");Serial.println(DEVICE_NAME);
  Serial.println("[SYSTEM] Startup complete");
}

void loop() {
  wifiLoop();

  TimeEvent event = timeLoop();

  switch (event) {
    case TIME_EVENT_FIRST_SYNC:
      eventAdd("Получено точное время NTP",EventCode::NTP_SYNC);
      break;

    case TIME_EVENT_STALE:
      eventAdd("NTP давно не синхронизировался",EventCode::NTP_STALE,EventLevel::Warning);
      break;

    case TIME_EVENT_RESTORED:
      eventAdd("Синхронизация NTP восстановлена",EventCode::NTP_RESTORED);
      break;

    default:
      break;
  }

  encoderLoop();
  motorLoop();
  controllerLoop();
  webLoop();

  yield();
}