#pragma once
// Совместимость со старым путём include.
// Единый источник параметров: src/system_settings.h
#include "../system_settings.h"
// В рабочем motor_controller.cpp используются непосредственно
// DEFAULT_MOTOR_STOP_DELAY_MS, DEFAULT_MODE_DELAY_MS и runtimeSettings.
// Старые MOTOR_MAX_RUN_MS (30 секунд) и MOTOR_TEST_OUTPUT_PIN удалены.
