#include <Bounce2.h>

int bump1 = 5;
int bumpRead = 6;
int val = 0;

void setup(){
Serial.begin(9600);
pinMode(bump1, INPUT);
pinMode(bumpRead, OUTPUT);
}

void loop(){
val = digitalRead(bump1);
Serial.println(val);
}
