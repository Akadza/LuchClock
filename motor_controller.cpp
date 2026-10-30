#include <Arduino.h>
#include "motor_settings.h"
#include "motor_controller.h"

// Только симуляция: никаких digitalWrite или включения контакторов.
static MotorState state = MOTOR_STOPPED;
static MotorState pendingDirection = MOTOR_STOPPED;
static unsigned long waitStarted = 0;
static unsigned long runStarted = 0;

void motorSetup() {
  state = MOTOR_STOPPED;
  pendingDirection = MOTOR_STOPPED;
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
  unsigned long elapsed = millis() - waitStarted;
  return elapsed >= MOTOR_REVERSE_DELAY_MS ? 0 : MOTOR_REVERSE_DELAY_MS - elapsed;
}
void motorStop() {
  state = MOTOR_STOPPED;
  pendingDirection = MOTOR_STOPPED;
}
void motorLoop() {
  if (state == MOTOR_WAITING &&
      millis() - waitStarted >= MOTOR_REVERSE_DELAY_MS) {
    state = pendingDirection;
    pendingDirection = MOTOR_STOPPED;
    runStarted = millis();
    Serial.println("[MOTOR SIM] Reversal delay complete");
  }
  if ((state == MOTOR_FORWARD || state == MOTOR_REVERSE) &&
      millis() - runStarted >= MOTOR_MAX_RUN_MS) {
    motorStop();
    state = MOTOR_FAULT;
    Serial.println("[MOTOR SIM] FAULT: maximum runtime exceeded");
  }
}
bool motorCommand(const String& action) {
  if (action == "stop") {
    motorStop();
    Serial.println("[MOTOR SIM] STOP");
    return true;
  }
  MotorState requested;
  if (action == "forward") requested = MOTOR_FORWARD;
  else if (action == "backward") requested = MOTOR_REVERSE;
  else return false;
  if (state == MOTOR_FAULT) return false;
  if (state == MOTOR_WAITING) {
    // Новая команда во время ожидания не ускоряет реверс.
    pendingDirection = requested;
    return true;
  }
  if (state == requested) return true;
  if (state == MOTOR_STOPPED) {
    state = requested;
    runStarted = millis();
  } else {
    // Сначала STOP, затем выдержка и лишь потом новое направление.
    pendingDirection = requested;
    waitStarted = millis();
    state = MOTOR_WAITING;
  }
  Serial.print("[MOTOR SIM] State: ");
  Serial.println(motorStateText());
  return true;
}
