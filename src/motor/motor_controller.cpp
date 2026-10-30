#include <Arduino.h>
#include "motor_settings.h"
#include "motor_controller.h"

// Тестовый выход GPIO14: только Вперёд / Стоп.
static MotorState state = MOTOR_STOPPED;
static unsigned long waitStarted = 0;
static unsigned long runStarted = 0;

void motorSetup() {
  digitalWrite(MOTOR_TEST_OUTPUT_PIN, LOW);
  pinMode(MOTOR_TEST_OUTPUT_PIN, OUTPUT);
  state = MOTOR_STOPPED;
  waitStarted = 0;
  runStarted = 0;
}
MotorState motorState() { return state; }
const char* motorStateText() {
  switch (state) {
    case MOTOR_FORWARD: return "forward";
    case MOTOR_REVERSE: return "reverse";
    case MOTOR_WAITING: return "waiting";
    case MOTOR_FAULT: return "fault";
    default: return "stopped";
  }
}
unsigned long motorWaitRemainingMs() {
  if (state != MOTOR_WAITING) return 0;
  const unsigned long elapsed = millis() - waitStarted;
  return elapsed >= MOTOR_STOP_DELAY_MS ? 0 : MOTOR_STOP_DELAY_MS - elapsed;
}
bool motorCanForward() { return state == MOTOR_STOPPED || state == MOTOR_FORWARD; }
bool motorCanReverse() { return false; }
bool motorCanStop() { return true; }
void motorStop() {
  digitalWrite(MOTOR_TEST_OUTPUT_PIN, LOW);
  if (state == MOTOR_FORWARD || state == MOTOR_REVERSE) {
    // Отключить движение и выдержать паузу, даже при повторном STOP.
    state = MOTOR_WAITING;
    waitStarted = millis();
    Serial.println("[MOTOR TEST] STOP; cooldown started");
  } else if (state == MOTOR_STOPPED) {
    Serial.println("[MOTOR TEST] Already stopped");
  }
  // В WAITING повторный STOP не сбрасывает таймер.
  // В FAULT команда STOP не снимает аварийную блокировку.
}
void motorLoop() {
  if (state == MOTOR_WAITING && millis() - waitStarted >= MOTOR_STOP_DELAY_MS) {
    state = MOTOR_STOPPED;
    Serial.println("[MOTOR TEST] Cooldown complete; stopped");
  }
  if ((state == MOTOR_FORWARD || state == MOTOR_REVERSE) &&
      millis() - runStarted >= MOTOR_MAX_RUN_MS) {
    digitalWrite(MOTOR_TEST_OUTPUT_PIN, LOW);
    state = MOTOR_FAULT;
    Serial.println("[MOTOR TEST] FAULT: maximum runtime exceeded");
  }
}
bool motorCommand(const String& action) {
  if (action == "stop") {
    motorStop();
    return true;
  }
  if (action == "forward" && motorCanForward()) {
    if (state == MOTOR_STOPPED) runStarted = millis();
    state = MOTOR_FORWARD;
    digitalWrite(MOTOR_TEST_OUTPUT_PIN, HIGH);
  } else {
    return false; // Запрет на реверс без STOP и выдержки.
  }
  Serial.print("[MOTOR TEST] ");
  Serial.println(motorStateText());
  return true;
}
