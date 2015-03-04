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
#include "Motorlib.h"
#include <Timers.h>
#include <Servo.h>

/*---------------- Module Defines ---------------------------*/
#define ONE_SEC            1000
#define TIME_INTERVAL      ONE_SEC

#define OFF                0x00
#define ON                 0x01

#define FAST               100
#define MEDIUM             50
#define SLOW               20

#define BACKUP_TIMER       0
#define TURN_TIMER         1
#define SIXTEENTH_SEC      62
#define EIGTH_SEC          125
#define QUARTER_SEC        250
#define HALF_SEC           500
#define THREE_QUARTER_SEC  750
#define FULL_SEC           1000
#define NINETY_DEG         650
#define THREE_SEC          3000
#define FOUR_N_A_HALF_SEC  4000

#define FRONT              0x00
#define BACK               0x01

/*------------------ STATES ----------------------------------*/
enum globalState {
  INIT,
  GET_BALLS,
  TAPE_SENSING,
  DRIVE_STRAIGHT,
  DUNK_BALLS
};


/****** TAPE SENSING STATES ******/
enum tapeSensingState {
  TAPE_SENSING_INIT,
  TURN_90_DEGREES_LEFT,
  BACK_UP_TO_WALL,
  KICK_OFF,
  FOUND_TAPE,
  BACKING_UP,
  START_ROTATING_LEFT,
  KEEP_ROTATING_LEFT,
  ALIGNED
};

/****** DRIVING STRAIGHT STATES ******/
enum drivingStraightState {
  DRIVING_STRAIGHT_INIT,
  GOING_STRAIGHT,
  CORRECTING_DRIFT_RIGHT,
  BACK_AFTER_CORRECTING_DR,
  CORRECTING_DRIFT_LEFT,
  BACK_AFTER_CORRECTING_DL,
  AT_END_OF_COURT
};

/****** GETTING BALLS STATES ******/
enum getBallsState {
  GET_BALLS_INIT,
  FIRST_BALL,
  SECOND_BALL,
  THIRD_BALL,
  DONE_WITH_GETTING_BALLS
};
//////DUNKING BALLS STATE//////////////
enum dunkBallsState {
  DUNK_BALLS_INIT,
  DUNKING,
  RETURN1,
  RETURN2,
  RETURN3,
  DONE_DUNKING
};
  
/******************** initialize global state machine *********/
int globalState = INIT;
//int globalState = GET_BALLS;
//int globalState = DUNK_BALLS;


#define frontTapeSensorPin A0
#define backRightTapeSensorPin A2
#define backLeftTapeSensorPin A1
#define backBumperPin 4
#define servoPin      5


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

void dunkBalls(void);
Servo myservo;
int servoPos = 0;

/**********************************/


