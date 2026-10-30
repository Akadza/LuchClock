#pragma once
#include <Arduino.h>
enum MotorState { MOTOR_STOPPED, MOTOR_FORWARD, MOTOR_REVERSE, MOTOR_WAITING, MOTOR_FAULT };
void motorSetup();
void motorLoop();
void motorStop();
bool motorCommand(const String& action);
MotorState motorState();
const char* motorStateText();
unsigned long motorWaitRemainingMs();
bool motorCanForward();
bool motorCanReverse();
bool motorCanStop();
