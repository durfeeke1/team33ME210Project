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
#define ONE_SEC            1000
#define TIME_INTERVAL      ONE_SEC

#define OFF                0x00
#define ON                 0x01

#define FAST               10
#define MEDIUM             5
#define SLOW               2

#define BACKUP_TIMER       0
#define TURN_TIMER         1
#define SIXTEENTH_SEC      62
#define EIGTH_SEC          125
#define QUARTER_SEC        250
#define HALF_SEC           500
#define FULL_SEC           1000

#define FRONT              0x00
#define BACK               0x01

/*------------------ STATES ----------------------------------*/
enum globalState {
  INIT,
  TAPE_SENSING,
  DRIVE_STRAIGHT
};


/****** TAPE SENSING STATES ******/
enum tapeSensingState {
  TAPE_SENSING_INIT,
  FOUND_TAPE,
  BACKING_UP,
  START_ROTATING_LEFT,
  ROTATE_AROUND_RIGHT_WHEEL
};

/****** DRIVING STRAIGHT STATES ******/
enum drivingStraightState {
  DRIVING_STRAIGHT_INIT,
  GOING_STRAIGHT,
  CORRECTING_DRIFT_RIGHT,
  BACK_AFTER_CORRECTING_DR,
  CORRECTING_DRIFT_LEFT,
  BACK_AFTER_CORRECTING_DL
};

#define frontTapeSensorPin A2
#define backRightTapeSensorPin A1
#define backLeftTapeSensorPin A0


/*---------------- Module Function Prototypes ---------------*/
unsigned char TestForKey(void);
void RespToKey(void);
unsigned char TestForLightOn(void);
void RespToLightOn(void);
unsigned char TestForLightOff(void);
void RespToLightOff(void);
unsigned char TestTimerExpired(unsigned char);
void RespTimerExpired(void);
void outputState(char);
void runStraight(void);
void backUp(void);
void stopMtrs(void);
void turnRight(void);
void turnLeft(void);
void respondToTapeBL(void);
void respondToTapeFront(void);
void senseTape(void);
boolean isTapeSensorHigh(unsigned int,boolean);

/**********************************/
int globalState = TAPE_SENSING;

boolean isTapeSensorHigh(uint8_t tapeSensorPin, boolean prevStatus){
    unsigned int currPinVal = analogRead(tapeSensorPin);
    
    if(tapeSensorPin == backLeftTapeSensorPin){
      //Serial.print("pinval: ");
      //Serial.println(currPinVal,DEC );
    }
    
    //if last value was high
    if(prevStatus == true){
      //if analog value is greater than 2 volts
      if(currPinVal > 275){
          return true;
      }else{
          return false; 
      }
    }else{
      if(currPinVal > 300){
          return true;
      }else{
          return false; 
      }
    }
}

boolean frontTapeSensorHigh(void){
  static boolean currStatus = false;
  currStatus = isTapeSensorHigh(frontTapeSensorPin, currStatus);
  return currStatus;
}

boolean backRightTapeSensorHigh(void){
  static boolean currStatus = false;
  currStatus = isTapeSensorHigh(backRightTapeSensorPin, currStatus);
  return currStatus;
}

boolean backLeftTapeSensorHigh(void){
  static boolean currStatus = false;
  currStatus = isTapeSensorHigh(backLeftTapeSensorPin, currStatus);
  return currStatus;
}

unsigned char getNewLEDPosition(void){
    unsigned char newLEDPosition = 0x00;
     if(backRightTapeSensorHigh()){
      newLEDPosition |= 0x01;
    }
    if(frontTapeSensorHigh()){
      newLEDPosition |= 0x02;
    }
    if(backLeftTapeSensorHigh()){
      newLEDPosition |= 0x04;
    } 
    return newLEDPosition;
}

void veerRight(){
     RightMtrSpeed(4);
     LeftMtrSpeed(6);
}

void veerLeft(){
     RightMtrSpeed(6);
     LeftMtrSpeed(5);
}

void goStraight(){
    RightMtrSpeed(6);
    LeftMtrSpeed(7);
}

void rotateToLeft(void){
  RightMtrSpeed(6);
  LeftMtrSpeed(-5); 
}

void turnAroundRightWheel(void){
  RightMtrSpeed(1);
  LeftMtrSpeed(-6); 
}


void senseTape(void){
    static int tapeSensingState = TAPE_SENSING_INIT;
    static unsigned char LEDPosition = 0x00;
    unsigned char newLEDPosition = 0x00;
    
    newLEDPosition = getNewLEDPosition();
    //if we changed our positioning print it out
    if(newLEDPosition != LEDPosition){
      Serial.print("LED Position: ");
      Serial.println(newLEDPosition,HEX);
    }
    LEDPosition = newLEDPosition;

     switch (tapeSensingState) {
        case TAPE_SENSING_INIT:
          goStraight();
          if(newLEDPosition == 0x02){
            //pulse to stop
           
            stopMtrs();
            tapeSensingState = FOUND_TAPE;
             TMRArd_InitTimer(0, HALF_SEC);
            while(TestTimerExpired(0) != TMRArd_EXPIRED){
              //  backUp();
            }
          }
          break;
        case FOUND_TAPE:
            if(newLEDPosition != 0x05){
               backUp();
            }else{
              stopMtrs();
              tapeSensingState = START_ROTATING_LEFT;
            }
          break;
        case START_ROTATING_LEFT:
            //test for just the front led
            //bitwise and with the second bit position. if this bit is high it evaluate to true, otherwise it is false
            if(newLEDPosition == 0x04){
                stopMtrs();
                tapeSensingState = ROTATE_AROUND_RIGHT_WHEEL;
            }else{
              rotateToLeft();
            }
          break;
        case ROTATE_AROUND_RIGHT_WHEEL:
            if(newLEDPosition == 0x02 || newLEDPosition == 0x04 || newLEDPosition == 0x01){
                stopMtrs();
                globalState = DRIVE_STRAIGHT;
            }else{
              turnAroundRightWheel();
            }
        break;
     }
};

