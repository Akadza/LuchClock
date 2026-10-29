#pragma once
#include "secrets.h"

// Имя устройства в сети
#define DEVICE_HOSTNAME "luch-clock"

// Веб-сервер
#define WEB_PORT 80

// Интервалы
#define WIFI_RECONNECT_INTERVAL 10000UL

// Будущие функции
#define MAX_LOG_ENTRIES 20
#define SIMULATION_MODE true

// Минск: UTC+3
#define TIMEZONE_TZ "MSK-3"

// Серверы точного времени
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.google.com"

// Если подтверждённой синхронизации не было 3 часа,
// показываем предупреждение.
#define NTP_STALE_INTERVAL 10800000UL