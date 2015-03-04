/******************************************************************************
Module
  Roachlib.cpp 
Description
  This library contains code to interface with the ME210 Cocroach hardware.
  It includes functions to control the motors, read the light sensor, and read
  the bumper sensors.

Arduino IDE version:  0022
Arduino UNO R2 resources used
  Analog ch0 - light sensor
  Pin 4, 5, 6, 7 - bumper sensors (LF, RF, RR, LR, respectively)
  Pin 8, 9 - left motor direction and enable
  Pin 10, 11 - right motor direction and enable
  
History
When      Who  Description
--------  ---  -------------------------------------
01/19/12  RMO  created new version of Roachlib.c for the Arduino UNO R2

******************************************************************************/
/*----------------------------- Include Files -------------------------------*/
#if defined(ARDUINO) && ARDUINO >= 100 
#include "Arduino.h"  // if Arduino version 1.0 or later, include Arduino.h
#else
#include "WProgram.h"  // if Arduino version 22, include WProgram.h
#endif

#include "Motorlib.h"

/*----------------------------- Module Defines ------------------------------*/
#define LIGHT_SENSOR 0   // light sensor connected to analog ch 0
#define LF_BUMP      4   // left front bumper sensor connected to pin 4
#define RF_BUMP      5   // right front bumper sensor connected to pin 5
#define RR_BUMP      6   // right rear bumper sensor connected to pin 6
#define LR_BUMP      7   // left rear bumper sensor connected to pin 7

#define L_MOTOR_DIR  8   // left motor direction controlled with pin 8
#define L_MOTOR_EN   6   // left motor enable controlled by pin 9
#define R_MOTOR_DIR  10  // right motor direction controlled with pin 10
#define R_MOTOR_EN   11  // right motor enable controlled by pin 11


//  #define L_MOTOR_DIR  11   // left motor direction controlled with pin 8
//  #define L_MOTOR_EN   10   // left motor enable controlled by pin 9
//  #define R_MOTOR_DIR  9  // right motor direction controlled with pin 10
//  #define R_MOTOR_EN   8  // right motor enable controlled by pin 11

#define SPEED_SCALER 2.5  // map 0-255 PWM settings to 0-100 speed settings


/*----------------------------- Module Variables ---------------------------*/
static unsigned char SharedByte;
static unsigned int SharedWord;

/*----------------------------- Module Code --------------------------------*/
/******************************************************************************
  Function:    RoachInit
  Contents:    Performs all the intitialization necessary for the Cockroach.
               This includes initializing the port pins to interface with the
               bumper sensors and the h-bridges that drive the motors.
  Parameters:  None
  Returns:     Nothing
  Notes:
******************************************************************************/
void MotorInit(void) {
  /* initialize motor control pins */
  PORTB &= 0xF0;  // EN=0 (off) for both motors and also DIR=0 for both motors
  DDRB |= 0x0F;   // make motor EN and DIR pins outputs
  
  /* initialize bumper pins */
  //DDRD &= 0x0F;   // make bumper sensor pins inputs
}

/******************************************************************************
  Function:    LeftMtrSpeed
  Contents:    This function is used to set the speed and direction of the left motor.
  Parameters:  A value between -10 and 10 which is the new speed of the left motor.
                0 stops the motor.  A negative value is reverse.
  Returns:     OK_OPERATION == new speed was successfully sent
               ERR_BADSPEED == an invalid speed was given
  Notes:
******************************************************************************/
RoachReturn_t LeftMtrSpeed(char newSpeed) {
  if ((newSpeed < -100) || (newSpeed > 100)) {
    return ERR_BADSPEED;
  }
  if (newSpeed < 0) {
    digitalWrite(L_MOTOR_DIR,LOW); // set the direction to reverse
  } else {
    digitalWrite(L_MOTOR_DIR,HIGH); // set the direction to forward
  }
  analogWrite(L_MOTOR_EN,SPEED_SCALER*abs(newSpeed));
    return OK_SPEED;
}

