
#include <SoftTimer.h>
#include <EEPROM.h>
#include <DelayRun.h>
/*
 Fading

 This example shows how to fade an LED using the analogWrite() function.

 The circuit:
 * LED attached from digital pin 9 to ground.

 Created 1 Nov 2008
 By David A. Mellis
 modified 30 Aug 2011
 By Tom Igoe

 http://www.arduino.cc/en/Tutorial/Fading

 This example code is in the public domain.

 */
#define PWM_PIN 0
#define UP_KEY_PIN 3
#define DOWN_KEY_PIN 4
#define EEPROM_SIZE 512

int currentPWM = 5;
bool keyUp = false;
bool keyDown = false;
bool justsaved = false;
bool needssave = false;

void setPwm(Task* me) {
  analogWrite(PWM_PIN, currentPWM);
}
Task tSetPwm(800, setPwm);

boolean savePwm(Task* me){
  if(EEPROM.read(511) != 0) eeprominit(true);
  for(int i=1; i < EEPROM_SIZE; i++){
    if(EEPROM.read(i) == 0){
      EEPROM.write(i, currentPWM);
      delay(10);
      break;
    }
  }
  needssave = false;
  justsaved = false;
  return false;
}
DelayRun delaySave(5000, savePwm);

void procKeys(Task* me){
  int oldPWM = currentPWM;
  if(keyUp && currentPWM < 255){
    currentPWM++;
    if(currentPWM > 32 && currentPWM < 255){
      currentPWM++;
      if(currentPWM > 128 && currentPWM < 255){
        currentPWM++;
      }
    }
  } else {
    if(keyDown && currentPWM > 1){
      currentPWM--;
      if(currentPWM > 32){
        currentPWM--;
        if(currentPWM > 128){
          currentPWM--;
        }
      }
    }
  }
  if(oldPWM != currentPWM) needssave = true;
}
Task tProcKeys(10000, procKeys);

void getKeys(Task* me) {
  if(digitalRead(UP_KEY_PIN) == LOW){
    keyUp = true;
  } else {
    keyUp = false;
  }
  if(digitalRead(DOWN_KEY_PIN) == LOW){
    keyDown = true;
  } else {
    keyDown = false;
  }
  if(keyDown || keyUp){
    SoftTimer.add(&tProcKeys);
  } else {
    SoftTimer.remove(&tProcKeys);
    if(needssave && !justsaved){
      delaySave.startDelayed();
      justsaved = true;
    }
  }
}
Task tGetKeys(200, getKeys);

void readLastBrigthness(){
  int cAddr = 1;
  while(cAddr < EEPROM_SIZE){
    int cVal = EEPROM.read(cAddr);
    if(cVal == 0) break;
    currentPWM = cVal;
    cAddr++;
  }
}
void eeprominit(){
  eeprominit(false);
}
void eeprominit(bool force){
  if(EEPROM.read(0) != 128 || force){
    for(int i = 1; i < EEPROM_SIZE; i++){
       EEPROM.write(i, 0);
    }
    EEPROM.write(0, 128);
  }
  delay(10);
}

void setup() {
  // nothing happens in setup  
  pinMode(PWM_PIN, OUTPUT);
  digitalWrite(PWM_PIN, LOW);
  delay(10);
  eeprominit();
  readLastBrigthness();
  TCCR0A = 2<<COM0A0 | 2<<COM0B0 | 3<<WGM00;
  TCCR0B = 0<<CS02 | 1<<CS01 | 0<<CS00;
   
  pinMode(PWM_PIN, OUTPUT);
  pinMode(UP_KEY_PIN, INPUT_PULLUP);
  pinMode(DOWN_KEY_PIN, INPUT_PULLUP);
  SoftTimer.add(&tSetPwm);
  SoftTimer.add(&tGetKeys);
  
}



