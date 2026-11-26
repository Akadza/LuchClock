#pragma once
#include "secrets.h"

#define FIRMWARE_VERSION "1.19" // Версия прошивки — менять только здесь.
#define API_VERSION 1 // Версия внешнего контракта API.

// Имя конкретного контроллера. Менять здесь перед прошивкой каждой ESP.
#define DEVICE_NAME "LuchClock1"
// Сетевое имя: для четырёх ESP должно быть уникальным.
#define DEVICE_HOSTNAME "luchclock1"

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