unsigned int handleGoingStraight(unsigned char newLEDPosition){
    switch (newLEDPosition) {
      case 0x00:
        break;
       case 0x02:
       case 0x07:
            goStraight();
            return GOING_STRAIGHT;
          break;
       case 0x01:
          veerRight();
          return CORRECTING_DRIFT_LEFT;
         break;
       case 0x03:
       break;
       case 0x04:
          veerLeft();
          return CORRECTING_DRIFT_RIGHT;
          break;
       case 0x06:
       break;
    }
}

unsigned int handleCorrectingDriftLeft(unsigned char newLEDPosition){
    switch (newLEDPosition) {
      case 0x00:
          break;
       case 0x02:
       case 0x07:
          break;
       case 0x01:
          veerRight();
          return CORRECTING_DRIFT_LEFT;
         break;
       case 0x03:
         //pulse motors in opposite directions to kill speed
         backUp();
         //turnRight();
         veerLeft();
         return BACK_AFTER_CORRECTING_DL;
         break;
       case 0x04:
         break;
       case 0x06:
         break;
    }
}

unsigned int handleBackAfterCorrectingDL(unsigned char newLEDPosition){
    switch (newLEDPosition) {
      case 0x00:
            break;
       case 0x02:
       case 0x07:
            goStraight();
            return GOING_STRAIGHT;
            break;
       case 0x01:
          break;
       case 0x03:
         //pulse motors in opposite directions to kill speed
         backUp();
         //turnRight();
         veerLeft();
         return BACK_AFTER_CORRECTING_DL;
         break;
       case 0x04:
          break;
       case 0x06:
          break;
    }
}

unsigned int handleCorrectingDriftRight(unsigned char newLEDPosition){
    switch (newLEDPosition) {
      case 0x00:
        break;
       case 0x02:
       case 0x07:
          break;
       case 0x01:
         break;
       case 0x03:
       break;
       case 0x04:
          veerLeft();
          return CORRECTING_DRIFT_RIGHT;
          break;
       case 0x06:
         //pulse motors in opposite directions to kill speed
         backUp();   
         veerRight();
         return BACK_AFTER_CORRECTING_DR;
       break;
    }
}

unsigned int handleBackAfterCorrectingDR(unsigned char newLEDPosition){
    switch (newLEDPosition) {
      case 0x00:
        break;
       case 0x02:
       case 0x07:
          goStraight();
          return GOING_STRAIGHT;
          break;
       case 0x01:
         break;
       case 0x03:
         break;
       case 0x04:
         break;
       case 0x06:
         //pulse motors in opposite directions to kill speed
         backUp();   
         veerRight();
         return BACK_AFTER_CORRECTING_DR;
       break;
    }
}


void driveStraightOnTape(){
    static unsigned int driveStraightState = DRIVING_STRAIGHT_INIT;
    unsigned int newDriveStraightState = 0;
    static unsigned char LEDPosition = 0x00;
    unsigned char newLEDPosition = 0x00;
    
    newLEDPosition = getNewLEDPosition();
    //if we changed our positioning print it out
    if(newLEDPosition != LEDPosition){
      Serial.print("LED Position: ");
      Serial.println(newLEDPosition,HEX);
    }

    LEDPosition = newLEDPosition;
    
    switch (driveStraightState)
    {
      case DRIVING_STRAIGHT_INIT:
        newDriveStraightState = GOING_STRAIGHT;
        //if(newDriveStraightState != driveStraightState){
          Serial.println("Init!");
        //}
        break;
      case GOING_STRAIGHT:
        newDriveStraightState = handleGoingStraight(newLEDPosition);
        //if(newDriveStraightState != driveStraightState){
          Serial.println("Going Straight");
        //}
        break;
      case CORRECTING_DRIFT_RIGHT:
        newDriveStraightState = handleCorrectingDriftRight(newLEDPosition);
        //if(newDriveStraightState != driveStraightState){
          Serial.println("correcting drift right");
        //}
        break;
      case BACK_AFTER_CORRECTING_DR:
        newDriveStraightState = handleBackAfterCorrectingDR(newLEDPosition);
       // if(newDriveStraightState != driveStraightState){
          Serial.println("back after correcting drift right");
        //}
        break;
      case CORRECTING_DRIFT_LEFT:
        newDriveStraightState = handleCorrectingDriftLeft(newLEDPosition);
        //if(newDriveStraightState != driveStraightState){
          Serial.println("correcting drift left");
        //}
        break;
      case BACK_AFTER_CORRECTING_DL:
        newDriveStraightState = handleBackAfterCorrectingDL(newLEDPosition);
        //if(newDriveStraightState != driveStraightState){
          Serial.println("back after correcting drift left");
        //}
       break;
    }
    
    driveStraightState = newDriveStraightState;
}

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
    
   switch (globalState) {
    case INIT:
      runStraight();
      globalState = TAPE_SENSING;
      break;
    case TAPE_SENSING:
      //Serial.println("Sensing tape!");
      senseTape();
      break;
    case DRIVE_STRAIGHT:
      driveStraightOnTape();
      break;
    default: 
      stopMtrs();
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
    case(TAPE_SENSING):
    Serial.print("State: ");
    Serial.println("TAPE_SENSING");
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

void backUpHard(void){
  RightMtrSpeed(-FAST);
  LeftMtrSpeed(-FAST); 
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
  RightMtrSpeed(MEDIUM);
  LeftMtrSpeed(-MEDIUM); 
}