boolean isTapeSensorHigh(uint8_t tapeSensorPin, boolean prevStatus){
    unsigned int currPinVal = analogRead(tapeSensorPin);
    
    //if(tapeSensorPin == frontTapeSensorPin){
      //Serial.print("pinval: ");
      //Serial.println(currPinVal,DEC );
    //}
    
    //if last value was high
    if(prevStatus == true){
      //if analog value is greater than 2 volts
      if(currPinVal > 300){
          return true;
      }else{
          return false; 
      }
    }else{
      if(currPinVal > 325){
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

boolean backBumperHit(){
  if(HIGH == digitalRead(4)){
    Serial.println("back bumper touching");
    return true;
  }else{
    //Serial.println("back bumper not touching");
    return false;
  }
}

boolean frontBumperHit(){
  if(HIGH == digitalRead(2)){
    Serial.println("front bumper touching");
    return true;
  }else{
    //Serial.println("back bumper not touching");
    return false;
  }
}

void runStraight(void){
  RightMtrSpeed(55);
  LeftMtrSpeed(60); 
}

void backUp(void){
  RightMtrSpeed(-55);
  LeftMtrSpeed(-65); 
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

void veerRight(){
     RightMtrSpeed(50);
     LeftMtrSpeed(70);
}

void veerLeft(){
     RightMtrSpeed(75);
     LeftMtrSpeed(60);
}

void goStraight(){
    RightMtrSpeed(69);
    LeftMtrSpeed(80);
}

//to make sure we can get out of stall
void kickOff(){
   RightMtrSpeed(70);
   LeftMtrSpeed(80);
  TMRArd_InitTimer(0, QUARTER_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
}

void rotateToLeft(void){
  RightMtrSpeed(75);
  LeftMtrSpeed(-80); 
}

void rotateToRight(void){
  RightMtrSpeed(-75);
  LeftMtrSpeed(80); 
}

void turn90DegreesLeft(void){
  TMRArd_InitTimer(0, NINETY_DEG);
  RightMtrSpeed(-85);
  LeftMtrSpeed(85); 
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  pulseBack();
  stopMtrs();
  TMRArd_InitTimer(0, QUARTER_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
 
}

void pulseBack(void){
  stopMtrs();
  TMRArd_InitTimer(0, EIGTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  RightMtrSpeed(-70);
  LeftMtrSpeed(-70);
  TMRArd_InitTimer(0, EIGTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  stopMtrs();
}

/***** for tape sensing ******/////
void pulseStraight(){
  stopMtrs();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  RightMtrSpeed(63);
  LeftMtrSpeed(67);
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  stopMtrs();
}

void pulseLeft(){
  stopMtrs();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  rotateToLeft();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  stopMtrs();
}

void pulseRight(){
  stopMtrs();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  rotateToRight();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  stopMtrs();
}
/***** for tape sensing ******/////

/***** for Driving Straight sensing ******/////
void pulseStraightDriveStraight(){
  stopMtrs();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  RightMtrSpeed(75);
  LeftMtrSpeed(82);
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  stopMtrs();
}

void pulseLeftDriveStraight(){
  stopMtrs();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  rotateToLeft();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  stopMtrs();
}

void pulseRightDriveStraight(){
  stopMtrs();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  rotateToRight();
  TMRArd_InitTimer(0, SIXTEENTH_SEC);
  while(TestTimerExpired(0) != TMRArd_EXPIRED){
  
  }
  stopMtrs();
}
/***** for Driving Straight sensing ******/////
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
          tapeSensingState = TURN_90_DEGREES_LEFT;
          break;
       case TURN_90_DEGREES_LEFT:
          turn90DegreesLeft();
          //backUp();
          tapeSensingState = BACK_UP_TO_WALL;
          break;
       case BACK_UP_TO_WALL:
          if(backBumperHit()){
             //go straight for a short period of time
             pulseBack();
             TMRArd_InitTimer(0, HALF_SEC);
              while(TestTimerExpired(0) != TMRArd_EXPIRED){
                pulseBack();
              }
              pulseBack();
              tapeSensingState = KICK_OFF;
          }else{
            //Serial.println("Backing up!");
            backUp();
          }
          break;
        case KICK_OFF:
        Serial.println("Kick Off");
          //kickOff();
          pulseStraight();
          if(newLEDPosition == 0x05){
           //for(int i = 0; i<3; i++){
            //pulse backwards to stop
            pulseBack();
            //}
            tapeSensingState = FOUND_TAPE;
          }
        break;
        case FOUND_TAPE:
          Serial.println("FOUND TAPE");
            if(newLEDPosition == 0x02){
//                backUp();
                stopMtrs();
                tapeSensingState = START_ROTATING_LEFT;
            }else{
                pulseBack();
            }
          break;
        case START_ROTATING_LEFT:
            //test for just the front led
            //bitwise and with the second bit position. if this bit is high it evaluate to true, otherwise it is false
            if(newLEDPosition == 0x03 || newLEDPosition == 0x01){
                stopMtrs();
                tapeSensingState = KEEP_ROTATING_LEFT;
            }else{
              pulseLeft();
            }
          break;
        case KEEP_ROTATING_LEFT:
            if(newLEDPosition == 0x02 ){
              for(int i = 0; i<7; i++){
                pulseLeft();
              }
              stopMtrs();
              tapeSensingState = ALIGNED;
            }else if(newLEDPosition == 0x06){
              for(int i = 0; i<2; i++){
                pulseRight();
              }
              stopMtrs();
              tapeSensingState = ALIGNED;
            }
            else{
              pulseLeft();
            }
        break;
        case ALIGNED:
          tapeSensingState = TAPE_SENSING_INIT;
          for(int i=0;i<10;i++){
           pulseStraight(); 
          }
          if(frontBumperHit()){
              Serial.println("go dunk now");
              globalState = DUNK_BALLS;
          }
//else{
//              Serial.println("not dunking");
//              globalState = DRIVE_STRAIGHT;
//       }
        break;
     }
};

unsigned int handleGoingStraight(unsigned char newLEDPosition){
    switch (newLEDPosition) {
      case 0x00:
        globalState = TAPE_SENSING;
        break;
       case 0x02:
       case 0x07:
            //runStraight();
            pulseStraightDriveStraight();
            Serial.println("Going Straight");
          break;
       case 0x01:
          Serial.println("Veering Right");
          //pulseRightShort();
          //veerRight();
          pulseRightDriveStraight();
         break;
       case 0x03:
          Serial.println("Going Straight");
          //goStraight();
          pulseStraightDriveStraight();
          break;
       case 0x04:
         Serial.println("Veering Left");
          //pulseLeftShort();
          //veerLeft();
          pulseLeftDriveStraight();
          break;
       case 0x06:
         Serial.println("Going Straight");
         //runStraight();
         pulseStraightDriveStraight();
       break;
    }
}
/*
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
         pulseLeftShort();
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
         pulseRightShort();
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
           veerLeft();
           return BACK_AFTER_CORRECTING_DR;
         break;
       case 0x06:
         //pulse motors in opposite directions to kill speed
         backUp();   
         veerRight();
         return BACK_AFTER_CORRECTING_DR;
       break;
    }
}
*/

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
    
    if(frontBumperHit()){
      driveStraightState = AT_END_OF_COURT;
      
      TMRArd_InitTimer(0, HALF_SEC);
      while(TestTimerExpired(0) != TMRArd_EXPIRED){
        
      }
    }
    
    
    switch (driveStraightState)
    {
      case DRIVING_STRAIGHT_INIT:
        newDriveStraightState = GOING_STRAIGHT;
        //if(newDriveStraightState != driveStraightState){
          //Serial.println("Driving Straight Init!");
        //}
        break;
      
      case GOING_STRAIGHT:
        handleGoingStraight(newLEDPosition);
        //runStraight();
        //if(newDriveStraightState != driveStraightState){
          //Serial.println("Going Straight");
        //}
        break;
        
      /*
      case CORRECTING_DRIFT_RIGHT:
        newDriveStraightState = handleCorrectingDriftRight(newLEDPosition);
        //if(newDriveStraightState != driveStraightState){
          //Serial.println("correcting drift right");
        //}
        break;
      case BACK_AFTER_CORRECTING_DR:
        newDriveStraightState = handleBackAfterCorrectingDR(newLEDPosition);
       // if(newDriveStraightState != driveStraightState){
          //Serial.println("back after correcting drift right");
        //}
        break;
      case CORRECTING_DRIFT_LEFT:
        newDriveStraightState = handleCorrectingDriftLeft(newLEDPosition);
        //if(newDriveStraightState != driveStraightState){
          //Serial.println("correcting drift left");
        //}
        break;
      case BACK_AFTER_CORRECTING_DL:
        newDriveStraightState = handleBackAfterCorrectingDL(newLEDPosition);
        //if(newDriveStraightState != driveStraightState){
          //Serial.println("back after correcting drift left");
        //}
       break;
     */
       case AT_END_OF_COURT:
        //make sure you are at the end of the court
         for(int i=0;i<5;i++){
           pulseStraight();
         }

         //give room for rim
         for(int i=0;i<2;i++){
           pulseBack();
         }
         stopMtrs();
         globalState = DUNK_BALLS;

       break;
    }
    
    driveStraightState = newDriveStraightState;
}

void getBalls(){
     static int getBallsState = GET_BALLS_INIT;
     
     switch (getBallsState)
    {
      case GET_BALLS_INIT:
          Serial.println("Backing up!");
          backUp();
          getBallsState = FIRST_BALL;
        break;
      case FIRST_BALL:
        if(backBumperHit()){
           Serial.println("motors stopped");
           //go straight for a short period of time
           runStraight();
           TMRArd_InitTimer(0, HALF_SEC);
            while(TestTimerExpired(0) != TMRArd_EXPIRED){
              
            }
           //wait 3 seconds
           stopMtrs();
           TMRArd_InitTimer(0, THREE_SEC);
            while(TestTimerExpired(0) != TMRArd_EXPIRED){
              
            }
            
            backUp();
           getBallsState = SECOND_BALL;
        }else{
          //Serial.println("Backing up!");
        }
        break;
      case SECOND_BALL:
        if(backBumperHit()){
           Serial.println("motors stopped");
           //go straight for a short period of time
           runStraight();
           TMRArd_InitTimer(0, HALF_SEC);
            while(TestTimerExpired(0) != TMRArd_EXPIRED){
              
            }
           //wait 3 seconds
           stopMtrs();
           TMRArd_InitTimer(0, THREE_SEC);
            while(TestTimerExpired(0) != TMRArd_EXPIRED){ 
            }
            backUp();
           getBallsState = THIRD_BALL;
        }else{
          //Serial.println("Backing up!");
          backUp();
        }
        break;
          break;
      case THIRD_BALL:
        if(backBumperHit()){
           //go straight for a short period of time
           runStraight();
           TMRArd_InitTimer(0, FULL_SEC);
            while(TestTimerExpired(0) != TMRArd_EXPIRED){
              
            }
            stopMtrs();
           TMRArd_InitTimer(0, THREE_SEC);
            while(TestTimerExpired(0) != TMRArd_EXPIRED){ 
            }
            getBallsState = DONE_WITH_GETTING_BALLS;
        }else{
          //Serial.println("Backing up!");
          pulseBack();
        }
        break;
       case DONE_WITH_GETTING_BALLS:
       //place holder
         globalState = DRIVE_STRAIGHT;
         break;
    }
}
void dunkBalls(){
  static int dunkBallsState = DUNK_BALLS_INIT;
  switch(dunkBallsState){
    case DUNK_BALLS_INIT:
      Serial.println("DUNK_BALLS_INIT");
      stopMtrs();
      myservo.write(100);
      delay(2*FULL_SEC);
      dunkBallsState = DUNKING;
      break;
    case DUNKING:
      Serial.println("DUNKING");
      myservo.write(140);
      delay(2*FULL_SEC);
      myservo.write(130);
      delay(HALF_SEC);
      myservo.write(120);
      delay(HALF_SEC);
      dunkBallsState = RETURN1;
      break;
    case RETURN1:
      Serial.println("RETURN");
      myservo.write(110);
      delay(HALF_SEC);
      myservo.write(100);
      delay(HALF_SEC);
      myservo.write(90);
      delay(HALF_SEC);
      dunkBallsState = RETURN2;
      break; 
    case RETURN2:
      myservo.write(80);
      delay(HALF_SEC);
      myservo.write(70);
      delay(HALF_SEC);
      dunkBallsState = RETURN3;
      break;   
    case RETURN3:
      myservo.write(68);
      dunkBallsState = DONE_DUNKING;  
      break;
    case DONE_DUNKING:
      //place holder
      break;
     default: 
       stopMtrs();
       break;
  }
//  if the front bumper hits enter dunk balls state // 
}
/*---------------- Arduino Main Functions -------------------*/
void setup() {  // setup() function required for Arduino
  Serial.begin(9600);
  Serial.println("Starting Bot...");
  //Set pins for servo
  myservo.attach(5);
  myservo.write(68);
  MotorInit();
  TMRArd_InitTimer(0, TIME_INTERVAL);
}

void loop() {  // loop() function required for Arduino
    
    if (TestForKey()){
      RespToKey();
    }
   switch (globalState) {
    case INIT:
      //runStraight();
      globalState = GET_BALLS;
      break;
    case GET_BALLS:
      Serial.println("GET BALLS!");
      getBalls();
      break;
    case TAPE_SENSING:
//      Serial.println("Sensing tape!");
      senseTape();
      break;
    case DRIVE_STRAIGHT:
      //driveStraightOnTape();
      Serial.println("DRIVING STRAIGHT");
      if(frontBumperHit()){
        for(int i=0;i<5;i++){
          pulseBack();
        }
          globalState = TAPE_SENSING; 
      }else{
          TMRArd_InitTimer(0, FOUR_N_A_HALF_SEC);
          runStraight();
          while(TestTimerExpired(0) != TMRArd_EXPIRED){ 
          }
      }
      break;
    case DUNK_BALLS:
      Serial.println("DUNKING");
      dunkBalls();
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

