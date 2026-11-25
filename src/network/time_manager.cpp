#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <coredecls.h>
#include <time.h>

#include "../../config.h"
#include "time_manager.h"
#include "../system_settings.h"

static volatile bool ntpCallbackPending = false;

static bool everSynchronized = false;
static bool syncStale = false;

static uint32_t lastTick = 0;
static uint64_t syncAgeMs = 0;
static time_t lastSyncEpoch = 0;

// Вызывается ядром ESP8266 после установки времени.
void onTimeSet() {
  ntpCallbackPending = true;
}

void timeSetup() {
  ntpCallbackPending = false;
  everSynchronized = false;
  syncStale = false;

  lastTick=millis();
  syncAgeMs=0;
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
  return everSynchronized && syncAgeMs + uint32_t(millis()-lastTick) < uint64_t(settingsCurrent().ntpStaleSec)*1000 && timeIsValid();
}

unsigned long timeSyncAgeSeconds() {
  if (!everSynchronized) {
    return 0;
  }

  return (syncAgeMs + uint32_t(millis()-lastTick)) / 1000ULL;
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
  const uint32_t now=millis();
  if(everSynchronized)syncAgeMs+=uint32_t(now-lastTick);
  lastTick=now;
  if (ntpCallbackPending) {
    ntpCallbackPending = false;

    if (timeIsValid()) {
      bool firstSync = !everSynchronized;
      bool recovered = everSynchronized && syncStale;

      everSynchronized = true;
      syncStale = false;

      syncAgeMs = 0;
      lastTick = millis();
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
      !timeIsFresh()) {

    syncStale = true;

    Serial.println("[NTP] WARNING: synchronization is stale");

    return TIME_EVENT_STALE;
  }

  if(everSynchronized && syncStale && timeIsFresh()){
    syncStale=false;
    return TIME_EVENT_THRESHOLD_CHANGED;
  }
  return TIME_EVENT_NONE;
}