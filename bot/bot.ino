/**************************************************************
  File:      roach.ino
  Contents:  This program gives the arduino powered roach the 
             following behavior:
             1) run around when the lights are on
             2) stop running when the lights are off
             3) avoid getting stuck in corners or other obstacles
  Notes:    Target: Arduino Uno R1 & R2
            Arduino IDE version: 1.0.6 & 1.5.8 BETA

  History:
  when      who  what/why
  ----      ---  -------------------------------------------
  01/26/15  KD  program created
**************************************************************/

/*---------------- Includes ---------------------------------*/
#include <Roachlib.h>
#include <Timers.h>

/*---------------- Module Defines ---------------------------*/
#define LIGHT_THRESHOLD    125
#define LIGHT_THRESHOLD_HI LIGHT_THRESHOLD + 10
#define LIGHT_THRESHOLD_LO LIGHT_THRESHOLD - 10
#define ONE_SEC            1000
#define TIME_INTERVAL      ONE_SEC
#define HEARTBEAT_LED      13

#define OFF                0x00
#define ON                 0x01

#define FAST               10
#define MEDIUM             12
#define SLOW               2

#define BACKUP_TIMER       0
#define TURN_TIMER         1
#define HALF_SEC           500

#define FRONT              0x00
#define BACK               0x01

/*------------------ STATES ----------------------------------*/
#define INIT               0x00
#define RUN                0x01
#define STOP               0x02
#define BACK_UP            0x03
#define TURN               0x04
#define STOPPED_BACKING_UP 0x05
#define STOPPED_TURNING    0x06



/*---------------- Module Function Prototypes ---------------*/
unsigned char TestForKey(void);
void RespToKey(void);
unsigned char TestForLightOn(void);
void RespToLightOn(void);
unsigned char TestForLightOff(void);
void RespToLightOff(void);
unsigned char TestForBump(void);
void RespToBump(void);
unsigned char TestTimerExpired(unsigned char);
void RespTimerExpired(void);
void outputState(char);
void runStraight(void);
void backUp(void);
void stopMtrs(void);
void turnRight(void);
void turnLeft(void);

/*---------------- Arduino Main Functions -------------------*/
void setup() {  // setup() function required for Arduino
  Serial.begin(9600);
  Serial.println("Starting Bot...");
  RoachInit();
  TMRArd_InitTimer(0, TIME_INTERVAL);
}

void loop() {  // loop() function required for Arduino
    if (TestForKey()){
      RespToKey();
    }
}

/*---------------- Module Functions -------------------------*/
unsigned char TestForKey(void) {
  unsigned char KeyEventOccurred;
  
  KeyEventOccurred = Serial.available();
  return KeyEventOccurred;
}

void RespToKey(void) {
  unsigned char theKey;
  
  theKey = Serial.read();
  Serial.print(theKey);
  Serial.print(", ASCII=");
  Serial.println(theKey,HEX);
  
  //if pressed "g" then go
  if(theKey == 0x67){
    runStraight();
  }
  //if pressed "s" then stop
  else if(theKey == 0x73){
    stopMtrs(); 
  }
  
}

unsigned char TestForLightOn(void) {
  char EventOccurred;
  unsigned int ThisLight;
  static unsigned int LastLight = 0;
  
  ThisLight = LightLevel();
  EventOccurred = ((ThisLight >= LIGHT_THRESHOLD_HI));

  LastLight = ThisLight;
  return EventOccurred;
}

void RespToLightOn(void) {
  Serial.println("  ON");
  LeftMtrSpeed(5);
  RightMtrSpeed(-5);
}

unsigned char TestForLightOff(void) {
  char EventOccurred;
  unsigned int ThisLight;
  static unsigned int LastLight = 0;

  ThisLight = LightLevel();
  EventOccurred = ((ThisLight < LIGHT_THRESHOLD_LO));
  
  LastLight = ThisLight;
  return EventOccurred;
}

void RespToLightOff(void) {
  Serial.println("  OFF");
  LeftMtrSpeed(0);
  RightMtrSpeed(0);
}

unsigned char TestForBump(void) {
  static unsigned char LastBumper = 0xF0;
  unsigned char bumper;
  unsigned char EventOccurred;
  
  bumper = ReadBumpers();
  EventOccurred = ((bumper != 0xF0) && (bumper != LastBumper));
  if (EventOccurred) {
    SET_SHARED_BYTE_TO(bumper);
    LastBumper = bumper;
  }
  return EventOccurred;
}

void RespToBump(void) {
    unsigned char bumper;

    bumper = GET_SHARED_BYTE();

    // display which bumper(s) were hit
    switch (bumper) {
        case (0xD0):  Serial.println("  Front Right...");
          backUp();
          break;
        case (0xE0):  Serial.println("  Front Left...");
          backUp();
          break;
        case (0x70):  Serial.println("  Back Left..."); 
        break;
        case (0xB0):  Serial.println("  Back Right..."); 
        break;
        case (0xC0):  Serial.println("  Both Front ..."); 
          backUp();
          break;
        case (0x30):  Serial.println("  Both Back..."); 
        break;
        case (0x90):  Serial.println("  Both Right..."); 
        break;
        case (0x60):  Serial.println("  Both Left..."); 
        break;

		// should never get here unless borked
        default:      Serial.print("  What is this I don�t even->");
                      Serial.println(bumper,HEX);
    }
}

unsigned char TestTimerExpired(unsigned char channelNum) {
  return (unsigned char)TMRArd_IsTimerExpired(channelNum);
}

void RespTimerExpired(void) {
  static int Time = 0;

  TMRArd_InitTimer(0, TIME_INTERVAL);

  Serial.print(" time:");
  Serial.print(++Time);
  Serial.print(" light:");
  Serial.println(LightLevel(),DEC);
}

void outputState(char state) {
  switch(state) {
   case(INIT):
    Serial.print("State: ");
    Serial.println("INIT");
    break; 
    case(RUN):
    Serial.print("State: ");
    Serial.println("RUN");
    break; 
    case(STOP):
    Serial.print("State: ");
    Serial.println("STOP");
    break; 
    case(BACK_UP):
    Serial.print("State: ");
    Serial.println("BACK_UP");
    break; 
    case(TURN):
    Serial.print("State: ");
    Serial.println("TURN");
    break; 
  }
}

void runStraight(void){
  RightMtrSpeed(MEDIUM);
  LeftMtrSpeed(MEDIUM); 
}

void backUp(void){
  RightMtrSpeed(-MEDIUM);
  LeftMtrSpeed(-MEDIUM); 
}

void stopMtrs(void){
  RightMtrSpeed(0);
  LeftMtrSpeed(0); 
}

void turnRight(void){
  RightMtrSpeed(-MEDIUM);
  LeftMtrSpeed(MEDIUM); 
}

void turnLeft(void){
  RightMtrSpeed(-MEDIUM);
  LeftMtrSpeed(MEDIUM); 
}


