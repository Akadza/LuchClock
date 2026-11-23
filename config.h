#pragma once
#include "secrets.h"

// Имя устройства в сети
#define DEVICE_HOSTNAME "luch-clock"

// Веб-сервер
#define WEB_PORT 80

// Будущие функции
#define MAX_LOG_ENTRIES 20
#define SIMULATION_MODE true

// Минск: UTC+3
#define TIMEZONE_TZ "MSK-3"

// Серверы точного времени
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.google.com"

// Временные параметры находятся в src/system_settings.h