/******************************************************************************
  Function:    RightMtrSpeed
  Contents:    This function is used to set the speed and direction of the right motor.
  Parameters:  A value between -10 and 10 which is the new speed of the right motor.
                0 stops the motor.  A negative value is reverse.
  Returns:     OK_OPERATION == new speed was successfully sent
               ERR_BADSPEED == an invalid speed was given
  Notes:
******************************************************************************/
RoachReturn_t RightMtrSpeed(char newSpeed) {
  if ((newSpeed < -100) || (newSpeed > 100)) {
    return ERR_BADSPEED;
  }
  if (newSpeed < 0) {
    digitalWrite(R_MOTOR_DIR,LOW); // set the direction to reverse
  } else {
    digitalWrite(R_MOTOR_DIR,HIGH); // set the direction to forward
  }
  analogWrite(R_MOTOR_EN,SPEED_SCALER*abs(newSpeed));
    return OK_SPEED;
}

/******************************************************************************
  Function:    LightLevel
  Contents:    This function is used to read the A/D converter value for the
                light sensor.
  Parameters:  None
  Returns:     A 10 bit unsigned integer corresponding to the amount of light
                incident on the Cockroach's photocell array.  Higher values
                indicate higher light levels.
  Notes:
******************************************************************************/
unsigned int LightLevel(void) {
   return ((unsigned int)(analogRead(LIGHT_SENSOR)));
}

/******************************************************************************
  Function:    ReadBumpers
  Contents:    This function checks the four bumper sensors and reports whether
                the bumper has been displaced sufficiently to trigger 1 or more
                of the bumper sensors.
  Parameters:  None
  Returns:     An 8 bit value where the upper 4 bits correspond to the bumper
                sensors.  If a bumper is hit, the corresponding bit will be 0,
                otherwise it will be 1.  The lower 4 bits always return 0.
  Notes:
******************************************************************************/
unsigned char ReadBumpers(void) {
  unsigned char bumpers = 0;
  
  bumpers = PIND & 0xF0;
  return bumpers;
}

/******************************************************************************
  Function:    SET_SHARED_BYTE_TO
  Contents:    This function sets the value of the module-level variable
                SharedByte to the new value specified when called.  The data
                is intended to be used immediately afterward using the function
                GET_SHARED_BYTE.
  Parameters:  An 8 bit value.
  Returns:     Nothing
  Notes:
******************************************************************************/
void SET_SHARED_BYTE_TO(unsigned char newByte)
{
  SharedByte = newByte;
}

/******************************************************************************
  Function:    GET_SHARED_BYTE
  Contents:    This function returns the value of the module-level variable
                SharedByte, and is intended to be called immediately after storing
                a value in SharedByte using the SET_SHARED_BYTE_TO function.
  Parameters:  None
  Returns:     An 8 bit value.
  Notes:
******************************************************************************/
unsigned char GET_SHARED_BYTE(void)
{
  return SharedByte;
}

/******************************************************************************
  Function:    SET_SHARED_WORD_TO
  Contents:    This function sets the value of the module-level variable
                SharedWord to the new value specified when called.  The data
                is intended to be used immediately afterward using the function
                GET_SHARED_WORD.
  Parameters:  A 16 bit value.
  Returns:     Nothing
  Notes:
******************************************************************************/
void SET_SHARED_WORD_TO(unsigned int newWord)
{
  SharedWord = newWord;
}

/******************************************************************************
  Function:    GET_SHARED_WORD
  Contents:    This function returns the value of the module-level variable
                SharedWord, and is intended to be called immediately after storing
                a value in SharedWord using the SET_SHARED_WORD_TO function.
  Parameters:  None
  Returns:     A 16 bit value.
  Notes:
******************************************************************************/
unsigned int GET_SHARED_WORD(void)
{
  return SharedWord;
}


