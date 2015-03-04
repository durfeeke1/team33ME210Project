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
#define NINETY_DEG         550
#define ALIGN_NINETY_DEG_RIGHT  850
#define ALIGN_NINETY_DEG_LEFT  800
#define TWO_SEC            2000
#define THREE_SEC          3000
#define FOUR_SEC           4000

#define FRONT              0x00
#define BACK               0x01

/*------------------ STATES ----------------------------------*/
enum globalState {
  INIT,
  GET_BALLS,
  ALIGN_TO_DRIVE_STRAIGHT,
  TAPE_SENSING,
  DRIVE_STRAIGHT,
  DUNK_BALLS
};


/****** TAPE SENSING STATES ******/
enum tapeSensingState {
  TAPE_SENSING_INIT,
  TURN_90_DEGREES_RIGHT,
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


enum alignToDriveStraightState {
  ALIGN_TDS_INIT,
  ALIGN_TDS_TURN_90_RIGHT,
  ALIGN_TDS_BACK_UP_TO_WALL,
  ALIGN_TDS_TURN_90_LEFT,

};

/******************** initialize global state machine *********/
int globalState = INIT;
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
  RightMtrSpeed(54);
  LeftMtrSpeed(60); 
}

void backUp(void){
  RightMtrSpeed(-55);
  LeftMtrSpeed(-65); 
}

//for when we loop back and need to get back to the beginning of the court
void backUpStraight(void){
  RightMtrSpeed(-60);
  LeftMtrSpeed(-65); 
}

void backUpHard(void){
  RightMtrSpeed(-75);
  LeftMtrSpeed(-83); 
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
     RightMtrSpeed(65 );
     LeftMtrSpeed(60);
}

void goStraight(){
    RightMtrSpeed(69);
    LeftMtrSpeed(80);
}

