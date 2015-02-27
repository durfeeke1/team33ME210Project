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
#define MEDIUM             5
#define SLOW               2

#define BACKUP_TIMER       0
#define TURN_TIMER         1
#define EIGTH_SEC          125
#define QUARTER_SEC          125
#define HALF_SEC           500
#define FULL_SEC           1000

#define FRONT              0x00
#define BACK               0x01

/*------------------ STATES ----------------------------------*/
enum globalState {
  INIT,
  TAPE_SENSING,
};


/****** TAPE SENSING STATES ******/
enum tapeSensingState {
  TAPE_SENSING_INIT,
  FOUND_TAPE,
  BACKING_UP,
  START_TURNING_RIGHT,
  DRIVE_STRAIGHT,
  TURNING_RIGHT
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
int tapeSensingState = DRIVE_STRAIGHT;

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


void senseTape(void){
    static unsigned char LEDPosition = 0x00;
    unsigned char newLEDPosition = 0x00;
     switch (tapeSensingState) {
        case TAPE_SENSING_INIT:
          //handled in interrupt functions
          __asm__("nop\n\t");
          break;
        case FOUND_TAPE:
          tapeSensingState = START_TURNING_RIGHT;
          break;
        case START_TURNING_RIGHT:
          turnRight();
          tapeSensingState = TURNING_RIGHT;
          break;
        case TURNING_RIGHT:
         //handled in interrupt
          break;
        case DRIVE_STRAIGHT:
          //Serial.println("driving straight");
          if(backRightTapeSensorHigh()){
            newLEDPosition |= 0x01;
          }
          if(frontTapeSensorHigh()){
            newLEDPosition |= 0x02;
          }
          if(backLeftTapeSensorHigh()){
            newLEDPosition |= 0x04;
          }
          if(newLEDPosition != LEDPosition){
            Serial.print("LED Position: ");
            Serial.println(newLEDPosition,HEX);
          }
          LEDPosition = newLEDPosition;
          
          switch (newLEDPosition) {
             case 0x00:
               stopMtrs();
               break;
             case 0x02:
             case 0x07:
                //Serial.println("Drving Straight");
//                runStraight();
                  RightMtrSpeed(7);
                  LeftMtrSpeed(7);
                break;
                
             case 0x01:
             case 0x03:
               //pulse motors in opposite directions to kill speed
               backUp();
               turnRight();
                 RightMtrSpeed(5);
                 LeftMtrSpeed(4); 
               for(int i = 0; i<50;i++){
                  __asm__("nop\n\t");
               }
               
             break;
             
             case 0x04:
             case 0x06:
               //pulse motors in opposite directions to kill speed
               backUp();   
               RightMtrSpeed(4);
               LeftMtrSpeed(5);
               for(int i = 0; i<50;i++){
                  __asm__("nop\n\t");
               }
              Serial.println("breaking");
             break;
          }
          break;
        default: 
          stopMtrs();
          break;
  }
};


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

void stopMtrs(void){
  RightMtrSpeed(0);
  LeftMtrSpeed(0); 
}

void turnRight(void){
  RightMtrSpeed(-MEDIUM-2);
  LeftMtrSpeed(MEDIUM+2); 
}

void turnLeft(void){
  RightMtrSpeed(MEDIUM);
  LeftMtrSpeed(-MEDIUM); 
}


