//Library includes////
#include <Roachlib.h>

//Define Module Functions////
unsigned char TestForKey(void);
void RespToKey(void);
void DriveForward(void);
void notMoving(void);


void setup() {
  Serial.begin(9600);
  Serial.println("Motor Test...");
  RoachInit();
}

void loop() {
  DriveForward();
 if (TestForKey()) RespToKey();
}

// Module Functions////
void DriveForward(void){
   
  RightMtrSpeed(10);
  LeftMtrSpeed(10);
  
}

void notMoving(void){
  RightMtrSpeed(0);
  LeftMtrSpeed(0);
} 

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
  
    switch(theKey){
    case(0x77): DriveForward(); break;
    case(0x73): notMoving(); break;
  }
}