//to make sure we can get out of stall
void kickOff(){
   RightMtrSpeed(55);
   LeftMtrSpeed(60);
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

void turn90DegreesRight(void){
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

void alignTurn90DegreesRight(void){
  TMRArd_InitTimer(0, ALIGN_NINETY_DEG_RIGHT);
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

void alignTurn90DegreesLeft(void){
  TMRArd_InitTimer(0, ALIGN_NINETY_DEG_LEFT);
  RightMtrSpeed(85);
  LeftMtrSpeed(-85); 
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
  RightMtrSpeed(65);
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
          tapeSensingState = TURN_90_DEGREES_RIGHT;
          break;
       case TURN_90_DEGREES_RIGHT:
          turn90DegreesRight();
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
            backUpHard();
          }
          break;
        case KICK_OFF:
//        Serial.println("Kick Off");
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
//          Serial.println("FOUND TAPE");
            if(newLEDPosition == 0x02){
                backUp();
                stopMtrs();
                tapeSensingState = START_ROTATING_LEFT;
            }else{
                pulseBack();
            }
          break;
        case START_ROTATING_LEFT:
            //test for just the front led
            //bitwise and with the second bit position. if this bit is high it evaluate to true, otherwise it is false
            if(newLEDPosition == 0x03 || newLEDPosition == 0x01 || newLEDPosition == 0x04){
                stopMtrs();
                tapeSensingState = KEEP_ROTATING_LEFT;
            }else{
              pulseLeft();
            }
          break;
        case KEEP_ROTATING_LEFT:
            if(newLEDPosition == 0x02 ){
              for(int i = 0; i<4; i++){
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
            else if (newLEDPosition == 0x01){
              for(int i=0;i<2;i++){
                pulseRight();
              }
              stopMtrs();
              tapeSensingState = ALIGNED;
            } else {
              pulseLeft();
            }
        break;
        case ALIGNED:
           pulseStraight(); 
           //runStraight();
          if(frontBumperHit()){
              tapeSensingState = TAPE_SENSING_INIT;
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
         driveStraightState = DRIVING_STRAIGHT_INIT;
         globalState = DUNK_BALLS;

       break;
    }
    
    driveStraightState = newDriveStraightState;
}

void alignToDriveStraight(){
  static int alignToDriveStraightState = ALIGN_TDS_INIT;
  switch (alignToDriveStraightState)
    {
       case ALIGN_TDS_INIT:
         Serial.println("ALIGN_TDS_INIT");
         alignToDriveStraightState = ALIGN_TDS_TURN_90_RIGHT;
       break;
       case ALIGN_TDS_TURN_90_RIGHT:
           Serial.println("ALIGN_TDS_TURN_90_RIGHT");  
          alignTurn90DegreesRight();
          alignToDriveStraightState = ALIGN_TDS_BACK_UP_TO_WALL;
          break;
       case ALIGN_TDS_BACK_UP_TO_WALL:
         Serial.println("ALIGN_TDS_BACK_UP_TO_WALL");  
          if(backBumperHit()){
             pulseBack();
             TMRArd_InitTimer(0, HALF_SEC);
              while(TestTimerExpired(0) != TMRArd_EXPIRED){
                pulseBack();
              }
              pulseBack();
              alignToDriveStraightState = ALIGN_TDS_TURN_90_LEFT;
          }else{
            //Serial.println("Backing up!");
            backUpHard();
          }
         break;
       case ALIGN_TDS_TURN_90_LEFT:
          Serial.println("ALIGN_TDS_TURN_90_LEFT");  
          kickOff();
          kickOff();
          alignTurn90DegreesLeft();
          //backUp();
          alignToDriveStraightState = ALIGN_TDS_INIT;
          globalState = DRIVE_STRAIGHT;
          break;
          
    }
}

void getBalls(){
     static int getBallsState = GET_BALLS_INIT;
     
     switch (getBallsState)
    {
      case GET_BALLS_INIT:
          Serial.println("Backing up!");
          backUpStraight();
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
         getBallsState = GET_BALLS_INIT;
         globalState = ALIGN_TO_DRIVE_STRAIGHT;
         break;
    }
}
void dunkBalls(){
  static unsigned char LEDPosition = 0x00;
  static unsigned char lastLEDPosition = 0x02;
  unsigned char newLEDPosition = 0x00;
  static int dunkBallsState = DUNK_BALLS_INIT;
  static boolean dunkedOnce = false;
    
   newLEDPosition = getNewLEDPosition();
    //if we changed our positioning print it out
    if(newLEDPosition != LEDPosition){
      Serial.print("LED Position: ");
      Serial.println(newLEDPosition,HEX);
    }

    LEDPosition = newLEDPosition;
    
  
  switch(dunkBallsState){
    case DUNK_BALLS_INIT:
      Serial.println("DUNK_BALLS_INIT");
      stopMtrs();
      for(int i=0;i<2;i++){
        pulseBack();
      }
      if (lastLEDPosition == 0x01 || LEDPosition == 0x01){
      for(int i=0;i<5;i++){
        pulseRight();
        }
      }
//      if (lastLEDPosition == 0x03 || LEDPosition == 0x03){
//      for(int i=0;i<3;i++){
//        pulseRight();
//        }
//      }
      else if (lastLEDPosition == 0x04 || LEDPosition == 0x04){
      for(int i=0;i<5;i++){
        pulseLeft();
        }
      }else if (lastLEDPosition == 0x03 || LEDPosition == 0x03){
      for(int i=0;i<5;i++){
        pulseLeft();
        }
      }
//      else if (lastLEDPosition == 0x06 || LEDPosition == 0x06){
//      for(int i=0;i<3;i++){
//        pulseLeft();
//        }
//      }
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
      if(dunkedOnce){
        dunkBallsState = RETURN1;
      }else{
        dunkedOnce = true;
        dunkBallsState = DUNKING;
      }
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
      dunkedOnce = false;
      dunkBallsState = DUNK_BALLS_INIT; 
      globalState = GET_BALLS;
      break;
     default: 
       stopMtrs();
       break;
  }
  if(newLEDPosition != 0x00){
    lastLEDPosition = newLEDPosition;
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
//      runStraight();
      globalState = GET_BALLS;
      break;
    case GET_BALLS:
      Serial.println("GET BALLS!");
      getBalls();
      break;
    case ALIGN_TO_DRIVE_STRAIGHT:
      Serial.println("ALIGN TO DRIVE STRAIGHT!");
      alignToDriveStraight();
      break;
    case TAPE_SENSING:
//      Serial.println("Sensing tape!");
      senseTape();
      break;
    case DRIVE_STRAIGHT:
      //driveStraightOnTape();
      Serial.println("DRIVING STRAIGHT");
      if(frontBumperHit()){
        for(int i=0;i<4;i++){
          pulseBack();
        }
          globalState = TAPE_SENSING; 
      }else{
          TMRArd_InitTimer(0, TWO_SEC);
          while(TestTimerExpired(0) != TMRArd_EXPIRED){
            //runStraight();
            veerLeft();
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

