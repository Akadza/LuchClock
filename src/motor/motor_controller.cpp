#include <Arduino.h>
#include "../system_settings.h"
#include "../application/hardware.h"
#include "motor_controller.h"

static MotorState state=MOTOR_STOPPED;
static unsigned long waitStarted=0, runStarted=0, manualPausedAt=0;
static bool manualPaused=false, autoMode=false, autoFault=false;
static unsigned long manualPauseDurationMs=0;
static unsigned long stopDelayMs=DEFAULT_MOTOR_STOP_DELAY_MS;
static int16_t lastPosition=-1;
static unsigned long lastMovementAt=0;
static const char* faultReason="";

void motorSetup(){
  digitalWrite(MOTOR_OUTPUT_PIN,LOW);
  pinMode(MOTOR_OUTPUT_PIN,OUTPUT);
  state=MOTOR_STOPPED; autoMode=false; autoFault=false; manualPaused=false;
  waitStarted=runStarted=manualPauseDurationMs=0;
  lastPosition=-1;lastMovementAt=0;faultReason="";
}
MotorState motorState(){return state;}
const char* motorStateText(){
  switch(state){
    case MOTOR_FORWARD:return "forward";
    case MOTOR_REVERSE:return "reverse";
    case MOTOR_WAITING:return "waiting";
    case MOTOR_FAULT:return "fault";
    default:return "stopped";
  }
}
unsigned long motorRunElapsedMs(){return state==MOTOR_FORWARD ? millis()-runStarted : 0;}
unsigned long motorManualPauseRemainingMs(){
  if(!manualPaused)return 0;
  const unsigned long duration=manualPauseDurationMs;
  const unsigned long elapsed=millis()-manualPausedAt;
  return elapsed>=duration?0:duration-elapsed;
}
unsigned long motorWaitRemainingMs(){
  if(state!=MOTOR_WAITING)return 0;
  unsigned long elapsed=millis()-waitStarted;
  return elapsed>=stopDelayMs?0:stopDelayMs-elapsed;
}
bool motorAutoFault(){return autoFault;}
const char* motorFaultReason(){return faultReason;}
void motorSetAutoMode(bool enabled){autoMode=enabled;}
bool motorResetAutoFault(){
  if(!autoFault || state==MOTOR_FORWARD || state==MOTOR_REVERSE)return false;
  autoFault=false;faultReason="";
  if(state==MOTOR_FAULT)state=MOTOR_STOPPED;
  Serial.println(F("[MOTOR] Auto fault reset (no restart)"));
  return true;
}
bool motorCanForward(){
  return !autoMode && !autoFault && state==MOTOR_STOPPED &&
    motorManualPauseRemainingMs()==0 && encoder().encoderValid();
}
bool motorCanReverse(){return false;}
bool motorCanStop(){return true;}
void motorStop(){
  digitalWrite(MOTOR_OUTPUT_PIN,LOW);
  if(state==MOTOR_FORWARD || state==MOTOR_REVERSE){

    state=MOTOR_WAITING;waitStarted=millis();stopDelayMs=DEFAULT_MOTOR_STOP_DELAY_MS;
    Serial.println(F("[MOTOR] STOP"));
  }
}
void motorLoop(){
  if(state==MOTOR_FORWARD && !encoder().encoderValid()){
    digitalWrite(MOTOR_OUTPUT_PIN,LOW);

    state=MOTOR_FAULT;
    autoFault=true;faultReason="encoder_lost";
    Serial.println(F("[MOTOR] CRITICAL: encoder lost during movement"));
  }
  if(state==MOTOR_FORWARD){
    // Encoder movement must be observed during motor operation.
    const int16_t position=encoder().encoderRawPosition();
    if(position!=lastPosition){
      lastPosition=position;lastMovementAt=millis();
    }else if(millis()-lastMovementAt>=settingsCurrent().noMotionWarnSec*1000UL){
      digitalWrite(MOTOR_OUTPUT_PIN,LOW);

      state=MOTOR_FAULT;autoFault=true;faultReason="no_motion";
      Serial.println(F("[MOTOR] CRITICAL: no encoder movement"));
    }
  }
  if(state==MOTOR_FORWARD){
    const unsigned long limit=(autoMode?settingsCurrent().autoLimitSec:settingsCurrent().manualLimitSec)*1000UL;
    if(millis()-runStarted>=limit){
      digitalWrite(MOTOR_OUTPUT_PIN,LOW);

      if(autoMode){
        state=MOTOR_FAULT;autoFault=true;faultReason="auto_limit";
        Serial.println(F("[MOTOR] CRITICAL: automatic runtime limit"));
      }else{
        state=MOTOR_WAITING;waitStarted=millis();
        stopDelayMs=DEFAULT_MOTOR_STOP_DELAY_MS;
        manualPauseDurationMs=settingsCurrent().manualPauseSec*1000UL;
        manualPaused=true;manualPausedAt=millis();
        Serial.println(F("[MOTOR] WARNING: manual runtime limit, protective pause"));
      }
    }
  }
  if(state==MOTOR_WAITING && millis()-waitStarted>=stopDelayMs)state=MOTOR_STOPPED;
  if(manualPaused && motorManualPauseRemainingMs()==0)manualPaused=false;
}
bool motorCommand(const String& action){
  if(action=="stop"){motorStop();return true;}
  if(action!="forward" || !motorCanForward())return false;
  runStarted=millis();state=MOTOR_FORWARD;
  lastPosition=encoder().encoderRawPosition();lastMovementAt=millis();
  digitalWrite(MOTOR_OUTPUT_PIN,HIGH);
  Serial.println(F("[MOTOR] FORWARD"));
  return true;
}
