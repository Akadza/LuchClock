#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <coredecls.h>
#include <time.h>

#include "../../config.h"
#include "time_manager.h"

static volatile bool ntpCallbackPending = false;

static bool everSynchronized = false;
static bool syncStale = false;

static unsigned long lastSyncMillis = 0;
static time_t lastSyncEpoch = 0;

// Вызывается ядром ESP8266 после установки времени.
void onTimeSet() {
  ntpCallbackPending = true;
}

void timeSetup() {
  ntpCallbackPending = false;
  everSynchronized = false;
  syncStale = false;

  settimeofday_cb(onTimeSet);

  configTime(
    TIMEZONE_TZ,
    NTP_SERVER_1,
    NTP_SERVER_2
  );

  Serial.println("[NTP] Synchronization configured");
  Serial.println("[NTP] Timezone: Europe/Minsk (UTC+3)");
}

bool timeIsValid() {
  // Отсекаем исходное время после запуска ESP.
  return time(nullptr) >= 1700000000;
}

bool timeIsFresh() {
  return everSynchronized && !syncStale && timeIsValid();
}

unsigned long timeSyncAgeSeconds() {
  if (!everSynchronized) {
    return 0;
  }

  return (millis() - lastSyncMillis) / 1000UL;
}

String formatLocalTime(time_t timestamp, const char* format) {
  struct tm localTime;

  if (localtime_r(&timestamp, &localTime) == nullptr) {
    return "";
  }

  char buffer[32];

  if (strftime(buffer, sizeof(buffer), format, &localTime) == 0) {
    return "";
  }

  return String(buffer);
}

String timeLocalString() {
  if (!timeIsValid()) {
    return "";
  }

  return formatLocalTime(time(nullptr), "%H:%M:%S");
}

String timeLastSyncString() {
  if (!everSynchronized || lastSyncEpoch == 0) {
    return "";
  }

  return formatLocalTime(
    lastSyncEpoch,
    "%d.%m.%Y %H:%M:%S"
  );
}

TimeEvent timeLoop() {
  if (ntpCallbackPending) {
    ntpCallbackPending = false;

    if (timeIsValid()) {
      bool firstSync = !everSynchronized;
      bool recovered = everSynchronized && syncStale;

      everSynchronized = true;
      syncStale = false;

      lastSyncMillis = millis();
      lastSyncEpoch = time(nullptr);

      Serial.print("[NTP] Time synchronized: ");
      Serial.println(timeLastSyncString());

      if (firstSync) {
        return TIME_EVENT_FIRST_SYNC;
      }

      if (recovered) {
        return TIME_EVENT_RESTORED;
      }
    }
  }

  if (everSynchronized && !syncStale &&
      millis() - lastSyncMillis >= NTP_STALE_INTERVAL) {

    syncStale = true;

    Serial.println("[NTP] WARNING: synchronization is stale");

    return TIME_EVENT_STALE;
  }

  return TIME_EVENT_NONE;
}